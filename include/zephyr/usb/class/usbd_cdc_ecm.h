/*
 * Copyright (c) 2026 Tolt Technologies LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USB CDC-ECM class (device_next) public API
 */

#ifndef ZEPHYR_INCLUDE_USB_CLASS_USBD_CDC_ECM_H_
#define ZEPHYR_INCLUDE_USB_CLASS_USBD_CDC_ECM_H_

#include <stdint.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief How long the bulk IN transfer has been waiting for the host.
 *
 * The host's CDC-ECM driver keeps the bulk IN pipe polled while the interface
 * is up, so a transfer waiting for more than a moment means the host stopped
 * polling it. macOS does when another device on the same hub re-enumerates,
 * and the device sees no bus event. Re-enumerating, e.g. usbd_disable() then
 * usbd_enable(), makes the host rebuild the pipe.
 *
 * @param dev CDC-ECM Ethernet device
 *
 * @return Milliseconds the oldest bulk IN request has been queued, 0 if none
 */
uint32_t usbd_cdc_ecm_tx_pending_ms(const struct device *dev);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_USB_CLASS_USBD_CDC_ECM_H_ */
