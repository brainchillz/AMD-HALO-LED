# AMD-HALO-LED

**Make the RGB light bar on the AMD Ryzen AI Halo box (`RAH-001`,
Ryzen AI Max+ 395) work on stock Ubuntu 26.04.**

## What this is

Two things, both for the Halo box's front LED bar:

1. **A kernel driver package.** The official `amd_halo_led` driver from
   Linux 7.3, backported so it builds on Ubuntu 26.04's 7.0 kernel, and
   packaged as a DKMS `.deb` you can install with `apt`.
2. **A command-line tool, `halo-led`.** Sets the bar's colour and brightness,
   turns it off and on, and can make the colour stick across reboots.

This is not AMD software and is not needed on AMD's own Halo OS, whose kernel
already includes the driver. It is for people who installed a stock Ubuntu
on the box and found the LED bar could no longer be controlled.

## Why it is needed

The `amd_halo_led` driver was merged for Linux 7.3
(`drivers/platform/x86/amd/amd_halo_led.c`). AMD's own Halo OS kernel carries
it, but Ubuntu 26.04 ships Linux 7.0, where the LED device does not exist.
This repository builds the 7.3 driver out of tree for 7.0 to 7.2 kernels.

## Contents

| Path | What it is |
| --- | --- |
| `amd-halo-led-dkms_7.3+halo1_all.deb` | Ready-to-install DKMS package |
| `amd-halo-led-dkms-7.3/` | Source of that package (driver, compat header, Makefile, `debian/`) |
| `halo-led` | CLI to set the LED colour (bash) |
| `halo-led.README.md` | The CLI's own documentation, written for the Halo OS kernel |
| `halo-led.LICENSE` | Licence of the CLI (MIT) |

## Install the driver

```sh
sudo apt install ./amd-halo-led-dkms_7.3+halo1_all.deb
```

DKMS builds the module for every installed 7.0 to 7.2 kernel and rebuilds it
on kernel updates. The module loads by its WMI alias; afterwards the LED is at
`/sys/class/leds/amd_halo:multicolor:status`.

Kernels from 7.3 on include the driver. DKMS skips them, and the package can
then be removed with `sudo apt purge amd-halo-led-dkms`.

## Set the colour

With the CLI (values are 0-100, not 0-255):

```sh
sudo install -m 0755 halo-led /usr/local/bin/halo-led
sudo halo-led green 10        # named colour, brightness 10
sudo halo-led rgb 100 30 0    # explicit R G B
sudo halo-led off
sudo halo-led on
sudo halo-led persist         # reapply the current colour at boot (udev rule)
halo-led status
```

Or directly through sysfs:

```sh
echo "0 0 100" | sudo tee /sys/class/leds/amd_halo:multicolor:status/multi_intensity
echo 50 | sudo tee /sys/class/leds/amd_halo:multicolor:status/brightness
```

On Ubuntu, writing needs root. The CLI's non-root path is the Halo Center
REST API, which only exists on Halo OS.

## What the backport changes

`amd_halo_led.c` is the file from Linux v7.3, byte for byte
(sha256 `8b4e4a8e7de52c3285386aedbd45bd9120ab914243cc79bb3b83ae799cb75b29`).

The one incompatibility is `wmidev_invoke_method()`: 7.3 takes a sixth
argument, the minimum size of the result buffer, while 7.0 has the
five-argument form. `compat.h` is force-included by the Makefile and wraps the
call, doing the size check itself. The Makefile reads the prototype from the
kernel headers being built against and enables the wrapper only when the
five-argument form is found.

Kernel configuration needed: `CONFIG_ACPI_WMI` and
`CONFIG_LEDS_CLASS_MULTICOLOR`.

## Rebuild the package

```sh
sudo apt install dkms build-essential debhelper dh-dkms
cd amd-halo-led-dkms-7.3
dpkg-buildpackage -us -uc -b
```

To build the module alone, without packaging: `make` in that directory.

## Tested

Ubuntu 26.04, kernel 7.0.0-44-generic, AMD Ryzen AI Halo `RAH-001`,
BIOS 03.03. Colours were set and read back from the firmware across module
reloads; off, range checking and the `persist` udev rule were exercised.
Not tested: other kernels in the 7.0 to 7.2 range, Secure Boot enabled
(DKMS signs the module with the machine's MOK key, which must be enrolled).

The CLI's README describes a bug in the older Halo OS 6.18 driver, where
writing brightness 0 corrupts the colour. With the 7.3 driver, brightness 0
turned the LED off in testing. The CLI still avoids writing 0, which is
harmless.

## Licences

- Driver and backport glue (`amd-halo-led-dkms-7.3/`, the `.deb`): GPL-2.0,
  as the Linux kernel. Driver copyright Advanced Micro Devices, Inc.
- `halo-led` CLI: MIT, see `halo-led.LICENSE`.
