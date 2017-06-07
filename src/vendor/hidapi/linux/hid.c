/*******************************************************
 HIDAPI - Multi-Platform library for
 communication with HID devices.

 Alan Ott
 Signal 11 Software

 8/22/2009
 Linux Version - 6/2/2009

 Copyright 2009, All Rights Reserved.

 At the discretion of the user of this library,
 this software may be licensed under the terms of the
 GNU General Public License v3, a BSD-Style license, or the
 original HIDAPI license as outlined in the LICENSE.txt,
 LICENSE-gpl3.txt, LICENSE-bsd.txt, and LICENSE-orig.txt
 files located at the root of the source distribution.
 These files may also be found in the public source
 code repository located at:
        http://github.com/signal11/hidapi .
********************************************************/

/* C */
/* For asnprintf */
#define _GNU_SOURCE
#include <stdio.h>
#undef _GNU_SOURCE
#include <string.h>
#include <stdlib.h>
#include <locale.h>
#include <errno.h>

/* Unix */
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/utsname.h>
#include <fcntl.h>
#include <poll.h>

/* Linux */
#include <linux/hidraw.h>
#include <linux/version.h>
#include <linux/input.h>
#include <libudev.h>

#include "hidapi.h"

struct hid_device_ {
	int device_handle;
};

static hid_device *new_hid_device(void)
{
	hid_device *dev = calloc(1, sizeof(hid_device));
	dev->device_handle = -1;

	return dev;
}

static int get_device_string(hid_device *dev, const char *key_str, wchar_t *string, size_t maxlen)
{
	char *usb_device_sysfs_filename;
	int usb_device_sysfs_fd = 0;
	char *usb_device_sysfs_buf;
	struct stat s;
	int ret = -1;

	/* Get the dev_t (major/minor numbers) from the file handle. */
	ret = fstat(dev->device_handle, &s);
	if (-1 == ret)
		return ret;

	/* Assume that we get the hidraw node, the device/ link is the hid bus node, its parent is the USB interface,
	   and its parent is the USB device. This isn't the most resilient against future sysfs ABI changes, but
	   *) based on poking around the kernel source, changes here are unlikely
	   *) this is much simpler than traversing up and trying to find which node (if any) is the USB device */
	ret = asprintf(&usb_device_sysfs_filename, "/sys/dev/char/%d:%d/device/../../%s",
		major(s.st_rdev), minor(s.st_rdev), key_str);
	if (-1 == ret) {
		goto end;
	}
	usb_device_sysfs_fd = open(usb_device_sysfs_filename, O_RDONLY);
	if (usb_device_sysfs_fd == -1) {
		ret = -1;
		goto end;
	}

	// Read the contents
	ret = fstat(usb_device_sysfs_fd, &s);
	if (-1 == ret)
		goto end;
	usb_device_sysfs_buf = malloc(s.st_size + 1);
	if (!usb_device_sysfs_buf) {
		ret = -1;
		goto end;
	}
	size_t sysfs_size = read(usb_device_sysfs_fd, usb_device_sysfs_buf, s.st_size);
	if ((size_t)-1 == sysfs_size) {
		ret = -1;
		goto end;
	}
	// Need to add a NULL termination
	usb_device_sysfs_buf[sysfs_size] = 0;

	// Deliberately remove a potential \n at the end
	if (usb_device_sysfs_buf[sysfs_size - 1] == '\n')
		usb_device_sysfs_buf[sysfs_size - 1] = 0;

	/* Convert the string from UTF-8 to wchar_t */
	size_t retm;
	retm = mbstowcs(string, usb_device_sysfs_buf, maxlen);
	ret = (retm == (size_t)-1)? -1: 0;

end:
	free(usb_device_sysfs_buf);
	free(usb_device_sysfs_filename);
	if (usb_device_sysfs_fd != -1) {
		close(usb_device_sysfs_fd);
	}

	return ret;
}

int HID_API_EXPORT hid_init(void)
{
	const char *locale;

	/* Set the locale if it's not set. */
	locale = setlocale(LC_CTYPE, NULL);
	if (!locale)
		setlocale(LC_CTYPE, "");

	return 0;
}

int HID_API_EXPORT hid_exit(void)
{
	/* Nothing to do for this in the Linux/hidraw implementation. */
	return 0;
}

// Result needs to be free()d
static char *uevent_find_line(char *haystack, const char *needle) {
	char *saveptr = NULL;
	char *line;
	char *key;
	char *value;

	line = strtok_r(haystack, "\n", &saveptr);
	while (line != NULL) {
		/* line: "KEY=value" */
		key = line;
		value = strchr(line, '=');
		if (!value) {
			goto next_line;
		}
		*value = '\0';
		value++;

		if (strcmp(key, needle) == 0) {
			return strdup(value);
		}

next_line:
		line = strtok_r(NULL, "\n", &saveptr);
	}

	return NULL;
}

struct hid_device_info  HID_API_EXPORT *hid_enumerate(unsigned short vendor_id, unsigned short product_id)
{
	struct udev *udev;
	struct udev_enumerate *enumerate;
	struct udev_list_entry *devices, *dev_list_entry;

	struct hid_device_info *root = NULL; /* return object */
	struct hid_device_info *cur_dev = NULL;

	hid_init();

	/* Create the udev object */
	udev = udev_new();
	if (!udev) {
		printf("Can't create udev\n");
		return NULL;
	}

