#include <net/net.h>

#include <pmm.h>
#include <util.h>
#include <virtio_net.h>

#include <vga.h>

static struct kmap ip_to_mac;
static struct klist packet_store;   // List that stores all available ipv4 packets

void net_init()
{
    kmap_init(&ip_to_mac, 16, 4, 6);
}

// Map IP to Mac addr
void net_mapping(u8 *ipv4_addr, u8 *mac_addr)
{
    kmap_new(&ip_to_mac, ipv4_addr, mac_addr);
}

/**
  * operation: 0 == request or 1 == response
  * Note: on request dst_mac can be NULL
 */
u8* arp_ipv4_craft_packet(u8 operation, u8 *src_mac, u8 *dst_mac, u8 *src_ip, u8 *dst_ip)
{
    struct eth_head eth_head;

    // Destination mac
    eth_head.mac_dst[0] = 0xff;
    eth_head.mac_dst[1] = 0xff;
    eth_head.mac_dst[2] = 0xff;
    eth_head.mac_dst[3] = 0xff;
    eth_head.mac_dst[4] = 0xff;
    eth_head.mac_dst[5] = 0xff;
    // Source mac
    memcpy(eth_head.mac_src, src_mac, 6);
    // Type
    eth_head.type_field = 0x0608; // ARP

    struct arp_head arp_head;
    
    arp_head.hw_addr_type = 0x0100; // Ethernet
    arp_head.proto_addr_type = 0x0008; // IPv4
    arp_head.hw_addr_size = 6;
    arp_head.proto_addr_size = 4;
    arp_head.operation = 256 << operation; // ARP request
    memcpy(arp_head.src_mac, src_mac, 6);
    memcpy(arp_head.src_ip, src_ip, 4);
    // Req or Res
    if(operation)
    {
        memcpy(arp_head.dst_mac, dst_mac, 6);
    }
    else
    {
        bzero(arp_head.dst_mac, 6);
    }
    memcpy(arp_head.dst_ip, dst_ip, 4);

    // Craft full packet
    u8 *packet = (u8*)kmalloc(
            sizeof(struct eth_head) + 
            sizeof(struct arp_head));

    memcpy(packet, 
           (u8*)&eth_head, 
           sizeof(struct eth_head));

    memcpy(packet + sizeof(struct eth_head), 
           (u8*)&arp_head, 
           sizeof(struct arp_head));

    return packet;
}

static u16 le16_to_be16(u16 x)
{
    u16 low = (x >> 0) & 0xFF;
    u16 high = (x >> 8) & 0xFF;
    return (low << 8) | high;
}

u32 csum_add(u8* buf, size_t len)
{
    u32 csum = 0;

    for(size_t i = 0; i < len; i++)
    {
        if(i & 1)
        {
            csum += (u32)buf[i];
        }
        else
        {
            csum += (u32)buf[i] << 8;
        }
    }

    return csum;
}

u16 csum_fin(u32 csum)
{
    while (csum>>16)
    {
	    csum = (csum & 0xFFFF) + (csum >> 16);
    }
    return ~csum;
}

void compute_ipv4_hdr_csum(struct ipv4_head *hdr)
{
    u32 csum = 0;
    csum = csum_add((u8*)hdr, sizeof(struct ipv4_head));
    hdr->hdr_csum = le16_to_be16(csum_fin(csum));
}

void compute_udp_hdr_csum(struct ipv4_head *ip_hdr, struct udp_head *udp_hdr, u8 *data)
{
    u32 csum = 0;
    udp_hdr->csum = 0;

    csum += csum_add((u8*)&ip_hdr->src_addr, sizeof(u32));
    csum += csum_add((u8*)&ip_hdr->dst_addr, sizeof(u32));
    csum += ip_hdr->proto;
    csum += le16_to_be16(udp_hdr->len);

    csum += csum_add((u8*)udp_hdr, sizeof(struct udp_head));
    csum += csum_add(data, le16_to_be16(udp_hdr->len) - sizeof(struct udp_head));

    udp_hdr->csum = le16_to_be16(csum_fin(csum));
}

