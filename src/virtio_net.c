#include <virtio_net.h>

#include <pmm.h>
#include <sync.h>
#include <util.h>

// Main network device the kernel operates on
// TODO: Build advanced data structure to handle multiple devices and map queues to devices
static virtio_net_dev_t *main_net_dev;

static struct kqueue net_ring;

bool virtio_net_dev_init(virtio_net_dev_t *net_dev, virtio_dev_t *virtio_dev)
{
     // Save device for later
     net_dev->virtio_dev = virtio_dev;

     // Get virtio device's io offset
     u32 iobase = pci_bar(virtio_dev->pci_dev, 0);
    
     // Check error
     if(iobase == 0xFFFFFFFF)
        return false;

     // Get only address from bar
     iobase &= 0xFFFFFFFC;

     // Reset device
     virtio_dev_reset(virtio_dev);

     // VirtIO Spec 3.1.1
    
     outb(iobase + VIRTIO_HEADER_DEVICE_STATUS, 
          VIRTIO_HEADER_DEV_S_ACK | 
          VIRTIO_HEADER_DEV_S_DRV);

     // Read feature bits
     u32 features_offered = ind(iobase + VIRTIO_HEADER_DEVICE_FEATURES);

     // Set accepted features (as mask)
     u32 features_accepted = 
         VIRTIO_NET_F_GUEST_CSUM |
         VIRTIO_NET_F_MTU |
         VIRTIO_NET_F_MAC |
         VIRTIO_NET_F_STATUS;

     outd(iobase + VIRTIO_HEADER_GUEST_FEATURES, 
          features_offered & features_accepted);

     // Freeze features
     outb(iobase + VIRTIO_HEADER_DEVICE_STATUS, 
          VIRTIO_HEADER_DEV_S_ACK | 
          VIRTIO_HEADER_DEV_S_DRV | 
          VIRTIO_HEADER_DEV_S_F_OK);

     // Setup VirtQueues
     bool succ;
     // Recv queue
     succ = virtio_create_queue(virtio_dev, 0);
     if(!succ)
          return false;
     // Send queue
     succ = virtio_create_queue(virtio_dev, 1);
     if(!succ)
          return false;

     // Set final status
     outb(iobase + VIRTIO_HEADER_DEVICE_STATUS, 
          VIRTIO_HEADER_DEV_S_ACK | 
          VIRTIO_HEADER_DEV_S_DRV | 
          VIRTIO_HEADER_DEV_S_F_OK | 
          VIRTIO_HEADER_DEV_S_D_OK);

    return true;
}

// Retrieves mac address from virtio device
// mac: Output buffer for the mac address
bool virtio_net_dev_mac(virtio_net_dev_t *net_dev, u8 *mac)
{
     // Get virtio device's io offset
     u32 iobase = pci_bar(net_dev->virtio_dev->pci_dev, 0);
    
     // Check error
     if(iobase == 0xFFFFFFFF)
        return false;

     // Get only address from bar
     iobase &= 0xFFFFFFFC;

     for(int i = 0; i < 6; i++)
     {
          mac[i] = inb(iobase + VIRTIO_HEADER_DEVICE_OFFSET + i);
     }

     return true;
}

void virtio_net_dev_send(virtio_net_dev_t *net_dev, u8 *packet, size_t packet_len)
{
     // VirtIO Spec 5.1.6.2

     // Craft virtio_net_hdr
     struct virtio_net_hdr *vnh = 
          (struct virtio_net_hdr*)kmalloc(sizeof(struct virtio_net_hdr));
     vnh->flags = 0;
     vnh->gso_type = VIRTIO_NET_HDR_GSO_NONE;
     vnh->hdr_len = 0;
     vnh->gso_size = 0;
     vnh->csum_start = 0;
     vnh->csum_offset = 0;

     // Create descriptors 
     //   (net header and packet should end up each in their 
     //    own descriptors)
     struct virtq_desc *desc = 
          (struct virtq_desc*)kmalloc(2 * sizeof(struct virtq_desc));
     
     desc[0].addr  = (u64)vnh;
     desc[0].len   = sizeof(struct virtio_net_hdr);
     desc[0].flags = VRING_DESC_F_NEXT;
     
     desc[1].addr  = (u64)packet;
     desc[1].len   = packet_len;
     desc[1].flags = 0;

     BARRIER

     // Deploy (to transmit queue) and notify device
     virtio_deploy(net_dev->virtio_dev, 1, desc, 2);

     // Free resources after virtio_deploy() copied them
     kfree((i64)desc);
}

/**
 * This method cleans all allocated buffers from the virtq on a virtio interrupt
 */
