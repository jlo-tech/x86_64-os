#pragma once

#include <types.h>

// Ethernet header
struct eth_head
{
    u8 mac_dst[6];
    u8 mac_src[6];
    u16 type_field;
};

struct arp_pkt
{
    u16 hw_addr_type;
    u16 proto_addr_type;
    u8 hw_addr_size;
    u8 proto_addr_size;
    u16 operation;
    u8 src_mac[6];
    u8 src_ip[4];
    u8 dst_mac[6];
    u8 dst_ip[4];
};

struct ipv4_head
{
    u8 ver_ihl;
    u8 tos;
    u16 total_length;
    u16 id;
    u16 flags_frag_offset;
    u8 ttl;
    u8 proto;
    u16 hdr_csum;
    u32 source_addr;
    u32 dest_addr;
    // u8 options[];
};

struct udp_head
{
    u16 src_port;
    u16 dst_port;
    u16 len;
    u16 csum;
};

struct arp_pkt arp_ipv4_craft_package(u8 *src_mac, 
                                       u8 *src_ip, 
                                       u8 *dst_ip);

struct udp_head udp_craft_header(u8 *src_mac, u8 *dst_mac, 
                      u8 *src_ip, u8 *dst_ip, 
                      u16 src_port, u16 dst_port, 
                      u8 *data, size_t data_len);