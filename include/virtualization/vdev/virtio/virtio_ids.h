/**
 * @file virtio_ids.h
 * @brief VirtIO Device IDs
 * The source has been largely adapted from xvisor 0.3.2:
 * core\include\vio\vmm_virtio_ids.h
 */

#ifndef ZEPHYR_INCLUDE_ZVM_VIRTIO_IDS_H_
#define ZEPHYR_INCLUDE_ZVM_VIRTIO_IDS_H_

enum virtio_id {
	VIRTIO_ID_UNKNOWN		=  0, /* Unknown */
	VIRTIO_ID_NET			=  1, /* Network card */
	VIRTIO_ID_BLOCK			=  2, /* Block device */
	VIRTIO_ID_CONSOLE		=  3, /* Console */
	VIRTIO_ID_RNG			=  4, /* Entropy source */
	VIRTIO_ID_BALLOON		=  5, /* Memory ballooning (traditional) */
	VIRTIO_ID_IO_MEMORY		=  6, /* ioMemory */
	VIRTIO_ID_RPMSG			=  7, /* rpmsg (remote processor messaging) */
	VIRTIO_ID_SCSI			=  8, /* SCSI host */
	VIRTIO_ID_9P			=  9, /* 9P transport */
	VIRTIO_ID_MAC_VLAN		= 10, /* mac 802.11 Vlan */
	VIRTIO_ID_RPROC_SERIAL	= 11, /* rproc serial */
	VIRTIO_ID_CAIF			= 12, /* virtio CAIF */
	VIRTIO_ID_BALLOON_NEW	= 13, /* New memory ballooning */
	VIRTIO_ID_GPU			= 16, /* GPU device */
	VIRTIO_ID_TIMER			= 17, /* Timer/Clock device */
	VIRTIO_ID_INPUT			= 18, /* Input device */
};

#define VIRTIO_ID_ANY		0xffffffff

#ifdef offsetof
#undef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

typedef unsigned int irq_flags_t;
typedef unsigned long long virtual_addr_t;
typedef unsigned long virtual_size_t;
typedef unsigned long long physical_addr_t;
typedef unsigned long physical_size_t;

#endif /* ZEPHYR_INCLUDE_ZVM_VIRTIO_IDS_H_ */
