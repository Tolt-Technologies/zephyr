# CDC-NCM Changes for macOS Compatibility



# Problem

macOS has ~50% success rate bringing up CDC-NCM interfaces. The USB device is always successfully enumerated. The observed behavior is that the USB host selects the CDC Data alternate interface 1 (as expected) but then immediately (10-20ms later) reselects alternate interface 0, disabling the connection.





# Hypothesis 1

MacOS is expecting to see NCM notifications (speed change, connection state) **before** it enables alternate interface 1. This idea is inspired by the observation that successful CDC-NCM implementations (full-speed Linux gadget) send these notifications before and after alternate interface 1 is enabled by the host. Note however that the specification is clear - these notifications are to be sent after alternate interface 1 is enabled.

## Changes Made

**File:** `zephyr/subsys/usb/device_next/class/usbd_cdc_ncm.c`

1. **Added `CDC_NCM_CONFIGURED` flag** (line 40)
   - Tracks when Set Configuration has completed (before alt 1 is selected)

2. **Modified `cdc_ncm_send_notification()` gate** (line 681)
   - Changed check from `CDC_NCM_DATA_IFACE_ENABLED` to `CDC_NCM_CONFIGURED`
   - Allows notifications to be sent as soon as device is configured

3. **Added `connected` parameter to `ncm_send_notification_sequence()`** (line 772)
   - Sends "disconnected" status before alt 1 (accurate state)
   - Sends "connected" status after alt 1

4. **Modified `send_notification_work()`** (lines 819-830)
   - Three-way logic:
     - If interface up AND data enabled → send connected=true
     - Else if configured → send connected=false
     - Else → do nothing

5. **Modified `usbd_cdc_ncm_enable()`** (lines 871-873)
   - Sets `CDC_NCM_CONFIGURED` flag
   - Starts notification sequence immediately with `K_NO_WAIT`

6. **Modified `usbd_cdc_ncm_disable()`** (line 883)
   - Clears `CDC_NCM_CONFIGURED` flag

## Behavior Change
- **Before:** Notifications sent only after alt 1 selected
- **After:** Notifications (speed change + disconnected) sent immediately after Set Configuration, then (speed change + connected) after alt 1

This mimics Linux's behavior, which sends notifications at both points in the enumeration sequence.

## Result

Unfortunately, the results are the same; macOS continues to revert to alternate interface setting 0 right after selecting alternate interface 1, roughly 50% of the time.

**zephyr-dual-notification-fail**

| Index | Level | Sp   | m:s.ms.us    | Dur            | Len  | Err  | Dev  | Ep   | Record                  | Summary                    |
| ----- | ----- | ---- | ------------ | -------------- | ---- | ---- | ---- | ---- | ----------------------- | -------------------------- |
| 36    | 0     | FS   | 0:05.921.688 | 24.833 us      | 0 B  |      | 00   | 00   | Set Address             | Address=06                 |
| 326   | 0     | FS   | 0:05.937.270 | 159.333 us     | 28 B |      | 06   | 00   | Get NTB Parameters      |                            |
| 346   | 0     | FS   | 0:05.992.659 | 18.250 us      | 16 B |      | 06   | 01   | Connection Speed Change | Up=12000000 Down=12000000  |
| 352   | 0     | FS   | 0:06.000.660 | 12.916 us      | 8 B  |      | 06   | 01   | Network Connection      | Disconnected               |
| 378   | 0     | FS   | 0:06.217.464 | 139.416 us     | 0 B  |      | 06   | 00   | Set Interface           | Interface=1 Alt. Setting=1 |
| 389   | 0     | FS   | 0:06.222.695 | 155.250 us     | 0 B  |      | 06   | 00   | Set Interface           | Interface=1 Alt. Setting=0 |
| 400   | 0     | FS   | 0:06.008.661 | 216.045.166 ms | 16 B |      | 06   | 01   | Connection Speed Change | Up=12000000 Down=12000000  |
| 407   | 0     | FS   | 0:06.232.689 | 12.916 us      | 8 B  |      | 06   | 01   | Network Connection      | Disconnected               |



# Hypothesis 2

Once macOS enables alternate interface setting 1, it must receive a connection speed notification within a narrow time window or it will revert to alternate interface setting 0, disabling the connection. Note however that the time that alternate interface settings 1 is held up varies, typically 10-20ms, but can be as short as **~5ms**, as in the capture above.

The following capture is taken from a successful connection of a USB gadget. Note index **406**, the connection speed change notification received after alternate interface setting 1 is enabled. This packet has a timestamp _before_ the previous (Set Interface) packet, however the timestamp indicates the time the USB transaction was *initiated*. Adding the duration gives use the time that it was actually completed: **1:16.125.284**. This is **~160ms** after the alternate interface setting 1 was enabled (index **343**).

In the failed capture above, the connection speed change notification arrived much quicker, at 0:06.224.706, only **7ms** after the alternate interface setting 1 was enabled. Nevertheless the connection failed.

This hypothesis is not supported by this evidence.

**gadget-success**

| Index   | Level | Sp     | m:s.ms.us        | Dur                | Len      | Err  | Dev    | Ep     | Record                      | Summary                        |
| ------- | ----- | ------ | ---------------- | ------------------ | -------- | ---- | ------ | ------ | --------------------------- | ------------------------------ |
| 27      | 0     | FS     | 1:15.825.917     | 128.833 us         | 0 B      |      | 00     | 00     | Set Address                 | Address=03                     |
| 281     | 0     | FS     | 1:15.845.815     | 79.250 us          | 28 B     |      | 03     | 00     | Get NTB Parameters          |                                |
| 296     | 0     | FS     | 1:15.933.243     | 18.166 us          | 16 B     |      | 03     | 02     | Connection Speed Change     | Up=9728000 Down=9728000        |
| 302     | 0     | FS     | 1:15.965.247     | 12.833 us          | 8 B      |      | 03     | 02     | Network Connection          | Disconnected                   |
| **343** | **0** | **FS** | **1:16.110.485** | **80.250 us**      | **0 B**  |      | **03** | **00** | **Set Interface**           | **Interface=1 Alt. Setting=1** |
| **406** | **0** | **FS** | **1:15.997.251** | **128.033.916 ms** | **16 B** |      | **03** | **02** | **Connection Speed Change** | **Up=9728000 Down=9728000**    |
| 413     | 0     | FS     | 1:16.157.271     | 18.166 us          | 16 B     |      | 03     | 02     | Connection Speed Change     | Up=9728000 Down=9728000        |
| 419     | 0     | FS     | 1:16.189.275     | 12.833 us          | 8 B      |      | 03     | 02     | Network Connection          | Disconnected                   |
| 425     | 0     | FS     | 1:16.221.279     | 192.041.583 ms     | 16 B     |      | 03     | 02     | Connection Speed Change     | Up=9728000 Down=9728000        |
| 432     | 0     | FS     | 1:16.445.306     | 12.833 us          | 8 B      |      | 03     | 02     | Network Connection          | Connected                      |
