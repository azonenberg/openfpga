/***********************************************************************************************************************
 * Copyright (C) 2016 Andrew Zonenberg and contributors                                                                *
 *                                                                                                                     *
 * This program is free software; you can redistribute it and/or modify it under the terms of the GNU Lesser General   *
 * Public License as published by the Free Software Foundation; either version 2.1 of the License, or (at your option) *
 * any later version.                                                                                                  *
 *                                                                                                                     *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied  *
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for     *
 * more details.                                                                                                       *
 *                                                                                                                     *
 * You should have received a copy of the GNU Lesser General Public License along with this program; if not, you may   *
 * find one here:                                                                                                      *
 * https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt                                                              *
 * or you may search the http://www.gnu.org website for the version 2.1 license, or you may write to the Free Software *
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA                                      *
 **********************************************************************************************************************/

//Windows appears to define an ERROR macro in its headers.
//Conflicts with ERROR enum defined in log.h.
#if defined(_WIN32)
	#undef ERROR
#endif

#include <log.h>
#include <gpdevboard.h>
#include <unistd.h>
#include <cstdlib>
#include <cstring>

using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// USB command helpers

bool SendInterruptTransfer(hdevice hdev, const uint8_t* buf, size_t size)
{
	//We need to prepend a zero byte here because this is the "report ID."
	//There aren't actually report IDs in use, but the way hidapi works
	//always requires this.
	uint8_t *new_buf = (uint8_t *)malloc(size + 1);
	new_buf[0] = 0x00;
	memcpy(&new_buf[1], buf, size);
	if(hid_write(hdev, new_buf, size + 1) < 0)
	{
		free(new_buf);
		LogError("hid_write failed (%ls)\n", hid_error(hdev));
		return false;
	}
	free(new_buf);
	return true;
}

bool ReceiveInterruptTransfer(hdevice hdev, uint8_t* buf, size_t size)
{
	if(hid_read_timeout(hdev, buf, size, 250) < 0)
	{
		LogError("hid_read_timeout failed (%ls)\n", hid_error(hdev));
		return false;
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Enumeration / setup helpers

//Set up USB stuff
bool USBSetup()
{
	if(0 != hid_init())
	{
		LogError("hid_init failed\n");
		return false;
	}
	return true;
}

void USBCleanup(hdevice hdev)
{
	hid_close(hdev);
	hid_exit();
}

/**
	@brief Gets the device handle

	@param idVendor		USB VID
	@param idProduct	USB PID
	@param nboard		Number of the board to open.
						Note that this index is counted for all VID matches regardless of PID!
						This is important so we can match both bootloader and operating dev boards.
 */
hdevice OpenDevice(uint16_t idVendor, uint16_t idProduct, int nboard)
{
	//initial sanity check
	if(nboard < 0)
	{
		LogError("invalid device index (should be >0)\n");
		return NULL;
	}

	int dev_index = 0;
	struct hid_device_info *devs, *cur_dev;
	devs = hid_enumerate(idVendor, idProduct);
	cur_dev = devs;
	while (cur_dev) {
		LogDebug("Found Silego device at %s\n", cur_dev->path);
		if (dev_index == nboard)
			break;
		cur_dev = cur_dev->next;
		dev_index++;
	}

	hdevice hdev;
	if (!cur_dev)
	{
		hid_free_enumeration(devs);
		return NULL;
	}

	LogVerbose("Using device at %s\n", cur_dev->path);
	hdev = hid_open_path(cur_dev->path);
	hid_free_enumeration(devs);
	if (!hdev)
	{
		LogError("hid_open_path failed\n");
		return NULL;
	}

	return hdev;
}

//Gets a string descriptor as a STL string
bool GetStringDescriptor(hdevice hdev, uint8_t index, string &desc)
{
	char strbuf[128];
	wchar_t wstrbuf[sizeof(strbuf)];

	switch (index)
	{
		case 1:
			if (hid_get_product_string(hdev, wstrbuf, sizeof(strbuf)) < 0)
			{
				LogFatal("hid_get_product_string failed\n");
				return false;
			}
			break;

		case 2:
			if (hid_get_manufacturer_string(hdev, wstrbuf, sizeof(strbuf)) < 0)
			{
				LogFatal("hid_get_manufacturer_string failed\n");
				return false;
			}
			break;

		default:
			LogFatal("Invalid index %d\n", index);
			return false;
	}

	wcstombs(strbuf, wstrbuf, sizeof(strbuf));

	desc = strbuf;
	return true;
}
