/**
 * @file virtio_blk.c
  * @brief VirtIO based block device Emulator.
 */

#include <stdint.h>
#include <zephyr.h>
#include <device.h>
#include <storage/disk_access.h>

#include <virtualization/zvm.h>
#include <virtualization/vdev/virtio/virtio.h>
#include <virtualization/vdev/virtio/virtio_blk.h>

#if IS_ENABLED(CONFIG_DISK_DRIVER_SDMMC)
#define DISK_NAME CONFIG_SDMMC_VOLUME_NAME
#elif IS_ENABLED(CONFIG_DISK_DRIVER_RAM)
#define DISK_NAME CONFIG_DISK_RAM_VOLUME_NAME
#else
#define DISK_NAME NULL
#endif

#define SECTOR_SIZE	512
#define VIRTIO_BLK_QUEUE_SIZE		128
#define VIRTIO_BLK_IO_QUEUE		0
#define VIRTIO_BLK_NUM_QUEUES		1
#define VIRTIO_BLK_SECTOR_SIZE		512
#define VIRTIO_BLK_DISK_SEG_MAX		(VIRTIO_BLK_QUEUE_SIZE - 2)

enum request_type {
	REQUEST_UNKNOWN=0,
	REQUEST_READ=1,
	REQUEST_WRITE=2
};

//request: A request contains multiple iovecs. 
struct virtio_blk_dev_req {
	struct virtio_queue		*vq;
	uint16_t				head;
	struct virtio_iovec		*read_iov;
	uint32_t				read_iov_cnt;
	uint32_t				len;
	struct virtio_iovec		status_iov;		
	void				*data;
	enum request_type type;
};

struct virtio_blk_dev {
	struct virtio_device 	*vdev;

	struct virtio_queue 	vqs[VIRTIO_BLK_NUM_QUEUES];
	struct virtio_iovec		iov[VIRTIO_BLK_QUEUE_SIZE];
	struct virtio_blk_dev_req	reqs[VIRTIO_BLK_QUEUE_SIZE];
	uint64_t 				features;

	struct virtio_blk_config 	config;
	char *disk_pdrv;
};

static uint64_t virtio_blk_get_host_features(struct virtio_device *dev)
{
	return	1UL << VIRTIO_BLK_F_SEG_MAX
		| 1UL << VIRTIO_BLK_F_BLK_SIZE
		| 1UL << VIRTIO_BLK_F_FLUSH
		| 1UL << VIRTIO_RING_F_EVENT_IDX;
}

static void virtio_blk_set_guest_features(struct virtio_device *dev,
					  uint32_t select, uint32_t features)
{
	struct virtio_blk_dev *vbdev = dev->emu_data;

	if (1 < select)
		return;

	vbdev->features &= ~((uint64_t)UINT_MAX << (select * 32));
	vbdev->features |= ((uint64_t)features << (select * 32));
}

static int virtio_blk_init_vq(struct virtio_device *dev,
			      uint32_t vq, uint32_t page_size, uint32_t align,
			      uint32_t pfn)
{
	int rc;
	struct virtio_blk_dev *vbdev = dev->emu_data;

	switch (vq) {
	case VIRTIO_BLK_IO_QUEUE:
		rc = virtio_queue_setup(&vbdev->vqs[vq], dev->guest,
				pfn, page_size, VIRTIO_BLK_QUEUE_SIZE, align);
		break;
	default:
		rc = -EINVAL;
		break;
	};

	return rc;
}

static int virtio_blk_get_pfn_vq(struct virtio_device *dev, uint32_t vq)
{
	int rc;
	struct virtio_blk_dev *vbdev = dev->emu_data;

	switch (vq) {
	case VIRTIO_BLK_IO_QUEUE:
		rc = virtio_queue_guest_pfn(&vbdev->vqs[vq]);
		break;
	default:
		rc = -EINVAL;
		break;
	};

	return rc;
}

static int virtio_blk_get_size_vq(struct virtio_device *dev, uint32_t vq)
{
	int rc;

	switch (vq) {
	case VIRTIO_BLK_IO_QUEUE:
		rc = VIRTIO_BLK_QUEUE_SIZE;
		break;
	default:
		rc = 0;
		break;
	};

	return rc;
}