void virtio_net_dev_send_cleanup(virtio_net_dev_t *net_dev)
{
     // Free already used descriptors by iterating through virtio used queue
     static i64 last_used_idx = 0;

     u16 used_idx = net_dev->virtio_dev->virtqs[1].used->idx;

     for(i64 i = last_used_idx; i < used_idx; i++)
     {
          struct virtq_desc *local_desc;
          // Query current hdr descriptor
          local_desc = (struct virtq_desc*)
               &net_dev->virtio_dev->virtqs[1].desc[
                    net_dev->virtio_dev->virtqs[1].used->ring[i].id+0];     
          // Free hdr buffer
          kfree(local_desc->addr);

          // Query current packet descriptor
          local_desc = (struct virtq_desc*)
               &net_dev->virtio_dev->virtqs[1].desc[
                    net_dev->virtio_dev->virtqs[1].used->ring[i].id+1];
          // Free packet buffer
          kfree(local_desc->addr);
     }

     last_used_idx = used_idx;
}

// Queries packet from system wide queue
void virtio_net_dev_recv(virtio_net_dev_t *net_dev, u8 **packet)
{
     kqueue_dequeue(&net_ring, (void**)packet);
}

// Places new buffers in the recv queue, 
void virtio_net_dev_recv_alloc(virtio_net_dev_t *net_dev)
{
     // Details at VirtIO Spec 1.1: 5.1.6.4
     // Craft virtio_net_hdr
     struct virtio_net_hdr *vnh = 
          (struct virtio_net_hdr*)kmalloc(sizeof(struct virtio_net_hdr));
     vnh->flags = 0;
     vnh->gso_type = VIRTIO_NET_HDR_GSO_NONE;
     vnh->hdr_len = 0;
     vnh->gso_size = 0;
     vnh->csum_start = 0;
     vnh->csum_offset = 0;

     // Create descriptors 
     //   (net header and packet should end up each in their 
     //    own descriptors)
     struct virtq_desc *desc = 
          (struct virtq_desc*)kmalloc(2 * sizeof(struct virtq_desc));
     
     desc[0].addr  = (u64)vnh;
     desc[0].len   = sizeof(struct virtio_net_hdr);
     desc[0].flags = VRING_DESC_F_NEXT | VRING_DESC_F_WRITE;
     
     // Maximal packet size we expect
     u32 packet_size = 1514; // Max size of a ethernet frame

     desc[1].addr  = (u64)kmalloc(packet_size);
     desc[1].len   = packet_size;
     desc[1].flags = VRING_DESC_F_WRITE;

     BARRIER

     // Deploy (to receive queue) and notify device
     virtio_deploy(net_dev->virtio_dev, 0, desc, 2);

     // Free resources after virtio_deploy() copied them
     kfree((i64)desc);
}

// Copies buffers that contain network packets into systems ring buffer
void virtio_net_dev_recv_cleanup(virtio_net_dev_t *net_dev, struct kqueue *net_ring)
{
     // Free already used descriptors by iterating through virtio used queue
     static i64 last_used_idx = 0;

     u16 used_idx = net_dev->virtio_dev->virtqs[0].used->idx;

     for(i64 i = last_used_idx; i < used_idx; i++)
     {
          struct virtq_desc *local_desc;

          // Query current hdr descriptor
          local_desc = (struct virtq_desc*)
               &net_dev->virtio_dev->virtqs[0].desc[
                    net_dev->virtio_dev->virtqs[0].used->ring[i].id+0];
               
          // Free hdr buffer
          kfree(local_desc->addr);

          // Query current packet descriptor
          local_desc = (struct virtq_desc*)
               &net_dev->virtio_dev->virtqs[0].desc[
                    net_dev->virtio_dev->virtqs[0].used->ring[i].id+1];

          // Insert pointer to packet buffer into system wide ring buffer
          kqueue_enqueue(net_ring, (void*)local_desc->addr);
     }

     // Allocate new recv buffers after old ones were retreived
     for(i64 j = 0; j < (used_idx - last_used_idx); j++)
     {
          virtio_net_dev_recv_alloc(net_dev);
     }

     last_used_idx = used_idx;
}

void virtio_net_init(virtio_net_dev_t *net_dev)
{
     // Register main net device
     main_net_dev = net_dev;

     // Capacity of recv virtq
     i64 recv_virtq_elems = net_dev->virtio_dev->virtqs[0].elems;

     // Init net queue
     kqueue_init(&net_ring, recv_virtq_elems);

     // Allocate recv buffers
     for(int i = 0; i < recv_virtq_elems; i++)
     {
          virtio_net_dev_recv_alloc(net_dev);
     }
}

void virtio_net_irq_handler()
{
     // Get iobase of device
     u32 iobase = pci_bar(main_net_dev->virtio_dev->pci_dev, 0);
     // Get only address from bar
     iobase &= 0xFFFFFFFC;
     // Reset ISR by reading it
     u8 isr = inb(iobase + VIRTIO_HEADER_ISR_STATUS);
     // Check for virtq changes
     if((isr & 1))
     {
          // Cleanup send stuff
          virtio_net_dev_send_cleanup(main_net_dev);
          // Cleanup recv stuff
          virtio_net_dev_recv_cleanup(main_net_dev, &net_ring);
     }
}