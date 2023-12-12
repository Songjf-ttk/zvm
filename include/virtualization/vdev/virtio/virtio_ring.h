/**
 * Copyright (c) 2013 Pranav Sawargaonkar.
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * @file virtio_ring.h
 * @brief VirtIO Ring Interface
 *
 * The source has been largely adapted from Linux 4.19 or higher:
 * include/uapi/linux/virtio_ring.h
 *
 * Copyright Rusty Russell IBM Corporation 2007.
 *
 * The original code is licensed under the BSD.
 */

#ifndef ZEPHYR_INCLUDE_ZVM_VIRTIO_RING_H_
#define ZEPHYR_INCLUDE_ZVM_VIRTIO_RING_H_

#include <stdint.h>
#include <stddef.h>
#include <virtualization/vdev/virtio/virtio_ids.h>

/* This marks a buffer as continuing via the next field. */
#define VRING_DESC_F_NEXT		1
/* This marks a buffer as write-only (otherwise read-only). */
#define VRING_DESC_F_WRITE		2
/* This means the buffer contains a list of buffer descriptors. */
#define VRING_DESC_F_INDIRECT	4

/* The Host uses this in used->flags to advise the Guest: don't kick me when
 * you add a buffer.  It's unreliable, so it's simply an optimization.  Guest
 * will still kick if it's out of buffers. */
#define VRING_USED_F_NO_NOTIFY	1
/* The Guest uses this in avail->flags to advise the Host: don't interrupt me
 * when you consume a buffer.  It's unreliable, so it's simply an
 * optimization.  */
#define VRING_AVAIL_F_NO_INTERRUPT	1

/* We support indirect buffer descriptors */
#define VIRTIO_RING_F_INDIRECT_DESC	28

/* The Guest publishes the used index for which it expects an interrupt
 * at the end of the avail ring. Host should ignore the avail->flags field.
 */
 /* The Host publishes the avail index for which it expects a kick
  * at the end of the used ring. Guest should ignore the used->flags field.
  */
#define VIRTIO_RING_F_EVENT_IDX	29

/* Virtio ring descriptors: 16 bytes.  These can chain together via "next". */
struct vring_desc {
	/* Address (guest-physical). */
	uint64_t addr;
	/* Length. */
	uint32_t len;
	/* The flags as indicated above. */
	uint16_t flags;
	/* We chain unused descriptors via this, too */
	uint16_t next;
};

struct vring_avail {
	uint16_t flags;
	uint16_t idx;
	uint16_t ring[];
};

/* uint32_t is used here for ids for padding reasons. */
struct vring_used_elem {
	/* Index of start of used descriptor chain. */
	uint32_t id;
	/* Total length of the descriptor chain which was used (written to) */
	uint32_t len;
};

struct vring_used {
	uint16_t flags;
	uint16_t idx;
	struct vring_used_elem ring[];
};

struct vring {
	unsigned int num;

	struct vring_desc *desc;
	physical_addr_t desc_base_gpa;		//physical address in guest

	struct vring_avail *avail;
	physical_addr_t avail_base_gpa;

	struct vring_used *used;
	physical_addr_t used_base_gpa;
};

/* Alignment requirements for vring elements.
 * When using pre-virtio 1.0 layout, these fall out naturally.
 */
#define VRING_DESC_ALIGN_SIZE 16
#define VRING_AVAIL_ALIGN_SIZE 2
#define VRING_USED_ALIGN_SIZE 4

/* The standard layout for the ring is a continuous chunk of memory which looks
 * like this.  We assume num is a power of 2.
 *
 * struct vring
 * {
 *	// The actual descriptors (16 bytes each)
 *	struct vring_desc desc[num];
 *
 *	// A ring of available descriptor heads with free-running index.
 *	__virtio16 avail_flags;
 *	__virtio16 avail_idx;
 *	__virtio16 available[num];
 *	__virtio16 used_event_idx;
 *
 *	// Padding to the next align boundary.
 *	char pad[];
 *
 *	// A ring of used descriptor heads with free-running index.
 *	__virtio16 used_flags;
 *	__virtio16 used_idx;
 *	struct vring_used_elem used[num];
 *	__virtio16 avail_event_idx;
 * };
 */

/* We publish the used event index at the end of the available ring, and vice
 * versa. They are at the end for backwards compatibility. */
#define vring_used_event(vr) ((vr)->avail->ring[(vr)->num])
#define vring_avail_event(vr) (*(uint16_t *)&(vr)->used->ring[(vr)->num])

static inline void vring_init(struct vring *vr,
				  unsigned int num,
				  void *base,
				  physical_addr_t base_pa,	//gpa
				  unsigned long align)
{
	vr->num = num;

	vr->desc = base;
	vr->desc_base_gpa = base_pa;

	vr->avail = (struct vring_avail *)((struct vring_desc *)base + num * sizeof(struct vring_desc));
	vr->avail_base_gpa = base_pa + num * sizeof(struct vring_desc);

	vr->used = (void *)(((unsigned long)&vr->avail->ring[num] + sizeof(uint16_t)
		+ align - 1) & ~(align - 1));

	vr->used_base_gpa = vr->avail_base_gpa +
		      offsetof(struct vring_avail, ring[num + 1]);
	vr->used_base_gpa = (vr->used_base_gpa + align - 1) & ~(align - 1);
}

static inline unsigned vring_size(unsigned int num, unsigned long align)
{
	return ((sizeof(struct vring_desc) * num +
		 sizeof(uint16_t) * (3 + num) + align - 1) & ~(align - 1))
		+ sizeof(uint16_t) * 3 + sizeof(struct vring_used_elem) * num;
}


static inline int vring_need_event(uint16_t event_idx, uint16_t new_idx, uint16_t old)
{
	return (uint16_t)(new_idx - event_idx - 1) < (uint16_t)(new_idx - old);
}

#endif /* __VIRTIO_RING_H__ */
