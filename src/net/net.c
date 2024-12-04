#include <net/net.h>

#include <pmm.h>
#include <util.h>

u8* arp_ipv4_craft_packet(u8 *src_mac, u8 *src_ip, u8 *dst_ip)
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
    arp_head.operation = 256; // ARP request
    memcpy(arp_head.src_mac, src_mac, 6);
    memcpy(arp_head.src_ip, src_ip, 4);
    bzero(arp_head.dst_mac, 6);
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