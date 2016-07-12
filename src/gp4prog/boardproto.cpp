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

#include "gp4prog.h"

using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Device I/O

/* For use with the following systemtap script and the vendor tool (run as `stap -g silego.stp`):

function getstr:string(buf:long, off:long, cnt:long, pad:long) %{
  const char hex[] = "0123456789abcdef";
  uint8_t *buf = (*(uint8_t **)STAP_ARG_buf) + 0x10 + STAP_ARG_off;
  int i;
  char *out = STAP_RETVALUE;
  int remain = MAXSTRINGLEN;
  for(i = 0; i < STAP_ARG_cnt - STAP_ARG_off; i++) {
    const char byte[3] = { hex[buf[i] >> 4], hex[buf[i] & 0xf] };
    strlcpy (out, byte, remain);
    out += 2; remain -= 2;
    if(STAP_ARG_pad && (i == 0 || i == 1 || i == 2 || i == 3)) {
      strlcpy(out, "_", remain);
      out += 1; remain -= 1;
    }
  }
%}

global rbuf, rcnt, rtyp
probe process("/usr/lib/libSilegoUSB-2.0.so.1").
    function("_ZN9USBDevice8dataReadER7QVectorIhE").
    call {
  rbuf = register("rsi")
  rcnt = register("rdx")
  rtyp = register("rcx")
}
probe process("/usr/lib/libSilegoUSB-2.0.so.1").
    function("_ZN9USBDevice8dataReadER7QVectorIhE").
    return {
  printf("R%02x %s\n", rtyp, getstr(rbuf, 0, 64, 1));
}

probe process("/usr/lib/libSilegoUSB-2.0.so.1").
    function("_ZN9USBDevice9dataWriteER7QVectorIhE").
    call {
  wbuf = register("rsi")
  wcnt = register("rdx")
  wtyp = register("rcx")
  printf("W%02x %s\n", wtyp, getstr(wbuf, 0, 64, 1));
}

*/
DataFrame::DataFrame(const char *ascii)
{
	uint8_t size;
	sscanf(ascii, "%02hhx_%02hhx_%02hhx_%02hhx_", &m_sequenceA, &m_type, &size, &m_sequenceB);
	for(size_t i=0; i<60; i++) {
		uint8_t byte;
		sscanf(ascii+12+i*2, "%02hhx", &byte);
		m_payload.push_back(byte);
	}
	if(size > 3)
		m_payload.resize(size - 3);
}

void DataFrame::Send(hdevice hdev)
{
	uint8_t data[64] = {};

	//Packet header
	data[0] = m_sequenceA;
	data[1] = m_type;
	if(m_payload.size() == 0)
		data[2] = 0x00;
	else
		data[2] = 3 + m_payload.size();
	data[3] = m_sequenceB;

	//Packet body
	for(size_t i=0; i<m_payload.size(); i++)
		data[4+i] = m_payload[i];

	LogDebug("H→D: ");
	for(int i=0; i<64; i++) {
		LogDebug("%02x", data[i] & 0xff);
		if(i < 4) LogDebug("_");
	}
	LogDebug("\n");

	SendInterruptTransfer(hdev, data, sizeof(data));
}

void DataFrame::Receive(hdevice hdev)
{
	uint8_t data[64];

	ReceiveInterruptTransfer(hdev, data, sizeof(data));

	LogDebug("D→H: ");
	for(int i=0; i<64; i++) {
		LogDebug("%02x", data[i] & 0xff);
		if(i < 4) LogDebug("_");
	}
	LogDebug("\n");

	//Packet header
	uint8_t size;
	m_sequenceA = data[0];
	m_type = data[1];
	if(data[2] == 0x00)
		size = 0;
	else if(data[2] > 3)
		size = data[2] - 3;
	else
		LogFatal("Unexpected size %d\n", data[2]);
	m_sequenceB = data[3];

	//Packet body
	m_payload.resize(size);
	for(size_t i=0; i<m_payload.size(); i++)
		m_payload[i] = data[4+i];
}

void DataFrame::Roundtrip(hdevice hdev, uint8_t ack_type)
{
	Send(hdev);

	// Receive an acknowledgement frame
	DataFrame ack_frame;
	ack_frame.Receive(hdev);

	// Compare the two frames.
	// Received frame will usually have length 0x3f; it is unimportant.
	// Received frame will sometimes have the same sequence number B, sometimes not. It is unimportant.
	if(!(m_sequenceA == ack_frame.m_sequenceA &&
	     ack_type == ack_frame.m_type &&
	     m_payload.size() <= ack_frame.m_payload.size() &&
	     std::equal(m_payload.begin(), m_payload.end(),
	                ack_frame.m_payload.begin()))) {
		LogFatal("Unexpected acknowledgement frame\n");
	}
}

