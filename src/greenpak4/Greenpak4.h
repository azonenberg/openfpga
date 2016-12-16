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

#ifndef Greenpak4_h
#define Greenpak4_h

/**
	@file
	@brief Master include file for all Greenpak4 related stuff
 */

#include "Greenpak4BitstreamEntity.h"
#include "Greenpak4EntityOutput.h"
#include "Greenpak4DualEntity.h"

#include "Greenpak4Abuf.h"
#include "Greenpak4Bandgap.h"
#include "Greenpak4ClockBuffer.h"
#include "Greenpak4Counter.h"
#include "Greenpak4Comparator.h"
#include "Greenpak4CrossConnection.h"
#include "Greenpak4DAC.h"
#include "Greenpak4DCMPMux.h"
#include "Greenpak4DCMPRef.h"
#include "Greenpak4Delay.h"
#include "Greenpak4DigitalComparator.h"
#include "Greenpak4Flipflop.h"
#include "Greenpak4Inverter.h"
#include "Greenpak4IOB.h"
#include "Greenpak4IOBTypeA.h"
#include "Greenpak4IOBTypeB.h"
#include "Greenpak4LFOscillator.h"
#include "Greenpak4LUT.h"
#include "Greenpak4MuxedClockBuffer.h"
#include "Greenpak4PatternGenerator.h"
#include "Greenpak4PGA.h"
#include "Greenpak4PairedEntity.h"
#include "Greenpak4PowerDetector.h"
#include "Greenpak4PowerOnReset.h"
#include "Greenpak4PowerRail.h"
#include "Greenpak4RCOscillator.h"
#include "Greenpak4RingOscillator.h"
#include "Greenpak4ShiftRegister.h"
#include "Greenpak4SystemReset.h"
#include "Greenpak4VoltageReference.h"

#include "Greenpak4NetlistNode.h"
#include "Greenpak4NetlistCell.h"
#include "Greenpak4NetlistModule.h"
#include "Greenpak4NetlistPort.h"
#include "Greenpak4Netlist.h"

#include "Greenpak4Device.h"

#endif