u8* udp_ipv4_craft_packet(u8 *src_mac, 
                          u8 *dst_mac, 
                          u8 *src_ip, 
                          u8 *dst_ip, 
                          u16 src_port, 
                          u16 dst_port, 
                          u8 *data, 
                          u16 data_len)
{
    // Ethernet
    struct eth_head eth_head;
    // Destination mac
    memcpy(eth_head.mac_dst, dst_mac, 6);
    // Source mac
    memcpy(eth_head.mac_src, src_mac, 6);
    // Type
    eth_head.type_field = 0x0008; // IPv4

    // IPv4
    struct ipv4_head ip_head;

    ip_head.ver_ihl = (4 << 4) | 5;
    ip_head.tos = 0;
    ip_head.total_length = le16_to_be16(sizeof(struct ipv4_head) + sizeof(struct udp_head) + data_len);
    // TODO: Currently we don't use fragmentation
    ip_head.id = 0;
    ip_head.flags_frag_offset = 0; // Don't fragment option
    ip_head.ttl = 64;
    // TODO: Also allow other protocols
    ip_head.proto = 17; // Always use UDP as next proto in net stack
    // Init for upcoming computation
    ip_head.hdr_csum = 0;
    // Addresses
    memcpy(&ip_head.src_addr, src_ip, 4);
    memcpy(&ip_head.dst_addr, dst_ip, 4);

    // Computes and writes csum to hdr
    compute_ipv4_hdr_csum(&ip_head);   

    // UDP
    struct udp_head udp_head;
    udp_head.src_port = le16_to_be16(src_port);
    udp_head.dst_port = le16_to_be16(dst_port);
    udp_head.len = le16_to_be16(sizeof(struct udp_head) + data_len);
    udp_head.csum = 0;

    // Computes and writes csum to hdr
    compute_udp_hdr_csum(&ip_head, &udp_head, data);

    // Allocate full packet
    u8 *pkt = (u8*)kmalloc(
        sizeof(struct eth_head) + 
        sizeof(struct ipv4_head) + 
        sizeof(struct udp_head) + 
        data_len
    );
    
    // Fill packet

    // Eth
    memcpy(pkt, 
            (u8*)&eth_head, 
            sizeof(struct eth_head));

    // IP
    memcpy(pkt+sizeof(struct eth_head), 
            (u8*)&ip_head, 
            sizeof(struct ipv4_head));

    // UDP
    memcpy(pkt+sizeof(struct eth_head) + 
               sizeof(struct ipv4_head), 
            (u8*)&udp_head, 
            sizeof(struct udp_head));

    // Payload
    memcpy(pkt+sizeof(struct eth_head) + 
               sizeof(struct ipv4_head)+
               sizeof(struct udp_head),
               data,
               data_len);

    // Return packet
    return pkt;
}

static bool mac_is_broadcast(u8 *mac)
{
    for(size_t i = 0; i < 6; i++)
    {
        if(mac[i] != 0xFF)
            return false;
    }
    return true;
}

struct ipv4_packet* new_ipv4_packet()
{
    struct ipv4_packet *p = (struct ipv4_packet*)kmalloc(sizeof(struct ipv4_packet));
    p->packet_pointer = NULL;
    p->list_handle.valid = false;
    p->list_handle.next = NULL;
    return p;
}

extern virtio_net_dev_t *main_net_dev;

// Respond to ARP packages
void net_handle_arp_packet(void *pkt)
{
    // Get Pointer to Arp part of package
    struct eth_head *eth_hdr = (struct eth_head*)pkt;
    struct arp_head *arp_hdr = (struct arp_head*)((u8*)pkt + sizeof(struct eth_head));

    // Check if ARP packet is for us
    if(mac_is_broadcast(eth_hdr->mac_dst) && kmap_contains(&ip_to_mac, arp_hdr->dst_ip))
    {
        // Get own mac
        u8 mac[6];
        kmap_get(&ip_to_mac, arp_hdr->dst_ip, mac);
        // Craft response and reply
        u8 *arp_res = arp_ipv4_craft_packet(1, mac, arp_hdr->src_mac, arp_hdr->dst_ip, arp_hdr->src_ip);
        // Send response
        virtio_net_dev_send(main_net_dev, arp_res, sizeof(struct eth_head) + sizeof(struct arp_head));
    }

    // Free packet (arp packets do not need to be stored)
    kfree((i64)pkt);
}

