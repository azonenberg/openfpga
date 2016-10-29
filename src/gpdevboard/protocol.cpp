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

#include <log.h>
#include <gpdevboard.h>

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
	for(size_t i=0; i<60; i++)
	{
		uint8_t byte;
		sscanf(ascii+12+i*2, "%02hhx", &byte);
		m_payload.push_back(byte);
	}
	if(size > 3)
		m_payload.resize(size - 3);
}

bool DataFrame::Send(hdevice hdev)
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
	for(int i=0; i<64; i++)
	{
		LogDebug("%02x", data[i] & 0xff);
		if(i < 4) LogDebug("_");
	}
	LogDebug("\n");

	return SendInterruptTransfer(hdev, data, sizeof(data));
}

bool DataFrame::Receive(hdevice hdev)
{
	uint8_t data[64];

	if(!ReceiveInterruptTransfer(hdev, data, sizeof(data)))
		return false;

	LogDebug("D→H: ");
	for(int i=0; i<64; i++)
	{
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

	return true;
}

bool DataFrame::Roundtrip(hdevice hdev, uint8_t ack_type)
{
	if(!Send(hdev))
		return false;

	// Receive an acknowledgement frame
	DataFrame ack_frame;
	if(!ack_frame.Receive(hdev))
		return false;

	// Compare the two frames.
	// Received frame will usually have length 0x3f; it is unimportant.
	// Received frame will sometimes have the same sequence number B, sometimes not. It is unimportant.
	if(!(m_sequenceA == ack_frame.m_sequenceA &&
	     ack_type == ack_frame.m_type &&
	     m_payload.size() <= ack_frame.m_payload.size() &&
	     std::equal(m_payload.begin(), m_payload.end(),
	                ack_frame.m_payload.begin())))
	{
		LogError("Unexpected acknowledgement frame\n");
		return false;
	}

	return true;
}

bool DataFrame::Roundtrip(hdevice hdev)
{
	return Roundtrip(hdev, m_type);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Device I/O

bool SwitchMode(hdevice hdev)
{
	uint8_t data[64] = {};
	data[60] = 0x09;
	return SendInterruptTransfer(hdev, data, sizeof(data));
}

bool SetPart(hdevice hdev, SilegoPart part)
{
	DataFrame frame(DataFrame::SET_PART);
	frame.push_back((uint8_t)(part >> 4));
	frame.push_back(0x00);
	frame.push_back(0x00);
	frame.push_back(0x00);
	return frame.Roundtrip(hdev);
}

bool Reset(hdevice hdev)
{
	DataFrame frame(DataFrame::RESET);
	return frame.Roundtrip(hdev);
}

bool SetStatusLED(hdevice hdev, bool status)
{
	DataFrame frame(DataFrame::SET_STATUS_LED);
	frame.push_back(status);
	return frame.Roundtrip(hdev);
}

bool SetIOConfig(hdevice hdev, IOConfig& config)
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
	return frame.Send(hdev);
}

static const double VOLTAGE_FACTOR = 0.001362; //mV/LSB

//ch1 = Vdd, CH2...20 = TP2...20
bool ConfigureSiggen(hdevice hdev, uint8_t channel, double voltage)
{
	DataFrame frame(DataFrame::CONFIG_SIGGEN);

	uint16_t raw_voltage = voltage / VOLTAGE_FACTOR;

	frame.push_back(2);					//signal generator
	frame.push_back(channel);			//channel number
	frame.push_back(1);					//hold at start value before starting
	frame.push_back(0);					//repeat waveform forever
	frame.push_back(0);					//end state: wipe to zero
	frame.push_back(raw_voltage >> 8);	//voltage
	frame.push_back(raw_voltage & 0xff);
	// frame.push_back(0);					//ramp delay
	// frame.push_back(0);
	// frame.push_back(0);					//integral step part
	// frame.push_back(0);
	// frame.push_back(0);					//step sign and fractional step part
	// frame.push_back(0);

	return frame.Roundtrip(hdev);
}

bool ResetAllSiggens(hdevice hdev)
{
	DataFrame frame(DataFrame::ENABLE_SIGGEN);

	for(unsigned int i=1; i<=19; i++)
	{
		frame.push_back((int)SiggenCommand::RESET);
	}

	return frame.Send(hdev);
}

bool ControlSiggen(hdevice hdev, unsigned int chan, SiggenCommand cmd)
{
	DataFrame frame(DataFrame::ENABLE_SIGGEN);

	for(unsigned int i=1; i<=19; i++)
	{
		if(i == chan)					//apply our status
			frame.push_back((int)cmd);

		else
			frame.push_back((int)SiggenCommand::NOP);	//no change
	}

	return frame.Send(hdev);
}

bool DownloadBitstream(hdevice hdev, std::vector<uint8_t> bitstream, DownloadMode mode)
{
	DataFrame::PacketType reqType, ack1Type, ack2Type;
	if(mode == DownloadMode::PROGRAMMING)
	{
		reqType = DataFrame::WRITE_BITSTREAM_NVRAM;
		ack1Type = DataFrame::WRITE_BITSTREAM_NVRAM_ACK1;
		ack2Type = DataFrame::WRITE_BITSTREAM_NVRAM_ACK2;
	}
	else
	{
		// DownloadMode::{EMULATION,TRIMMING}
		reqType = DataFrame::WRITE_BITSTREAM_SRAM;
		ack1Type = DataFrame::WRITE_BITSTREAM_SRAM_ACK1;
		ack2Type = DataFrame::WRITE_BITSTREAM_SRAM_ACK2;
	}

	DataFrame frame(reqType);
	frame.push_back((mode == DownloadMode::TRIMMING) ? 0x87 : 0x80);
	frame.push_back(0x00);
	frame.push_back(0x00);

	uint16_t cycles = bitstream.size() * 8 + 34;
	frame.push_back(cycles >> 8);
	frame.push_back(cycles & 0xff);

	frame.m_sequenceB = (bitstream.size() + 3) / 60;

	for(size_t i = 0; i < bitstream.size(); i++)
	{
		frame.push_back(bitstream[i]);

		if(frame.IsFull())
		{
			if(!frame.Roundtrip(hdev, ack1Type))
				return false;
			frame = frame.Next();
		}
	}

	if(!frame.IsEmpty())
	{
		if(!frame.Roundtrip(hdev, ack2Type))
			return false;
	}

	return true;
}

bool UploadBitstream(hdevice hdev, size_t octets, vector<uint8_t> &bitstream)
{
	DataFrame reqFrame(DataFrame::READ_BITSTREAM_START);
	reqFrame.push_back(0xc0);
	reqFrame.push_back(0x07);
	reqFrame.push_back(0xd0);

	uint16_t cycles = octets * 8 + 34;
	reqFrame.push_back(cycles >> 8);
	reqFrame.push_back(cycles & 0xff);

	bitstream = {};
	while(true)
	{
		if(!reqFrame.Send(hdev))
			return false;

		DataFrame repFrame;
		if(!repFrame.Receive(hdev))
			return false;
		if(!(repFrame.m_sequenceA == reqFrame.m_sequenceA &&
		     repFrame.m_type == DataFrame::READ_BITSTREAM_ACK))
		{
			LogError("Unexpected reply\n");
			return false;
		}

		bitstream.insert(bitstream.end(), repFrame.m_payload.begin(), repFrame.m_payload.end());
		if(repFrame.m_sequenceB == 0)
		{
			break;
		}

		reqFrame.m_type = DataFrame::READ_BITSTREAM_CONT;
	}

	if(bitstream.size() != octets)
	{
		LogError("Unexpected size of uploaded bitstream\n");
		return false;
	}

	return true;
}

bool SelectADCChannel(hdevice hdev, unsigned int chan)
{
	DataFrame frame(DataFrame::CONFIG_ADC_MUX);
	frame.push_back(0x00);
	if(chan >= 2 && chan <= 10)
		frame.push_back(chan - 1);
	else if(chan >= 12 && chan <= 20)
		frame.push_back(chan - 2);
	else
		LogFatal("Unexpected ADC channel (%d)\n", chan);
	frame.push_back(0x00);
	return frame.Send(hdev);
}

bool ReadADC(hdevice hdev, double &value)
{
	DataFrame frame(DataFrame::READ_ADC);
	frame.push_back(0x01); // conversion time
	if(!frame.Send(hdev))
		return false;

	if(!frame.Receive(hdev))
		return false;
	if(!(frame.m_type == DataFrame::READ_ADC))
	{
		LogError("Unexpected reply\n");
		return false;
	}
	uint32_t intValue =
		(frame.m_payload[0] << 24) |
		(frame.m_payload[1] << 16) |
		(frame.m_payload[2] <<  8) |
		(frame.m_payload[3] <<  0);
	value = (double)(((int32_t)intValue) >> 8) / 0x90000;

	//Datasheet comes back as a fraction of full scale
	//Multiply by the 1.024V reference
	value *= 1.024;

	return true;
}

bool TrimOscillator(hdevice hdev, uint8_t ftw)
{
	DataFrame reqFrame = DataFrame(DataFrame::TRIM_OSC);
	reqFrame.push_back(0x00);
	reqFrame.push_back(ftw);
	reqFrame.push_back(0x05);
	reqFrame.push_back(0x04);
	reqFrame.push_back(0x0a);
	reqFrame.push_back(0x11);
	reqFrame.push_back(0x02);
	reqFrame.push_back(0x01);
	if(!reqFrame.Send(hdev))
		return false;

	DataFrame repFrame;
	if(!repFrame.Receive(hdev))
		return false;
	if(!(repFrame.m_type == reqFrame.m_type))
	{
		LogError("Unexpected reply\n");
		return false;
	}
	return true;
}

bool MeasureOscillatorFrequency(hdevice hdev, unsigned &freq)
{
	DataFrame reqFrame = DataFrame(DataFrame::GET_OSC_FREQ);
	reqFrame.push_back(0x00);
	if(!reqFrame.Send(hdev))
		return false;

	DataFrame repFrame;
	if(!repFrame.Receive(hdev))
		return false;
	if(!(repFrame.m_type == reqFrame.m_type))
	{
		LogError("Unexpected reply\n");
		return false;
	}

	freq =
		(repFrame.m_payload[0] << 24) |
		(repFrame.m_payload[1] << 16) |
		(repFrame.m_payload[2] <<  8) |
		(repFrame.m_payload[3] <<  0);
	return true;
}

bool GetStatus(hdevice hdev, BoardStatus &status)
{
	DataFrame frame(DataFrame::GET_STATUS);
	if(!frame.Send(hdev))
		return false;

	// FIXME: we don't get any nonzero measurements here. It seems we are missing some sort of enable
	// command for this feature. I haven't a faintest clue as to which.
	if(!frame.Receive(hdev))
		return false;
	if(!(frame.m_type == DataFrame::GET_STATUS))
	{
		LogError("Unexpected reply\n");
		return false;
	}

	// uint16_t rawCurrent  = (frame.m_payload[10] << 8) | frame.m_payload[11];
	uint16_t rawVoltageA = (frame.m_payload[12] << 8) | frame.m_payload[13];
	uint16_t rawVoltageB = (frame.m_payload[14] << 8) | frame.m_payload[15];

	status.externalOverCurrent  = (frame.m_payload[7] == 0x01);
	status.internalUnderVoltage = (frame.m_payload[8] == 0x01);
	status.internalOverCurrent  = (frame.m_payload[9] == 0x02);
	status.voltageA = rawVoltageA * VOLTAGE_FACTOR / 2;
	status.voltageB = rawVoltageB * VOLTAGE_FACTOR / 2;
	return true;
}
