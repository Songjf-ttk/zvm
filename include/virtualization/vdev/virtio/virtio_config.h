/**
 * @file virtio_config.h
 * @brief VirtIO device feature and status defines
 * The source has been largely adapted from Linux 4.19 or higher:
 * include/uapi/linux/virtio_config.h.
 */

#ifndef ZEPHYR_INCLUDE_ZVM_VIRTIO_CONFIG_H_
#define ZEPHYR_INCLUDE_ZVM_VIRTIO_CONFIG_H_

/* Status byte for guest to report progress, and synchronize features. */
/* We have seen device and processed generic fields (VIRTIO_CONFIG_F_VIRTIO) */
#define VIRTIO_CONFIG_S_ACKNOWLEDGE		1
/* We have found a driver for the device. */
#define VIRTIO_CONFIG_S_DRIVER		2
/* Driver has used its parts of the config, and is happy */
#define VIRTIO_CONFIG_S_DRIVER_OK		4
/* Driver has finished configuring features */
#define VIRTIO_CONFIG_S_FEATURES_OK		8
/* Device entered invalid state, driver must reset it */
#define VIRTIO_CONFIG_S_NEEDS_RESET		0x40
/* We've given up on this device. */
#define VIRTIO_CONFIG_S_FAILED		0x80

/* Some virtio feature bits (currently bits 28 through 32) are reserved
 * for the transport being used (eg. virtio_ring), the rest are per-device
 * feature bits.
 */
#define VIRTIO_TRANSPORT_F_START		28
#define VIRTIO_TRANSPORT_F_END		38

#ifndef VIRTIO_CONFIG_NO_LEGACY
/* Do we get callbacks when the ring is completely used, even if we've
 * suppressed them? */
#define VIRTIO_F_NOTIFY_ON_EMPTY		24

/* Can the device handle any descriptor layout? */
#define VIRTIO_F_ANY_LAYOUT			27
#endif /* VIRTIO_CONFIG_NO_LEGACY */

/* v1.0 compliant. */
#define VIRTIO_F_VERSION_1			32

/*
 * If clear - device has the IOMMU bypass quirk feature.
 * If set - use platform tools to detect the IOMMU.
 *
 * Note the reverse polarity (compared to most other features),
 * this is for compatibility with legacy systems.
 */
#define VIRTIO_F_IOMMU_PLATFORM		33

/*
 * Does the device support Single Root I/O Virtualization?
 */
#define VIRTIO_F_SR_IOV			37

#endif /* __VIRTIO_CONFIG_H__ */