	/* Create a list of the devices in the 'hidraw' subsystem. */
	enumerate = udev_enumerate_new(udev);
	udev_enumerate_add_match_subsystem(enumerate, "hidraw");
	udev_enumerate_scan_devices(enumerate);
	devices = udev_enumerate_get_list_entry(enumerate);
	/* For each item, see if it matches the vid/pid, and if so
	   create a udev_device record for it */
	udev_list_entry_foreach(dev_list_entry, devices) {
		const char *sysfs_path;
		const char *dev_path;
		struct udev_device *raw_dev; /* The device's hidraw udev node. */
		struct udev_device *hid_dev; /* The device's HID udev node. */
		unsigned short dev_vid;
		unsigned short dev_pid;
		int bus_type;
		int result = 0;

		/* Get the filename of the /sys entry for the device
		   and create a udev_device object (dev) representing it */
		sysfs_path = udev_list_entry_get_name(dev_list_entry);
		raw_dev = udev_device_new_from_syspath(udev, sysfs_path);
		dev_path = udev_device_get_devnode(raw_dev);

		hid_dev = udev_device_get_parent_with_subsystem_devtype(
			raw_dev,
			"hid",
			NULL);

		if (!hid_dev) {
			/* Unable to find parent hid device. */
			goto next;
		}

		{
			char *tmp = strdup(udev_device_get_sysattr_value(hid_dev, "uevent"));
			char *hid_id = uevent_find_line(tmp, "HID_ID");
			free(tmp);

			if (hid_id) {
				/**
				 *        type vendor   product
				 * HID_ID=0003:000005AC:00008242
				 **/
				int ret = sscanf(hid_id, "%x:%hx:%hx", &bus_type, &dev_vid, &dev_pid);
				free(hid_id);
				if (ret == 3) {
					result = 1;
				}
			}
		}

		if (!result) {
			/* parse_uevent_info() failed for at least one field. */
			goto next;
		}

		if (bus_type != BUS_USB) {
			/* We only know how to handle USB devices. */
			goto next;
		}

		/* Check the VID/PID against the arguments */
		if ((vendor_id == 0x0 || vendor_id == dev_vid) &&
		    (product_id == 0x0 || product_id == dev_pid)) {
			struct hid_device_info *tmp;

			/* VID/PID match. Create the record. */
			tmp = malloc(sizeof(struct hid_device_info));
			if (cur_dev) {
				cur_dev->next = tmp;
			}
			else {
				root = tmp;
			}
			cur_dev = tmp;

			/* Fill out the record */
			cur_dev->next = NULL;
			cur_dev->path = dev_path? strdup(dev_path): NULL;

			/* VID/PID */
			cur_dev->vendor_id = dev_vid;
			cur_dev->product_id = dev_pid;
		}

	next:
		udev_device_unref(raw_dev);
		/* hid_dev, usb_dev and intf_dev don't need to be (and can't be)
		   unref()d.  It will cause a double-free() error.  I'm not
		   sure why.  */
	}
	/* Free the enumerator and udev objects. */
	udev_enumerate_unref(enumerate);
	udev_unref(udev);

	return root;
}

void  HID_API_EXPORT hid_free_enumeration(struct hid_device_info *devs)
{
	struct hid_device_info *d = devs;
	while (d) {
		struct hid_device_info *next = d->next;
		free(d->path);
		free(d);
		d = next;
	}
}

hid_device * HID_API_EXPORT hid_open_path(const char *path)
{
	hid_device *dev = NULL;

	hid_init();

	dev = new_hid_device();

	/* OPEN HERE */
	dev->device_handle = open(path, O_RDWR);

	/* If we have a good handle, return it. */
	if (dev->device_handle > 0) {
		return dev;
	}
	else {
		/* Unable to open any devices. */
		free(dev);
		return NULL;
	}
}


int HID_API_EXPORT hid_write(hid_device *dev, const unsigned char *data, size_t length)
{
	int bytes_written;

	bytes_written = write(dev->device_handle, data, length);

	return bytes_written;
}


int HID_API_EXPORT hid_read_timeout(hid_device *dev, unsigned char *data, size_t length, int milliseconds)
{
	int bytes_read;

	if (milliseconds >= 0) {
		/* Milliseconds is either 0 (non-blocking) or > 0 (contains
		   a valid timeout). In both cases we want to call poll()
		   and wait for data to arrive.  Don't rely on non-blocking
		   operation (O_NONBLOCK) since some kernels don't seem to
		   properly report device disconnection through read() when
		   in non-blocking mode.  */
		int ret;
		struct pollfd fds;

		fds.fd = dev->device_handle;
		fds.events = POLLIN;
		fds.revents = 0;
		ret = poll(&fds, 1, milliseconds);
		if (ret == -1 || ret == 0) {
			/* Error or timeout */
			return ret;
		}
		else {
			/* Check for errors on the file descriptor. This will
			   indicate a device disconnection. */
			if (fds.revents & (POLLERR | POLLHUP | POLLNVAL))
				return -1;
		}
	}

	bytes_read = read(dev->device_handle, data, length);
	if (bytes_read < 0 && (errno == EAGAIN || errno == EINPROGRESS))
		bytes_read = 0;

	return bytes_read;
}

void HID_API_EXPORT hid_close(hid_device *dev)
{
	if (!dev)
		return;
	close(dev->device_handle);
	free(dev);
}


int HID_API_EXPORT_CALL hid_get_manufacturer_string(hid_device *dev, wchar_t *string, size_t maxlen)
{
	return get_device_string(dev, "manufacturer", string, maxlen);
}

int HID_API_EXPORT_CALL hid_get_product_string(hid_device *dev, wchar_t *string, size_t maxlen)
{
	return get_device_string(dev, "product", string, maxlen);
}

HID_API_EXPORT const wchar_t * HID_API_CALL  hid_error(hid_device *dev)
{
	return NULL;
}
