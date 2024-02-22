/**
 * @file virtio.c
 * @brief VirtIO Para-virtualization Framework Implementation
 */

#include <stdbool.h>
#include <string.h>
#include <zephyr.h>
#include <spinlock.h>
#include <sys/dlist.h>

#include <virtualization/vm.h>
#include <virtualization/vdev/virtio/virtio.h>

static sys_dlist_t virtio_devs;
static sys_dlist_t virtio_emus;
static struct k_spinlock virtio_lock;

int virtio_dev_list_init()
{
    sys_dlist_init(&virtio_devs);
    return 0;
}

int virtio_drv_list_init()
{
    sys_dlist_init(&virtio_emus);
    return 0;
}

struct vm *virtio_queue_guest(struct virtio_queue *vq)
{
	return (vq) ? vq->guest : NULL;
}

uint32_t virtio_queue_desc_count(struct virtio_queue *vq)
{
	return (vq) ? vq->desc_count : 0;
}

uint32_t virtio_queue_align(struct virtio_queue *vq)
{
	return (vq) ? vq->align : 0;
}

physical_addr_t virtio_queue_guest_pfn(struct virtio_queue *vq)
{
	return (vq) ? vq->guest_pfn : 0;
}

physical_size_t virtio_queue_guest_page_size(struct virtio_queue *vq)
{
	return (vq) ? vq->guest_page_size : 0;
}

physical_addr_t virtio_queue_guest_addr(struct virtio_queue *vq)
{
	return (vq) ? vq->guest_addr : 0;
}

physical_addr_t virtio_queue_host_addr(struct virtio_queue *vq)
{
	return (vq) ? vq->host_addr : 0;
}

physical_size_t virtio_queue_total_size(struct virtio_queue *vq)
{
	return (vq) ? vq->total_size : 0;
}

uint32_t virtio_queue_max_desc(struct virtio_queue *vq)
{
	if (!vq || !vq->guest) {
		return 0;
	}

	return vq->desc_count;
}

int virtio_queue_get_desc(struct virtio_queue *vq, uint16_t indx,
			      struct vring_desc *desc)
{
	physical_addr_t desc_gpa;

	if (!vq || !vq->guest || !desc) {
		return -EINVAL;
	}

	desc_gpa = vq->vring.desc_base_gpa + indx * sizeof(*desc);
	vm_guest_memory_read(vq->guest, desc_gpa, desc, sizeof(*desc));

	return 0;
}

uint16_t virtio_queue_pop(struct virtio_queue *vq)
{
	uint16_t val;
	uint32_t idx;
	physical_addr_t avail_gpa;

	if (!vq || !vq->guest) {
		return -EINVAL;
	}

	idx = vq->last_avail_idx & (vq->desc_count- 1);
	vq->last_avail_idx++;

	avail_gpa = vq->vring.avail_base_gpa + offsetof(struct vring_avail, ring[idx]);
	vm_guest_memory_read(vq->guest, avail_gpa, (void *)&val, sizeof(val));

	return val;
}

bool virtio_queue_available(struct virtio_queue *vq)
{
	uint16_t val;
	physical_addr_t avail_gpa;

	if (!vq || !vq->guest) {
		return false;
	}

	avail_gpa = vq->vring.avail_base_gpa + offsetof(struct vring_avail, idx);
	vm_guest_memory_read(vq->guest, avail_gpa, (void *)&val, sizeof(val));

	return val != vq->last_avail_idx;
}

bool virtio_queue_should_signal(struct virtio_queue *vq)
{
	uint16_t old_idx, new_idx, event_idx;
	physical_addr_t used_gpa, avail_gpa;

	if (!vq || !vq->guest) {
		return false;
	}

	old_idx = vq->last_used_signalled;

	used_gpa = vq->vring.used_base_gpa + offsetof(struct vring_used, idx);
	vm_guest_memory_read(vq->guest, used_gpa, (void *)&new_idx, sizeof(new_idx));

	avail_gpa = vq->vring.avail_base_gpa + offsetof(struct vring_avail, ring[vq->vring.num]);
	vm_guest_memory_read(vq->guest, avail_gpa, (void *)&event_idx, sizeof(event_idx));

	if (vring_need_event(event_idx, new_idx, old_idx)) {
		vq->last_used_signalled = new_idx;
		return true;
	}

	return false;
}

void virtio_queue_set_avail_event(struct virtio_queue *vq)
{
	uint16_t val;
	physical_addr_t avail_evt_pa;

	if (!vq || !vq->guest) {
		return;
	}

	val = vq->last_avail_idx;
	avail_evt_pa = vq->vring.used_base_gpa + offsetof(struct vring_used, ring[vq->vring.num]);
	vm_guest_memory_write(vq->guest, avail_evt_pa, (void *)&val, sizeof(val));
}

