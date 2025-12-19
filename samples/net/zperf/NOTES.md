# Zperf USB Networking Notes

Tested with Zephyr 4.3.0 on nRF5340DK.

## CDC-ECM + CDC-ACM

### Overlay File

Create `usbd_cdc_ecm_acm.overlay`:

```bash
cat > samples/net/zperf/usbd_cdc_ecm_acm.overlay << 'EOF'
/ {
	chosen {
		zephyr,console = &cdc_acm_uart0;
		zephyr,shell-uart = &cdc_acm_uart0;
	};

	cdc_ecm_eth0: cdc_ecm_eth0 {
		compatible = "zephyr,cdc-ecm-ethernet";
		remote-mac-address = "00005E005301";
	};
};

&zephyr_udc0 {
	cdc_acm_uart0: cdc_acm_uart0 {
		compatible = "zephyr,cdc-acm-uart";
	};
};
EOF
```

### Build

```bash
west build -b nrf5340dk/nrf5340/cpuapp samples/net/zperf -- \
  -DEXTRA_CONF_FILE="overlay-usbd.conf" \
  -DDTC_OVERLAY_FILE="usbd_cdc_ecm_acm.overlay"

west flash
```

## CDC-NCM + CDC-ACM

### Overlay File

Create `usbd_cdc_ncm_acm.overlay`:

```bash
cat > samples/net/zperf/usbd_cdc_ncm_acm.overlay << 'EOF'
/ {
	chosen {
		zephyr,console = &cdc_acm_uart0;
		zephyr,shell-uart = &cdc_acm_uart0;
	};

	cdc_ncm_eth0: cdc_ncm_eth0 {
		compatible = "zephyr,cdc-ncm-ethernet";
		remote-mac-address = "00005E005301";
	};
};

&zephyr_udc0 {
	cdc_acm_uart0: cdc_acm_uart0 {
		compatible = "zephyr,cdc-acm-uart";
	};
};
EOF
```

### Build

```bash
west build -b nrf5340dk/nrf5340/cpuapp samples/net/zperf -- \
  -DEXTRA_CONF_FILE="overlay-usbd.conf" \
  -DDTC_OVERLAY_FILE="usbd_cdc_ncm_acm.overlay"

west flash
```

## Network Configuration

### Device (Zephyr) IP Address

- IPv4: `192.0.2.1`
- IPv6: `2001:db8::1`

### Host (Mac/Linux) IP Address

Configure the USB network interface on the host:

```bash
# Find the new interface
ifconfig -a

# Configure IPv4 (replace <interface> with actual interface name, e.g., en8)
sudo ifconfig <interface> 192.0.2.100 netmask 255.255.255.0 up

# Or for IPv6
sudo ifconfig <interface> inet6 2001:db8::2 prefixlen 64
```

Verify connectivity:

```bash
ping 192.0.2.1
```

## Connecting to the Console

Find the USB serial device:

```bash
ls /dev/tty.usb*
```

Connect with screen (look for a new `tty.usbmodem*` device):

```bash
screen /dev/tty.usbmodem<device> 115200
```

To exit screen: `Ctrl-a` then `k`, then `y`

## Running iperf Tests

### UDP Test (Host to Device)

On the device console:
```
zperf udp download 5001
```

On the host:
```bash
iperf -c 192.0.2.1 -u -b 10M -t 10
```

### TCP Test (Host to Device)

On the device console:
```
zperf tcp download 5001
```

On the host:
```bash
iperf -c 192.0.2.1 -t 10
```

### TCP Test (Device to Host)

On the host:
```bash
iperf -s
```

On the device console:
```
zperf tcp upload 192.0.2.100 5001 10 1K 1M
```

### UDP Test (Device to Host)

On the host:
```bash
iperf -s -u
```

On the device console:
```
zperf udp upload 192.0.2.100 5001 10 1K 1M
```

## Zperf Shell Commands

```
zperf --help                          # Show all commands
zperf tcp download <port>             # Start TCP server
zperf udp download <port>             # Start UDP server
zperf tcp upload <host> <port> <duration_sec> <packet_size> <rate>
zperf udp upload <host> <port> <duration_sec> <packet_size> <rate>
```

## Expected Performance (CDC-ECM, nRF5340DK)

- UDP: ~10 Mbits/sec
- TCP: ~4 Mbits/sec
