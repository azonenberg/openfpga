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
#include <string.h>
#undef _GNU_SOURCE
#include <stdlib.h>
#include <errno.h>

/* Unix */
#include <dirent.h>
#include <unistd.h>
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <poll.h>

/* Linux */
#include <linux/hidraw.h>
#include <linux/version.h>
#include <linux/input.h>

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

// Result needs to be free()d
static char *read_entire_sysfs_file(int fd) {
	struct stat s;
	char *buf;

	if (fstat(fd, &s) == -1)
		return NULL;
	buf = malloc(s.st_size + 1);
	if (!buf)
		return NULL;
	size_t sysfs_size = read(fd, buf, s.st_size);
	if ((size_t)-1 == sysfs_size) {
		free(buf);
		return NULL;
	}

	// Need to add a NULL termination
	buf[sysfs_size] = 0;

	// Deliberately remove a potential \n at the end
	if (buf[sysfs_size - 1] == '\n')
		buf[sysfs_size - 1] = 0;

	return buf;
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

int HID_API_EXPORT hid_init(void)
{
	return 0;
}

int HID_API_EXPORT hid_exit(void)
{
	/* Nothing to do for this in the Linux/hidraw implementation. */
	return 0;
}

struct hid_device_info  HID_API_EXPORT *hid_enumerate(unsigned short vendor_id, unsigned short product_id)
{
	struct hid_device_info *root = NULL; /* return object */
	struct hid_device_info *cur_dev = NULL;

	hid_init();

	DIR *hidraw_class_dir = opendir("/sys/class/hidraw");
	if (!hidraw_class_dir) {
		return NULL;
	}

	struct dirent *hidraw_entry_dirent;

	/* For each item, see if it matches the vid/pid */
	while ((hidraw_entry_dirent = readdir(hidraw_class_dir))) {
		if (strcmp(hidraw_entry_dirent->d_name, ".") == 0)
			continue;
		if (strcmp(hidraw_entry_dirent->d_name, "..") == 0)
			continue;

		char *hidraw_uevent_filename = NULL;
		char *hid_uevent_filename = NULL;
		int hidraw_uevent_fd = -1;
		int hid_uevent_fd = -1;
		char *hidraw_uevent_buf = NULL;
		char *hid_uevent_buf = NULL;
		char *hid_id = NULL;
		char *dev_name = NULL;

		if (asprintf(&hidraw_uevent_filename, "/sys/class/hidraw/%s/uevent", hidraw_entry_dirent->d_name) == -1)
			goto next;
		if (asprintf(&hid_uevent_filename, "/sys/class/hidraw/%s/device/uevent", hidraw_entry_dirent->d_name) == -1)
			goto next;

		if ((hidraw_uevent_fd = open(hidraw_uevent_filename, O_RDONLY)) == -1)
			goto next;
		if ((hid_uevent_fd = open(hid_uevent_filename, O_RDONLY)) == -1)
			goto next;

		if (!(hidraw_uevent_buf = read_entire_sysfs_file(hidraw_uevent_fd)))
			goto next;
		if (!(hid_uevent_buf = read_entire_sysfs_file(hid_uevent_fd)))
			goto next;

		unsigned short dev_vid;
		unsigned short dev_pid;
		int bus_type;

		if (!(hid_id = uevent_find_line(hid_uevent_buf, "HID_ID")))
			goto next;
		if (!(dev_name = uevent_find_line(hidraw_uevent_buf, "DEVNAME")))
			goto next;

		/**
		 *        type vendor   product
		 * HID_ID=0003:000005AC:00008242
		 **/
		int ret = sscanf(hid_id, "%x:%hx:%hx", &bus_type, &dev_vid, &dev_pid);
		if (ret != 3) {
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
			if (asprintf(&(cur_dev->path), "/dev/%s", dev_name) == -1) {
				// FIXME: Why is this considered fallible?
				cur_dev->path = NULL;
			}
		}

	next:
		free(hid_id);
		free(dev_name);
		free(hidraw_uevent_buf);
		free(hid_uevent_buf);
		if (hidraw_uevent_fd != -1)
			close(hidraw_uevent_fd);
		if (hid_uevent_fd != -1)
			close(hid_uevent_fd);
		free(hidraw_uevent_filename);
		free(hid_uevent_filename);
	}
	closedir(hidraw_class_dir);

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

static int get_device_string(hid_device *dev, const char *key_str, wchar_t *string, size_t maxlen)
{
	char *usb_device_sysfs_filename;
	int usb_device_sysfs_fd = 0;
	char *usb_device_sysfs_buf = NULL;
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
	usb_device_sysfs_buf = read_entire_sysfs_file(usb_device_sysfs_fd);
	if (!usb_device_sysfs_buf) {
		ret = -1;
		goto end;
	}

	/* Convert the string from UTF-8 to wchar_t */
	size_t retm;
	retm = mbstowcs(string, usb_device_sysfs_buf, maxlen);
	ret = (retm == (size_t)-1)? -1: 0;

end:
	free(usb_device_sysfs_buf);
	free(usb_device_sysfs_filename);
	if (usb_device_sysfs_fd != -1)
		close(usb_device_sysfs_fd);

	return ret;
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
	(void)dev;
	return NULL;
}