void net_handle_ipv4_packet(void *pkt)
{
    // Places the packet in global list to make it able to be received by applications, we store the pointer to it outside of the virtqueue as the virtio queue entry will be overwritten

    // TODO: Locking!!!!

    struct ipv4_packet *packet = new_ipv4_packet();
    packet->packet_pointer = pkt;
    klist_push(&packet_store, &packet->list_handle);
}

u32 htoni(u32 val)
{
    return (((val >> 24) & 0xFF) << 0) | 
           (((val >> 16) & 0xFF) << 8) |
           (((val >> 8) & 0xFF) << 16) |
           (((val >> 0) & 0xFF) << 24);
}

u16 htons(u16 val)
{
    return ((val & 0xFF) << 8) | (val >> 8);
}

/**
 *  @return: Returns pointer to packet in pkt
 */
void net_receive_udp_packet(u32 addr, u16 port, void **pkt)
{
    // No packages to return
    if(klist_empty(&packet_store)) {
        *pkt = (void*)(-1);
        return;
    }
    // Iterate over packet list
    struct klist_node *curr = packet_store.root;
    // Find packet with right address and port
    do {
        // Current ipv4 packet
        struct ipv4_packet *pack = ENCLAVE(struct ipv4_packet, list_handle, curr);
        // Parse packet
        struct ipv4_head *ip_hdr = (struct ipv4_head*)((u8*)pack->packet_pointer + sizeof(struct eth_head));
        struct udp_head *udp_hdr = (struct udp_head*)((u8*)pack->packet_pointer + sizeof(struct eth_head) + ((ip_hdr->ver_ihl & 0xF) * 4));   // Account for ip hdr length
        // Check properties
        if (ip_hdr->dst_addr == addr && udp_hdr->dst_port == port) {
            // Return package
            *pkt = pack->packet_pointer;
            // Remove from packet store
            klist_pop(&packet_store, &pack->list_handle);
            kfree((i64)pack);
            // Finished
            return;
        }
        // Next packet
        curr = curr->next;
    } while(curr->valid);

    *pkt = (void*)(-1);
    return;
}

// NOTE: This function does a busy wait
struct udp_data net_receive_udp_packet_blocking(int addr, int port)
{
    // Wait for a packet to arrive
    void *p = NULL;   
    net_receive_udp_packet(addr, port, &p);
    while(p == (void*)-1)
    {
        net_receive_udp_packet(addr, port, &p);    
    }

    // Parse packet and get pointer to data
    struct ipv4_head *ip_hdr = (struct ipv4_head*)((u8*)p + sizeof(struct eth_head));
    u8 *data = (u8*)((u8*)p + sizeof(struct eth_head) + ((ip_hdr->ver_ihl & 0xF) * 4) + sizeof(struct udp_head));

    // Build return value
    struct udp_data ret;
    ret.packet_pointer = p;
    ret.data_pointer = data;

    return ret;
}

void net_handle_packet(void *pkt_ptr)
{
    struct eth_head *eth_hdr = (struct eth_head*)pkt_ptr;

    switch(eth_hdr->type_field)
    {
        case 0x0608: // ARP
            // TODO: Remove
            kprintf("Got ARP\n");
            net_handle_arp_packet(pkt_ptr);
            break;
        
        case 0x0008: // IPv4
            // TODO: Remove
            kprintf("Got IP\n");
            net_handle_ipv4_packet(pkt_ptr);
            break;

        default:
            // Just drop package by freeing memory
            kfree((i64)pkt_ptr);
            break;
    }
}