static int virtio_blk_set_size_vq(struct virtio_device *dev,
				  uint32_t vq, int size)
{
	/* FIXME: dynamic */
	return size;
}
 
static void virtio_blk_req_done(struct virtio_blk_dev *vbdev,
				struct virtio_blk_dev_req *req, uint8_t status)
{
	struct virtio_device *dev = vbdev->vdev;
	int queueid = req->vq - vbdev->vqs;

	if (req->read_iov && req->len && req->data &&
	    (status == VIRTIO_BLK_S_OK) &&
	    (req->type == REQUEST_READ)) {
		virtio_buf_to_iovec_write(dev,
					  req->read_iov,
					  req->read_iov_cnt,
					  req->data,
					  req->len);
	}

	if (req->read_iov) {
		k_free(req->read_iov);
		req->read_iov = NULL;
		req->read_iov_cnt = 0;
	}

	req->type = REQUEST_UNKNOWN;
	if (req->data) {
		k_free(req->data);
		req->data = NULL;
	}

	virtio_buf_to_iovec_write(dev, &req->status_iov, 1, &status, 1);

	virtio_queue_set_used_elem(req->vq, req->head, req->len);

	if (virtio_queue_should_signal(req->vq)) {
		dev->tra->notify(dev, queueid);
	}
}

static void virtio_blk_do_io(struct virtio_device *dev,
			     struct virtio_blk_dev *vbdev)
{
	int rc;
	uint16_t head, thead;
	uint32_t i, iov_cnt, len, num_sectors, cmd_buf;
	struct virtio_blk_dev_req *req;
	struct virtio_queue *vq = &vbdev->vqs[VIRTIO_BLK_IO_QUEUE];
	struct virtio_blk_outhdr hdr;