void virtio_queue_set_used_elem(struct virtio_queue *vq, uint32_t head, uint32_t len)
{
	uint16_t used_idx;
	struct vring_used_elem used_elem;
	physical_addr_t used_idx_pa, used_elem_pa;

	if (!vq || !vq->guest) {
		return;
	}

	used_idx_pa = vq->vring.used_base_gpa + offsetof(struct vring_used, idx);
	vm_guest_memory_read(vq->guest, used_idx_pa, &used_idx, sizeof(used_idx));

	used_elem.id = head;
	used_elem.len = len;

	used_idx = used_idx & (vq->vring.num - 1);
	used_elem_pa = vq->vring.used_base_gpa + offsetof(struct vring_used, ring[used_idx]);
	vm_guest_memory_write(vq->guest, used_elem_pa, (void *)&used_elem, sizeof(used_elem));

	used_idx++;
	vm_guest_memory_write(vq->guest, used_idx_pa, &used_idx, sizeof(used_idx));
}

bool virtio_queue_setup_done(struct virtio_queue *vq)
{
	return (vq) ? ((vq->guest) ? true : false) : false;
}

bool virtio_queue_cleanup(struct virtio_queue *vq)
{
	if (!vq || !vq->guest) {
		goto done;
	}

	vq->last_avail_idx = 0;
	vq->last_used_signalled = 0;

	vq->guest = NULL;

	vq->desc_count = 0;
	vq->align = 0;
	vq->guest_pfn = 0;
	vq->guest_page_size = 0;

	vq->guest_addr = 0;
	vq->host_addr = 0;
	vq->total_size = 0;

done:
	return true;
}

bool virtio_queue_setup(struct virtio_queue *vq,
			   struct vm *guest,
			   physical_addr_t guest_pfn,
			   physical_size_t guest_page_size,
			   uint32_t desc_count, uint32_t align)
{
	physical_addr_t gphys_addr, hphys_addr;
	physical_size_t gphys_size, avail_size;
	struct vm_mem_partition *vpart;

    vpart = (struct vm_mem_partition *)k_malloc(sizeof(struct vm_mem_partition));
    if (!vpart) {
        return false;
    }

	if(!virtio_queue_cleanup(vq)) {
			return false;
	}

	gphys_addr = guest_pfn * guest_page_size;
	gphys_size = vring_size(desc_count, align);
	hphys_addr = vm_gpa_to_hpa(guest, gphys_addr, vpart);
	avail_size = vpart->part_hpa_size - (hphys_addr - vpart->part_hpa_base);

	if(avail_size < gphys_size) {
		printk("%s: available size less than required size\n",
			   __func__);
		return false;
	}

	vring_init(&vq->vring, desc_count, NULL, gphys_addr, align);

	vq->guest = guest;
	vq->desc_count = desc_count;
	vq->align = align;
	vq->guest_pfn = guest_pfn;
	vq->guest_page_size = guest_page_size;

	vq->guest_addr = gphys_addr;
	vq->host_addr = hphys_addr;
	vq->total_size = gphys_size;

	return true;
}

/*
 * Each buffer in the virtqueues is actually a chain of descriptors.  This
 * function returns the next descriptor in the chain, max descriptor count
 * if we're at the end.
 */
static unsigned next_desc(struct virtio_queue *vq,
			  struct vring_desc *desc,
			  uint32_t i, uint32_t max)
{
	int rc;
	uint32_t next;

	if (!(desc->flags & VRING_DESC_F_NEXT)) {
		return max;
	}

	next = desc->next;

	rc = virtio_queue_get_desc(vq, next, desc);
	if (rc < 0) {
		printk("%s: failed to get descriptor next=%d error=%d\n",
			   __func__, next, rc);
		return max;
	}

	return next;
}

