#pragma once

#include <types.h>

// Ethernet header
struct eth_head
{
    u8 mac_dst[6];
    u8 mac_src[6];
    u16 type_field;
};

struct ip_head
{
    // TODO
};