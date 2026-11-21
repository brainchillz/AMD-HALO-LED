/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Backport glue for building the Linux 7.3 amd_halo_led.c, unmodified, on
 * older kernels. Force-included by the Makefile (-include).
 *
 * Linux 7.3's wmidev_invoke_method() takes a sixth argument, the minimum
 * size of the result buffer, and fails the call when the firmware returns
 * less. Older kernels have the five-argument form and leave that check to
 * the caller. The Makefile defines HALO_WMI_INVOKE_NO_MIN_SIZE when the
 * kernel being built against has the five-argument form.
 */
#ifndef AMD_HALO_LED_COMPAT_H
#define AMD_HALO_LED_COMPAT_H

#ifdef HALO_WMI_INVOKE_NO_MIN_SIZE
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/wmi.h>

static inline int halo_compat_wmidev_invoke_method(struct wmi_device *wdev,
						   u8 instance, u32 method_id,
						   const struct wmi_buffer *in,
						   struct wmi_buffer *out,
						   size_t min_size)
{
	int ret;

	ret = wmidev_invoke_method(wdev, instance, method_id, in, out);
	if (ret)
		return ret;

	if (!out->data || out->length < min_size) {
		kfree(out->data);
		out->data = NULL;
		out->length = 0;
		return -ENOMSG;
	}

	return 0;
}

#define wmidev_invoke_method(wdev, instance, method_id, in, out, min_size) \
	halo_compat_wmidev_invoke_method(wdev, instance, method_id, in, out, min_size)
#endif /* HALO_WMI_INVOKE_NO_MIN_SIZE */

#endif /* AMD_HALO_LED_COMPAT_H */
