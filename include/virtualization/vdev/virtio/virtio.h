/**
 * @file virtio.h
 * @brief VirtIO Core Framework Interface
 */

#ifndef ZEPHYR_INCLUDE_ZVM_VIRTIO_H_
#define ZEPHYR_INCLUDE_ZVM_VIRTIO_H_

#include <stdint.h>
#include <spinlock.h>
#include <sys/dlist.h>
#include <virtualization/vm.h>
#include <virtualization/vdev/virtio/virtio_ids.h>
#include <virtualization/vdev/virtio/virtio_config.h>
#include <virtualization/vdev/virtio/virtio_ring.h>

#define VIRTIO_DEVICE_MAX_NAME_LEN		64

#define VIRTIO_IRQ_LOW			0
#define VIRTIO_IRQ_HIGH			1

struct vm;
struct virtio_device;

/* read from the vring_desc*/
struct virtio_iovec {
	uint64_t addr;
	uint32_t len;
	uint16_t flags;
};

struct virtio_queue {
	/* The last_avail_idx field is an index to ->ring of struct vring_avail.
	   It's where we assume the next request index is at.  */
	uint16_t			last_avail_idx;
	uint16_t			last_used_signalled;

	struct vring	vring;

	struct vm	*guest;
	uint32_t			desc_count;
	uint32_t			align;
	physical_addr_t		guest_pfn;
	physical_size_t		guest_page_size;	
	physical_addr_t		guest_addr;
	physical_addr_t		host_addr;
	physical_size_t		total_size;
};

struct virtio_device_id {
	uint32_t type;
};

struct virtio_device {
	char name[VIRTIO_DEVICE_MAX_NAME_LEN];
	struct virt_dev *edev;

	struct virtio_device_id id;

	struct virtio_transport *tra;
	void *tra_data;

	struct virtio_emulator *emu;
	void *emu_data;

	sys_dnode_t node;
	struct vm *guest;
};

struct virtio_transport {
	const char *name;

	int  (*notify)(struct virtio_device *, uint32_t vq);
};

struct virtio_emulator {
	const char *name;
	const struct virtio_device_id *id_table;

	/* VirtIO operations */
	uint64_t (*get_host_features) (struct virtio_device *dev);
	void (*set_guest_features) (struct virtio_device *dev,
				    uint32_t select, uint32_t features);
	int (*init_vq) (struct virtio_device *dev, uint32_t vq, uint32_t page_size,
			uint32_t align, uint32_t pfn);
	int (*get_pfn_vq) (struct virtio_device *dev, uint32_t vq);		
	int (*get_size_vq) (struct virtio_device *dev, uint32_t vq);
	int (*set_size_vq) (struct virtio_device *dev, uint32_t vq, int size);
	int (*notify_vq) (struct virtio_device *dev, uint32_t vq);
	void (*status_changed) (struct virtio_device *dev,
				uint32_t new_status);

	/* Emulator operations */
	int (*read_config)(struct virtio_device *dev,
			   uint32_t offset, void *dst, uint32_t dst_len);
	int (*write_config)(struct virtio_device *dev,
			    uint32_t offset, void *src, uint32_t src_len);
	int (*reset)(struct virtio_device *dev);
	int  (*connect)(struct virtio_device *dev,
			struct virtio_emulator *emu);
	void (*disconnect)(struct virtio_device *dev);

	sys_dnode_t node;
};

int virtio_dev_list_init();
int virtio_drv_list_init();

/** Get guest to which the queue belongs
 *  Note: only available after queue setup is done
 */
struct vm *virtio_queue_guest(struct virtio_queue *vq);

/** Get maximum number of descriptors in queue
 *  Note: only available after queue setup is done
 */
uint32_t virtio_queue_desc_count(struct virtio_queue *vq);

/** Get queue alignment
 *  Note: only available after queue setup is done
 */
uint32_t virtio_queue_align(struct virtio_queue *vq);

/** Get guest page frame number of queue
 *  Note: only available after queue setup is done
 */
physical_addr_t virtio_queue_guest_pfn(struct virtio_queue *vq);

/** Get guest page size for this queue
 *  Note: only available after queue setup is done
 */
physical_size_t virtio_queue_guest_page_size(struct virtio_queue *vq);

/** Get guest physical address of this queue
 *  Note: only available after queue setup is done
 */
physical_addr_t virtio_queue_guest_addr(struct virtio_queue *vq);

