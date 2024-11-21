#include <net/net.h>

#include <util.h>

struct arp_pkt arp_ipv4_craft_package(u8 *src_mac, u8 *src_ip, u8 *dst_ip)
{
    struct arp_pkt packet;
    
    packet.hw_addr_type = 0x0100; // Ethernet
    packet.proto_addr_type = 0x0008; // IPv4
    packet.hw_addr_size = 6;
    packet.proto_addr_size = 4;
    packet.operation = 256; // ARP request
    memcpy(packet.src_mac, src_mac, 6);
    memcpy(packet.src_ip, src_ip, 4);
    bzero(packet.dst_mac, 6);
    memcpy(packet.dst_ip, dst_ip, 4);

    return packet;
}

u8* udp_craft_package(u8 *src_mac, u8 *dst_mac, 
                      u8 *src_ip, u8 *dst_ip, 
                      u16 src_port, u16 dst_port, 
                      u8 *data, size_t data_len)
{
    // TODO
}