	while (virtio_queue_available(vq)) {
		thead = virtio_queue_pop(vq);
		req = &vbdev->reqs[thead];
		rc = virtio_queue_get_head_iovec(vq, thead, vbdev->iov,
						     &iov_cnt, &len, &head);
		if (!rc) {
			printk("%s: failed to get iovec (error %d)\n",
				   __func__, rc);
			continue;
		}

		req->vq = vq;
		req->head = head;
		req->read_iov = NULL;
		req->read_iov_cnt = 0;
		req->len = 0;
		for (i = 1; i < (iov_cnt - 1); i++) {
			req->len += vbdev->iov[i].len;
		}
		req->status_iov.addr = vbdev->iov[iov_cnt - 1].addr;
		req->status_iov.len = vbdev->iov[iov_cnt - 1].len;
		req->type = REQUEST_UNKNOWN;

		len = virtio_iovec_to_buf_read(dev, &vbdev->iov[0], 1,
						   &hdr, sizeof(hdr));
		if (len < sizeof(hdr)) {
			virtio_queue_set_used_elem(req->vq, req->head, 0);
			continue;
		}

		switch (hdr.type) {
		case VIRTIO_BLK_T_IN:
			req->type = REQUEST_READ;
			req->data = k_malloc(req->len);
			if (!req->data) {
				virtio_blk_req_done(vbdev, req,
						    VIRTIO_BLK_S_IOERR);
				continue;
			}
			len = sizeof(struct virtio_iovec) * (iov_cnt - 2);
			req->read_iov = k_malloc(len);
			if (!req->read_iov) {
				virtio_blk_req_done(vbdev, req,
						    VIRTIO_BLK_S_IOERR);
				continue;
			}
			req->read_iov_cnt = iov_cnt - 2;
			for (i = 0; i < req->read_iov_cnt; i++) {
				req->read_iov[i].addr = vbdev->iov[i + 1].addr;
				req->read_iov[i].len = vbdev->iov[i + 1].len;
			}
			num_sectors = (req->len + SECTOR_SIZE - 1) / SECTOR_SIZE;
			if(disk_access_read(vbdev->disk_pdrv, req->data, hdr.sector, num_sectors)){
				virtio_blk_req_done(vbdev, req,
						    VIRTIO_BLK_S_IOERR);
			} else {
				//printk("virtio_blk read: start_sector = %lld, num = %d\n", hdr.sector, num_sectors);
				virtio_blk_req_done(vbdev, req,
						    VIRTIO_BLK_S_OK);
			}
			break;
		case VIRTIO_BLK_T_OUT:
			req->type = REQUEST_WRITE;
			req->data = k_malloc(req->len);
			if (!req->data) {
				virtio_blk_req_done(vbdev, req,
						    VIRTIO_BLK_S_IOERR);
				continue;
			} else {
				virtio_iovec_to_buf_read(dev,
							 &vbdev->iov[1],
							 iov_cnt - 2,
							 req->data,
							 req->len);
			}
			num_sectors = (req->len + SECTOR_SIZE - 1) / SECTOR_SIZE;
			if(disk_access_write(vbdev->disk_pdrv, req->data, hdr.sector, num_sectors)){
				virtio_blk_req_done(vbdev, req,
						    VIRTIO_BLK_S_IOERR);
			} else {
				//printk("virtio_blk write: start_sector = %lld, num = %d\n", hdr.sector, num_sectors);
				virtio_blk_req_done(vbdev, req,
						    VIRTIO_BLK_S_OK);
			}
			break;
		case VIRTIO_BLK_T_FLUSH:
			req->type = REQUEST_WRITE;
			if (disk_access_ioctl(vbdev->disk_pdrv, DISK_IOCTL_CTRL_SYNC, &cmd_buf)) {
				virtio_blk_req_done(vbdev, req,
						    VIRTIO_BLK_S_IOERR);
			} else {
				virtio_blk_req_done(vbdev, req,
						    VIRTIO_BLK_S_OK);
			}
			break;
		case VIRTIO_BLK_T_GET_ID:
			req->type = REQUEST_READ;
			req->len = VIRTIO_BLK_ID_BYTES;
			req->data = k_malloc(req->len);
			if (!req->data) {
				virtio_blk_req_done(vbdev, req,
						    VIRTIO_BLK_S_IOERR);
				continue; 
			}
			req->read_iov = k_malloc(sizeof(struct virtio_iovec));
			if (!req->read_iov) {
				virtio_blk_req_done(vbdev, req,
						    VIRTIO_BLK_S_IOERR);
				continue;
			}
			req->read_iov_cnt = 1;
			req->read_iov[0].addr = vbdev->iov[1].addr;
			req->read_iov[0].len = vbdev->iov[1].len;
			if (!vbdev->disk_pdrv) {
				virtio_blk_req_done(vbdev, req, VIRTIO_BLK_S_IOERR);
			} else {
				strncpy(req->data, vbdev->disk_pdrv, req->len);
				virtio_blk_req_done(vbdev, req, VIRTIO_BLK_S_OK);
			}
			break;
		default:
			printk("%s: unhandled hdr.type=%d\n",
				   __func__, hdr.type);
			break;
		};
	}
}

static int virtio_blk_notify_vq(struct virtio_device *dev, uint32_t vq)
{
	int rc = 0;
	struct virtio_blk_dev *vbdev = dev->emu_data;

	switch (vq) {
	case VIRTIO_BLK_IO_QUEUE:
		virtio_blk_do_io(dev, vbdev);
		break;
	default:
		rc = -EINVAL;
		break;
	};

	return rc;
}

static void virtio_blk_status_changed(struct virtio_device *dev,
				      uint32_t new_status)
{
	/* Nothing to do here. */
}

static int virtio_blk_read_config(struct virtio_device *dev,
				  uint32_t offset, void *dst, uint32_t dst_len)
{
	uint32_t i;
	struct virtio_blk_dev *vbdev = dev->emu_data;
	uint8_t *src = (uint8_t *)&vbdev->config;

	for (i=0; (i<dst_len) && ((offset+i) < sizeof(vbdev->config)); i++) {
		((uint8_t *)dst)[i] = src[offset + i];
	}

	return 0;
}

