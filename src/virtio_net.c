#include <virtio_net.h>

#include <pmm.h>

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

bool virtio_net_dev_send(virtio_net_dev_t *net_dev, u8 *packet, size_t packet_len)
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
     //   (net header and packet should end each in their 
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

     return true;
}

// TODO: Recv

/**
 * This method cleans all allocated buffers from the virtq on a virtio interrupt
 */
void virtio_net_dev_send_cleanup(virtio_net_dev_t *net_dev)
{
     // Loop through second queue (idx 1) which is the send queue
     i64 elems = net_dev->virtio_dev->virtqs[1].elems;

     // Free already used descriptors by iterating through virtio used queue
     static i64 last_used_idx = 0;

     i64 i = last_used_idx;

     while(((i + 1) % elems) == net_dev->virtio_dev->virtqs[1].used->idx)
     {
          i64 chain_length = 0;
          // Loop through all chunks of descriptor chain
          while(1)
          {
               // Query current descriptor
               struct virtq_desc *local_desc = (struct virtq_desc*)
                    &net_dev->virtio_dev->virtqs[1].desc[i+chain_length];
               
               // Free buffer
               kfree(local_desc->addr);

               // Increase chain length 
               // (possible since virtio_deploy() places buffer one after another)
               chain_length++;
               
               // Continue with next buffer when arrived at end of chain
               if((local_desc->flags & VRING_DESC_F_NEXT) == 0)
               {
                    break;
               }
          }

          // Increase counter
          i += chain_length;
     }

     last_used_idx = i;
}

// Main network device the kernel operates on
// TODO: Build advanced data structure to handle multiple devices
static virtio_net_dev_t *main_net_dev;

void virtio_register_net_device(virtio_net_dev_t *net_dev)
{
     // Register main net device
     main_net_dev = net_dev;
}

void virtio_net_irq_handler()
{
     virtio_net_dev_send_cleanup(main_net_dev);

     // TODO: Read cleanup
}