bool virtio_queue_get_head_iovec(struct virtio_queue *vq,
				    uint16_t head, struct virtio_iovec *iov,
				    uint32_t *ret_iov_cnt, uint32_t *ret_total_len,
				    uint16_t *ret_head)
{
	uint16_t idx, max;
	struct vring_desc desc;
	int i, rc;

	if (!vq || !vq->guest || !iov) {
		goto fail;
	}

	if (ret_iov_cnt) {
		*ret_iov_cnt = 0;
	}
	if (ret_total_len) {
		*ret_total_len = 0;
	}
	if (ret_head) {
		*ret_head = 0;
	}

	idx = head;
	max = virtio_queue_max_desc(vq);

	rc = virtio_queue_get_desc(vq, idx, &desc);
	if (rc < 0) {
		printk("%s: failed to get descriptor idx=%d error=%d\n",
			   __func__, idx, rc);
		goto fail;
	}

	if (desc.flags & VRING_DESC_F_INDIRECT) {
		printk("%s: indirect descriptor not supported idx=%d\n",
			   __func__, idx);
		goto fail;
	}

	i = 0;
	do {
		iov[i].addr = desc.addr;
		iov[i].len = desc.len;

		if (ret_total_len) {
			*ret_total_len += desc.len;
		}

		if (desc.flags & VRING_DESC_F_WRITE) {
			iov[i].flags = 1;  /* Write */
		} else {
			iov[i].flags = 0; /* Read */
		}

		i++;
	} while ((idx = next_desc(vq, &desc, idx, max)) != max);

	if (ret_iov_cnt) {
		*ret_iov_cnt = i;
	}

	virtio_queue_set_avail_event(vq);

	if (ret_head) {
		*ret_head = head;
	}

	return true;

fail:
	if (ret_iov_cnt) {
		*ret_iov_cnt = 0;
	}
	if (ret_total_len) {
		*ret_total_len = 0;
	}
	return false;
}

bool virtio_queue_get_iovec(struct virtio_queue *vq,
			       struct virtio_iovec *iov,
			       uint32_t *ret_iov_cnt, uint32_t *ret_total_len,
			       uint16_t *ret_head)
{
	uint16_t head = virtio_queue_pop(vq);

	return virtio_queue_get_head_iovec(vq, head, iov,
					       ret_iov_cnt, ret_total_len,
					       ret_head);
}

uint32_t virtio_iovec_to_buf_read(struct virtio_device *dev,
				 struct virtio_iovec *iov,
				 uint32_t iov_cnt, void *buf,
				 uint32_t buf_len)
{
	uint32_t i = 0, pos = 0, len = 0;

	for (i = 0; i < iov_cnt && pos < buf_len; i++) {
		len = ((buf_len - pos) < iov[i].len) ?
				(buf_len - pos) : iov[i].len;

		if(!len){
			break;
		}

		vm_guest_memory_read(dev->guest, iov[i].addr,
					    buf + pos, len);
		pos += len;
	}

	return pos;
}

uint32_t virtio_buf_to_iovec_write(struct virtio_device *dev,
				  struct virtio_iovec *iov,
				  uint32_t iov_cnt, void *buf,
				  uint32_t buf_len)
{
	uint32_t i = 0, pos = 0, len = 0;

	for (i = 0; i < iov_cnt && pos < buf_len; i++) {
		len = ((buf_len - pos) < iov[i].len) ?
					(buf_len - pos) : iov[i].len;

		if (!len) {
			break;
		}

		vm_guest_memory_write(dev->guest, iov[i].addr,
					     buf + pos, len);

		pos += len;
	}

	return pos;
}

void virtio_iovec_fill_zeros(struct virtio_device *dev,
				 struct virtio_iovec *iov,
				 uint32_t iov_cnt)
{
	uint32_t i = 0, pos = 0, len = 0;
	uint8_t zeros[16];

	memset(zeros, 0, sizeof(zeros));

	while (i < iov_cnt) {
		len = (iov[i].len < 16) ? iov[i].len : 16;

		if (!len) {
			break;
		}

		vm_guest_memory_write(dev->guest, iov[i].addr + pos,
					     zeros, len);

		pos += len;
		if (pos == iov[i].len) {
			pos = 0;
			i++;
		}
	}
}

/* ========== VirtIO device and emulator implementations ========== */

static int __virtio_reset_emulator(struct virtio_device *dev)
{
	if (dev && dev->emu && dev->emu->reset) {
		return dev->emu->reset(dev);
	}

	return -EFAULT;
}

static int __virtio_connect_emulator(struct virtio_device *dev,
				     struct virtio_emulator *emu)
{
	if (dev && emu && emu->connect) {
		return emu->connect(dev, emu);
	}

	return -EFAULT;
}

static void __virtio_disconnect_emulator(struct virtio_device *dev)
{
	if (dev && dev->emu && dev->emu->disconnect) {
		dev->emu->disconnect(dev);
	}
}

static int __virtio_config_read_emulator(struct virtio_device *dev,
					 uint32_t offset, void *dst, uint32_t dst_len)
{
	if (dev && dev->emu && dev->emu->read_config) {
		return dev->emu->read_config(dev, offset, dst, dst_len);
	}

	return -EFAULT;
}

static int __virtio_config_write_emulator(struct virtio_device *dev,
					  uint32_t offset, void *src, uint32_t src_len)
{
	if (dev && dev->emu && dev->emu->write_config) {
		return dev->emu->write_config(dev, offset, src, src_len);
	}

	return -EFAULT;
}