static int virtio_blk_write_config(struct virtio_device *dev,
				   uint32_t offset, void *src, uint32_t src_len)
{
	uint32_t i;
	struct virtio_blk_dev *vbdev = dev->emu_data;
	uint8_t *dst = (uint8_t *)&vbdev->config;

	for (i=0; (i<src_len) && ((offset+i) < sizeof(vbdev->config)); i++) {
		dst[offset + i] = ((uint8_t *)src)[i];
	}

	return 0;
}

static int virtio_blk_reset(struct virtio_device *dev)
{
	int i, rc;
	struct virtio_blk_dev_req *req;
	struct virtio_blk_dev *vbdev = dev->emu_data;

	for (i = 0; i < VIRTIO_BLK_QUEUE_SIZE; i++) {
		req = &vbdev->reqs[i];
		memset(req, 0, sizeof(*req));
		req->type = REQUEST_UNKNOWN;
	}

	rc = virtio_queue_cleanup(&vbdev->vqs[VIRTIO_BLK_IO_QUEUE]);
	if (!rc) {
		return -EFAULT;
	}

	return 0;
}

static int virtio_blk_connect(struct virtio_device *dev,
			      struct virtio_emulator *emu)
{	
	int rc;
	uint32_t cmd_buf;
	struct virtio_blk_dev *vbdev;

	vbdev = k_malloc(sizeof(struct virtio_blk_dev));
	if (!vbdev) {
		printk("Failed to allocate virtio block device....\n");
		return -ENOMEM;
	}
	vbdev->vdev = dev;

	vbdev->config.capacity = 384;
	vbdev->config.seg_max = VIRTIO_BLK_DISK_SEG_MAX,
	vbdev->config.blk_size = VIRTIO_BLK_SECTOR_SIZE;

	vbdev->disk_pdrv = DISK_NAME;

	if (!DISK_NAME) {
		k_free(vbdev);
		printk("No disk device defined, is your board supported?\n");
		return -EFAULT;
	}

	rc = disk_access_init(vbdev->disk_pdrv);
	if (rc) {
		k_free(vbdev);
		printk("Disk access initialization failed\n");
		return -EFAULT;
	}

	rc = disk_access_status(vbdev->disk_pdrv);
	if (rc != DISK_STATUS_OK) {
		k_free(vbdev);
		printk("Disk status is not OK\n");
		return -EFAULT;
	}

	rc = disk_access_ioctl(vbdev->disk_pdrv, DISK_IOCTL_GET_SECTOR_COUNT, &cmd_buf);
	if (rc) {
		k_free(vbdev);
		printk("Disk ioctl get sector count failed\n");
		return -EFAULT;
	}
	//printk("Disk reports %u sectors\n", cmd_buf);

	rc = disk_access_ioctl(vbdev->disk_pdrv, DISK_IOCTL_GET_SECTOR_SIZE, &cmd_buf);
	if (rc) {
		k_free(vbdev);
		printk("Disk ioctl get sector size failed\n");
		return -EFAULT;
	}
	//printk("Disk reports sector size %u\n", cmd_buf);

	dev->emu_data = vbdev;

	return 0;
}

static void virtio_blk_disconnect(struct virtio_device *dev)
{
	struct virtio_blk_dev *vbdev = dev->emu_data;
	vbdev->disk_pdrv = NULL;
	k_free(vbdev);
}

struct virtio_device_id virtio_blk_emu_id[] = {
	{ .type = VIRTIO_ID_BLOCK },
	{ },
};

struct virtio_emulator virtio_blk = {
	.name = "virtio_blk",
	.id_table = virtio_blk_emu_id,

	/* VirtIO operations */
	.get_host_features      = virtio_blk_get_host_features,
	.set_guest_features     = virtio_blk_set_guest_features,
	.init_vq                = virtio_blk_init_vq,
	.get_pfn_vq             = virtio_blk_get_pfn_vq,
	.get_size_vq            = virtio_blk_get_size_vq,
	.set_size_vq            = virtio_blk_set_size_vq,
	.notify_vq              = virtio_blk_notify_vq,
	.status_changed         = virtio_blk_status_changed,

	/* Emulator operations */
	.read_config = virtio_blk_read_config,
	.write_config = virtio_blk_write_config,
	.reset = virtio_blk_reset,
	.connect = virtio_blk_connect,
	.disconnect = virtio_blk_disconnect,
};
