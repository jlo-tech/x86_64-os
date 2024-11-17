#pragma once

#include <virtio.h>

/* Feature bits */
#define VIRTIO_NET_F_CSUM           (0)
#define VIRTIO_NET_F_GUEST_CSUM     (1)
#define VIRTIO_NET_F_MTU            (3)
#define VIRTIO_NET_F_MAC            (5)
#define VIRTIO_NET_F_STATUS         (16)
#define VIRTIO_NET_F_CTRL_VQ        (17)
#define VIRTIO_NET_F_CTRL_RX        (18)
#define VIRTIO_NET_F_CTRL_VLAN      (19)
#define VIRTIO_NET_F_GUEST_ANNOUNCE (21)
#define VIRTIO_NET_F_CTRL_MAC_ADDR  (23)

/* Field offsets of the virtio_net_config structure 
   (offsets may change depending on negotiated features) */
#define VIRTIO_NET_CFG_MAC              0x14
#define VIRTIO_NET_CFG_STATUS           0x1A
#define VIRTIO_NET_CFG_MAX_VIRTQ_PAIRS  0x1C
#define VIRTIO_NET_CFG_MTU              0x1E

// Status bits
#define VIRTIO_NET_S_LINK_UP     1 
#define VIRTIO_NET_S_ANNOUNCE    2 

// Virtio net hdr related stuff
#define VIRTIO_NET_HDR_F_NEEDS_CSUM    1 
#define VIRTIO_NET_HDR_F_DATA_VALID    2 
#define VIRTIO_NET_HDR_F_RSC_INFO      4 

#define VIRTIO_NET_HDR_GSO_NONE        0 
#define VIRTIO_NET_HDR_GSO_TCPV4       1 
#define VIRTIO_NET_HDR_GSO_UDP         3 
#define VIRTIO_NET_HDR_GSO_TCPV6       4 
#define VIRTIO_NET_HDR_GSO_ECN      0x80 

struct virtio_net_hdr 
{ 
    u8 flags;
    u8 gso_type; 
    u16 hdr_len; 
    u16 gso_size; 
    u16 csum_start; 
    u16 csum_offset; 
    //u16 num_buffers; (only when VIRTIO_NET_F_MRG_RXBUF was negotiated)
};

typedef struct virtio_net_dev
{
    virtio_dev_t *virtio_dev;
} virtio_net_dev_t;

bool virtio_net_dev_init(virtio_net_dev_t *net_dev, virtio_dev_t *virtio_dev);
bool virtio_net_dev_send(virtio_net_dev_t *net_dev, u8 *packet, size_t packet_len);

void virtio_register_net_device(virtio_net_dev_t *net_dev);
void virtio_net_irq_handler();