static bool __virtio_match_device(const struct virtio_device_id *ids,
				  struct virtio_device *dev)
{
	while (ids->type) {
		if (ids->type == dev->id.type)
			return true;
		ids++;
	}
	return false;
}

static int __virtio_bind_emulator(struct virtio_device *dev,
				  struct virtio_emulator *emu)
{
	int rc = -EINVAL;
	if (__virtio_match_device(emu->id_table, dev)) {
		dev->emu = emu;
		if ((rc = __virtio_connect_emulator(dev, emu))) {
			dev->emu = NULL;
		}
	}
	return rc;
}

static int __virtio_find_emulator(struct virtio_device *dev)
{
	struct virtio_emulator *emu;
	sys_dnode_t *d_node, *ds_node;

	if (!dev || dev->emu) {
		return -EINVAL;
	}

	 SYS_DLIST_FOR_EACH_NODE_SAFE(&virtio_emus, d_node, ds_node) {
		emu = CONTAINER_OF(d_node, struct virtio_emulator, node);

		if (!__virtio_bind_emulator(dev, emu)) {
			return 0;
		}
	}

	return -EFAULT;
}

static void __virtio_attach_emulator(struct virtio_emulator *emu)
{
	struct virtio_device *dev;
	sys_dnode_t *d_node, *ds_node;

	if (!emu) {
		return;
	}

	SYS_DLIST_FOR_EACH_NODE_SAFE(&virtio_devs, d_node, ds_node) {
		dev = CONTAINER_OF(d_node, struct virtio_device, node);
		if (!dev->emu) {
			__virtio_bind_emulator(dev, emu);
		}
	}
}

int virtio_config_read(struct virtio_device *dev,
			   uint32_t offset, void *dst, uint32_t dst_len)
{
	return __virtio_config_read_emulator(dev, offset, dst, dst_len);
}

int virtio_config_write(struct virtio_device *dev,
			    uint32_t offset, void *src, uint32_t src_len)
{
	return __virtio_config_write_emulator(dev, offset, src, src_len);
}

int virtio_reset(struct virtio_device *dev)
{
	return __virtio_reset_emulator(dev);
}

int virtio_register_device(struct virtio_device *dev)
{
	int rc = 0;
	k_spinlock_key_t key;

	if (!dev || !dev->tra) {
		return -EFAULT;
	}

	sys_dnode_init(&dev->node);
	dev->emu = NULL;
	dev->emu_data = NULL;

	key = k_spin_lock(&virtio_lock);

	sys_dlist_append(&virtio_devs, &dev->node);
	rc = __virtio_find_emulator(dev);

	k_spin_unlock(&virtio_lock, key);

	return rc;
}

void virtio_unregister_device(struct virtio_device *dev)
{
	k_spinlock_key_t key;

	if (!dev) {
		return;
	}

	key = k_spin_lock(&virtio_lock);

	__virtio_disconnect_emulator(dev);
    sys_dlist_remove(&dev->node);

    k_spin_unlock(&virtio_lock, key);
}


int virtio_register_emulator(struct virtio_emulator *emu)
{
	bool found;
	struct virtio_emulator *vemu;
	k_spinlock_key_t key;
	sys_dnode_t *d_node, *ds_node;

	if (!emu) {
		return -EFAULT;
	}

	vemu = NULL;
	found = false;

	key = k_spin_lock(&virtio_lock);

	SYS_DLIST_FOR_EACH_NODE_SAFE(&virtio_emus, d_node, ds_node) {
		vemu = CONTAINER_OF(d_node, struct virtio_emulator, node);

		if (strcmp(vemu->name, emu->name) == 0) {
			found = true;
			break;
		}
	}

	if (found) {
    	k_spin_unlock(&virtio_lock, key);
		return 0;
	}

	sys_dnode_init(&emu->node);
	sys_dlist_append(&virtio_emus, &emu->node);

	__virtio_attach_emulator(emu);

    k_spin_unlock(&virtio_lock, key);

	return 0;
}

void virtio_unregister_emulator(struct virtio_emulator *emu)
{
	struct virtio_device *dev;
	k_spinlock_key_t key;
	sys_dnode_t *d_node, *ds_node;

	key = k_spin_lock(&virtio_lock);

    sys_dlist_remove(&emu->node);

	SYS_DLIST_FOR_EACH_NODE_SAFE(&virtio_devs, d_node, ds_node) {
		dev = CONTAINER_OF(d_node, struct virtio_device, node);
		if (dev->emu == emu) {
			__virtio_disconnect_emulator(dev);
			__virtio_find_emulator(dev);
		}
	}

    k_spin_unlock(&virtio_lock, key);
}
