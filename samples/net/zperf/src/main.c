/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Zperf sample.
 */
#include <zephyr/usb/usbd.h>
#include <zephyr/net/net_config.h>
#include <zephyr/net/zperf.h>

LOG_MODULE_REGISTER(zperf, CONFIG_NET_ZPERF_LOG_LEVEL);

#ifdef CONFIG_NET_LOOPBACK_SIMULATE_PACKET_DROP
#include <zephyr/net/loopback.h>
#endif

#if defined(CONFIG_USB_DEVICE_STACK_NEXT)
#include <sample_usbd.h>
#endif

#define ZPERF_UDP_PORT 5001

static void udp_session_cb(enum zperf_status status,
			   struct zperf_results *result,
			   void *user_data)
{
	if (status == ZPERF_SESSION_FINISHED) {
		LOG_INF("UDP session finished: %llu bytes in %llu us",
			result->total_len, result->time_in_us);
	}
}

int main(void)
{
#if defined(CONFIG_USB_DEVICE_STACK_NEXT)
	struct usbd_context *sample_usbd;
	int err;

	sample_usbd = sample_usbd_init_device(NULL);
	if (sample_usbd == NULL) {
		return -ENODEV;
	}

	err = usbd_enable(sample_usbd);
	if (err) {
		return err;
	}

	(void)net_config_init_app(NULL, "Initializing network");
#endif /* CONFIG_USB_DEVICE_STACK_NEXT */

#ifdef CONFIG_NET_LOOPBACK_SIMULATE_PACKET_DROP
	loopback_set_packet_drop_ratio(1);
#endif

#if defined(CONFIG_NET_DHCPV4) && !defined(CONFIG_NET_CONFIG_SETTINGS)
	net_dhcpv4_start(net_if_get_default());
#endif

	/* Auto-start UDP server for stress testing */
	struct zperf_download_params param = {
		.port = ZPERF_UDP_PORT,
	};

	int ret = zperf_udp_download(&param, udp_session_cb, NULL);
	if (ret < 0) {
		LOG_ERR("Failed to start UDP server: %d", ret);
	} else {
		LOG_INF("UDP server auto-started on port %d", ZPERF_UDP_PORT);
	}

	return 0;
}
