#pragma once

#include <io.h>
#include <pmm.h>
#include <pci.h>
#include <vga.h>
#include <types.h>

/*
 * Offsets in Virtio config space
 */
#define VIRTIO_HEADER_DEVICE_FEATURES 0x0
#define VIRTIO_HEADER_GUEST_FEATURES  0x4
#define VIRTIO_HEADER_QUEUE_ADDRESS   0x8
#define VIRTIO_HEADER_QUEUE_SIZE      0xC
#define VIRTIO_HEADER_QUEUE_SELECT    0xE
#define VIRTIO_HEADER_QUEUE_NOTIFY    0x10
#define VIRTIO_HEADER_DEVICE_STATUS   0x12
#define VIRTIO_HEADER_ISR_STATUS      0x13
#define VIRTIO_HEADER_DEVICE_OFFSET   0x14 // Offset where the device specific part starts

/* Buffer continues on next field */
#define VRING_DESC_F_NEXT 1

/* Buffer is write only */
#define VRING_DESC_F_WRITE 2

/* No interrupt when device puts buffer into used queue (unreliable, only optimization) */
#define VRING_USED_F_NO_NOTIFY 1

/* No interrupt when device consumes buffer, i.e. reads from avail queue (unreliable, only optimization) */
#define VRING_AVAIL_F_NO_INTERRUPT 1

struct virtq_desc // aka vring_desc
{
    // Guest physical address
    u64 addr;
    // Length
    u32 len;
    // Flags
    u16 flags;
    // Chain descriptors
    u16 next;
} __attribute__((packed));

struct virtq_avail // aka vring_avail
{
    u16 flags;
    u16 idx;
    u16 ring[];
    //u16 used_event;
} __attribute__((packed));

struct virtq_used_elem // aka vring_used_elem
{
    // Start index of used descriptor chain
    u32 id;
    // Total length of chain it was written to
    u32 len;
} __attribute__((packed));

struct virtq_used // aka vring_used
{
    u16 flags;
    u16 idx;
    struct virtq_used_elem ring[];
    //u16 avail_event;
} __attribute__((packed));

// Note: not actual struct in memory
struct virtq // aka vring
{
    i64 elems; // Number of elems in queue
    i64 space; // Physical addr/mem space holding queues
    i64 dptr;  // Points to next free descriptor
    struct virtq_desc  *desc;
    struct virtq_avail *avail;
    struct virtq_used  *used;
};

typedef struct virtio_device 
{
    u16 num_queues;
    pci_dev_t *pci_dev;
    struct virtq *virtqs;
} virtio_dev_t;

bool virtio_dev_init(virtio_dev_t *virtio_dev, pci_dev_t *pci_dev, u16 num_queues);
bool virtio_dev_deinit(virtio_dev_t *virtio_dev);
bool virtio_dev_reset(virtio_dev_t *virtio_dev);

bool virtio_create_queue(virtio_dev_t *virtio_dev, u16 queue_num);
bool virtio_deploy(virtio_dev_t *virtio_dev, u16 queue_num, struct virtq_desc *descriptors, u16 num_descriptors);