void DataFrame::Roundtrip(hdevice hdev)
{
	Roundtrip(hdev, m_type);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Device I/O

void SwitchMode(hdevice hdev)
{
	uint8_t data[64] = {};
	data[60] = 0x09;
	SendInterruptTransfer(hdev, data, sizeof(data));
}

void SetPart(hdevice hdev, SilegoPart part)
{
	DataFrame frame(DataFrame::SET_PART);
	frame.push_back((uint8_t)part);
	frame.push_back(0x00);
	frame.push_back(0x00);
	frame.push_back(0x00);
	frame.Roundtrip(hdev);
}

void SetStatusLED(hdevice hdev, bool status)
{
	DataFrame frame(DataFrame::SET_STATUS_LED);
	frame.push_back(status);
	frame.Roundtrip(hdev);
}

void SetIOConfig(hdevice hdev, IOConfig& config)
{
	DataFrame frame(DataFrame::CONFIG_IO);

	//Test point config data
	for(unsigned int i=2; i<=20; i++)
	{
		unsigned int cfg = config.driverConfigs[i];
		frame.push_back(cfg >> 8);
		frame.push_back(cfg & 0xff);

		//skip TP11 since that's ground, no config for it
		if(i == 10)
			i++;
	}

	//7 unknown bytes, leave zero for now
	for(size_t i=0; i<7; i++)
		frame.push_back(0);

	//Expansion connector
	uint8_t exp[3] = {0};
	uint8_t expansionBitMap[][2] =
	{
		{0, 0x00},		//unused
		{1, 0x01},		//Vdd
		{2, 0x04},		//TP2
		{2, 0x01},		//TP3
		{2, 0x10},		//TP4
		{2, 0x40},		//TP5
		{0, 0x01},		//TP6
		{0, 0x04},		//TP7
		{0, 0x10},		//TP8
		{0, 0x40},		//TP9
		{0, 0x80},		//TP10
		{0, 0x00},      //GND
		{0, 0x20},		//TP12
		{2, 0x08},		//TP13
		{2, 0x02},		//TP14
		{1, 0x80},		//TP15
		{2, 0x20},		//TP16
		{0, 0x02},		//TP17
		{1, 0x20},		//TP18
		{1, 0x08},		//TP19
		{0, 0x08}		//TP20
	};
	for(unsigned int i=1; i<=20; i++)
	{
		if(config.expansionEnabled[i])
			exp[expansionBitMap[i][0]] |= expansionBitMap[i][1];
	}
	for(size_t i=0; i<3; i++)
		frame.push_back(exp[i]);

	//LEDs from TP3 ... TP15
	unsigned int tpbase = 3;
	for(int i=0; i<3; i++)
	{
		uint8_t ledcfg = 0;
		for(int j=0; j<4; j++)
		{
			uint8_t bitmask = 1 << j;
			uint8_t tpnum = tpbase + j;

			if(config.ledEnabled[tpnum])
				ledcfg |= bitmask;
			if(config.ledInverted[tpnum])
				ledcfg |= (bitmask << 4);
		}
		frame.push_back(ledcfg);

		//Bump pointers. skip TP11 as it's not implemented in the hardware (ground)
		tpbase += 4;
		if(i == 1)
			tpbase ++;
	}

	//LEDs from TP16 ... TP20
	uint8_t leden = 0;
	uint8_t ledinv = 0;
	for(int i=0; i<5; i++)
	{
		uint8_t tpnum = 16 + i;
		uint8_t bitmask = 1 << i;

		if(config.ledEnabled[tpnum])
			leden |= bitmask;
		if(config.ledInverted[tpnum])
			ledinv |= bitmask;
	}
	frame.push_back(leden);
	frame.push_back(ledinv);

	//Always constant, meaning unknown
	frame.push_back(0x1);
	frame.push_back(0x0);
	frame.push_back(0x0);

	//Done, send it
	frame.Send(hdev);
}

//ch1 = Vdd, CH2...20 = TP2...20
//TODO: more than just a dummy placeholder
void ConfigureSiggen(hdevice hdev, uint8_t channel, double voltage)
{
	DataFrame frame(DataFrame::CONFIG_SIGGEN);

	uint16_t raw_voltage = voltage / 0.001362;

	frame.push_back(2);					//signal generator
	frame.push_back(channel);			//channel number
	frame.push_back(1);					//hold at start value before starting
	frame.push_back(0);					//repeat waveform forever
	frame.push_back(1);					//end state: keep last state
	frame.push_back(raw_voltage >> 8);	//voltage
	frame.push_back(raw_voltage & 0xff);
	// frame.push_back(0);					//ramp delay
	// frame.push_back(0);
	// frame.push_back(0);					//integral step part
	// frame.push_back(0);
	// frame.push_back(0);					//step sign and fractional step part
	// frame.push_back(0);

	frame.Roundtrip(hdev);
}

void ResetAllSiggens(hdevice hdev)
{
	DataFrame frame(DataFrame::ENABLE_SIGGEN);

	for(unsigned int i=1; i<=19; i++)
	{
		frame.push_back(SIGGEN_RESET);
	}

	frame.Send(hdev);
}

void SetSiggenStatus(hdevice hdev, unsigned int chan, unsigned int status)
{
	DataFrame frame(DataFrame::ENABLE_SIGGEN);

	for(unsigned int i=1; i<=19; i++)
	{
		if(i == chan)					//apply our status
			frame.push_back(status);

		else
			frame.push_back(SIGGEN_NOP);	//no change
	}

	frame.Send(hdev);
}

void LoadBitstream(hdevice hdev, std::vector<uint8_t> bitstream)
{
	DataFrame frame(DataFrame::WRITE_BITSTREAM_SRAM);
	frame.push_back(0x80);
	frame.push_back(0x00);
	frame.push_back(0x00);

	uint16_t cycles = bitstream.size() * 8 + 34;
	frame.push_back(cycles >> 8);
	frame.push_back(cycles & 0xff);

	frame.m_sequenceB = (bitstream.size() + 3) / 60;

	for(size_t i = 0; i < bitstream.size(); i++) {
		frame.push_back(bitstream[i]);

		if(frame.IsFull()) {
			frame.Roundtrip(hdev, DataFrame::WRITE_BITSTREAM_SRAM_ACK1);
			frame = frame.Next();
		}
	}

	if(!frame.IsEmpty())
		frame.Roundtrip(hdev, DataFrame::WRITE_BITSTREAM_SRAM_ACK2);
}