/** Get host physical address of this queue
 *  Note: only available after queue setup is done
 */
physical_addr_t virtio_queue_host_addr(struct virtio_queue *vq);

/** Get total physical space required by this queue
 *  Note: only available after queue setup is done
 */
physical_size_t virtio_queue_total_size(struct virtio_queue *vq);

/** Retrive maximum number of vring descriptors
 *  Note: works only after queue setup is done
 */
uint32_t virtio_queue_max_desc(struct virtio_queue *vq);

/** Retrive vring descriptor at given index
 *  Note: works only after queue setup is done
 */
int virtio_queue_get_desc(struct virtio_queue *vq, uint16_t indx,
			      struct vring_desc *desc);

/** Pop the index of next available descriptor
 *  Note: works only after queue setup is done
 */
uint16_t virtio_queue_pop(struct virtio_queue *vq);

/** Check whether any descriptor is available or not
 *  Note: works only after queue setup is done
 */
bool virtio_queue_available(struct virtio_queue *vq);

/** Check whether queue notification is required
 *  Note: works only after queue setup is done
 */
bool virtio_queue_should_signal(struct virtio_queue *vq);

/** Update avail_event in vring
 *  Note: works only after queue setup is done
 */
void virtio_queue_set_avail_event(struct virtio_queue *vq);

/** Update used element in vring
 *  Note: works only after queue setup is done
 */
void virtio_queue_set_used_elem(struct virtio_queue *vq,
				    uint32_t head, uint32_t len);

/** Check whether queue setup is done by guest or not */
bool virtio_queue_setup_done(struct virtio_queue *vq);

/** Cleanup or reset the queue
 *  Note: After cleanup we need to setup queue before reusing it.
 */
bool virtio_queue_cleanup(struct virtio_queue *vq);

/** Setup or initialize the queue
 *  Note: If queue was already setup then it will cleanup first.
 */
bool virtio_queue_setup(struct virtio_queue *vq,
			   struct vm *guest,
			   physical_addr_t guest_pfn,
			   physical_size_t guest_page_size,
			   uint32_t desc_count, uint32_t align);

/** Get guest IO vectors based on given head
 *  Note: works only after queue setup is done
 */
bool virtio_queue_get_head_iovec(struct virtio_queue *vq,
				    uint16_t head, struct virtio_iovec *iov,
				    uint32_t *ret_iov_cnt, uint32_t *ret_total_len,
				    uint16_t *ret_head);

/** Get guest IO vectors based on current head
 *  Note: works only after queue setup is done
 */
bool virtio_queue_get_iovec(struct virtio_queue *vq,
			       struct virtio_iovec *iov,
			       uint32_t *ret_iov_cnt, uint32_t *ret_total_len,
			       uint16_t *ret_head);

/** Read contents from guest IO vectors to a buffer */
uint32_t virtio_iovec_to_buf_read(struct virtio_device *dev,
				 struct virtio_iovec *iov,
				 uint32_t iov_cnt, void *buf,
				 uint32_t buf_len);

/** Write contents to guest IO vectors from a buffer */
uint32_t virtio_buf_to_iovec_write(struct virtio_device *dev,
				  struct virtio_iovec *iov,
				  uint32_t iov_cnt, void *buf,
				  uint32_t buf_len);

/** Fill guest IO vectors with zeros */
void virtio_iovec_fill_zeros(struct virtio_device *dev,
				 struct virtio_iovec *iov,
				 uint32_t iov_cnt);

/** Read VirtIO device configuration */
int virtio_config_read(struct virtio_device *dev,
			   uint32_t offset, void *dst, uint32_t dst_len);

/** Write VirtIO device configuration */
int virtio_config_write(struct virtio_device *dev,
			    uint32_t offset, void *src, uint32_t src_len);

/** Reset VirtIO device */
int virtio_reset(struct virtio_device *dev);

/** Register VirtIO device */
int virtio_register_device(struct virtio_device *dev);

/** UnRegister VirtIO device */
void virtio_unregister_device(struct virtio_device *dev);

/** Register VirtIO device emulator */
int virtio_register_emulator(struct virtio_emulator *emu);

/** UnRegister VirtIO device emulator */
void virtio_unregister_emulator(struct virtio_emulator *emu);

#endif /* ZEPHYR_INCLUDE_ZVM_VIRTIO_H_ */
