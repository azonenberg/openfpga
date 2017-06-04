/***********************************************************************************************************************
 * Copyright (C) 2016-2017 Andrew Zonenberg and contributors                                                           *
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

#ifndef gp4tchar_h
#define gp4tchar_h

#include <cstdlib>
#include <cstring>
#include <cmath>
#include <unistd.h>
#include <log.h>
#include <gpdevboard.h>
#include <Greenpak4.h>
#include "../xptools/Socket.h"
#include "TimingData.h"

bool InitializeHardware(hdevice hdev, SilegoPart expectedPart);
bool PostProgramSetup(hdevice hdev, int voltage_mv = 3300);
bool IOReset(hdevice hdev);
bool IOSetup(hdevice hdev);
bool PowerSetup(hdevice hdev, int voltage_mv = 3300);

bool ReadTraceDelays();
bool CalibrateTraceDelays(Socket& sock, hdevice hdev);
bool MeasureDelay(Socket& sock, int src, int dst, float& delay);

bool MeasureLUTDelays(Socket& sock, hdevice hdev);
bool MeasurePinToPinDelays(Socket& sock, hdevice hdev);
bool MeasureCrossConnectionDelays(Socket& sock, hdevice hdev);

void WaitForKeyPress();

extern DevkitCalibration g_devkitCal;
extern Greenpak4Device g_calDevice;

#endif
