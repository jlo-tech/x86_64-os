#pragma once

#include <types.h>

// Ethernet header
struct eth_head
{
    u8 mac_dst[6];
    u8 mac_src[6];
    u16 type_field;
} __attribute__((packed));

struct arp_head
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
} __attribute__((packed));

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
    u32 src_addr;
    u32 dst_addr;
    // u8 options[];
} __attribute__((packed));

struct udp_head
{
    u16 src_port;
    u16 dst_port;
    u16 len;
    u16 csum;
} __attribute__((packed));

u8* udp_ipv4_craft_packet(u8 *src_mac, 
                          u8 *dst_mac, 
                          u8 *src_ip, 
                          u8 *dst_ip, 
                          u16 src_port, 
                          u16 dst_port, 
                          u8 *data, 
                          u16 data_len);
