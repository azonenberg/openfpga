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
#include <Greenpak4.h>

using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Construction / destruction

Greenpak4DigitalComparator::Greenpak4DigitalComparator(
	Greenpak4Device* device,
	unsigned int cmpnum,
	unsigned int matrix,
	unsigned int ibase,
	unsigned int oword,
	unsigned int cbase)
	: Greenpak4BitstreamEntity(device, matrix, ibase, oword, cbase)
	, m_cmpNum(cmpnum)
{
}

Greenpak4DigitalComparator::~Greenpak4DigitalComparator()
{

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors

string Greenpak4DigitalComparator::GetDescription()
{
	char buf[128];
	snprintf(buf, sizeof(buf), "DCMP_%u", m_cmpNum);
	return string(buf);
}

vector<string> Greenpak4DigitalComparator::GetInputPorts() const
{
	vector<string> r;
	r.push_back("PWRDN");
	return r;
}

void Greenpak4DigitalComparator::SetInput(string /*port*/, Greenpak4EntityOutput /*src*/)
{
	//no inputs
}

vector<string> Greenpak4DigitalComparator::GetOutputPorts() const
{
	vector<string> r;
	r.push_back("OUTP");
	r.push_back("OUTN");
	return r;
}

unsigned int Greenpak4DigitalComparator::GetOutputNetNumber(string port)
{
	if(port == "OUTN")
		return m_outputBaseWord;
	else if(port == "OUTP")
		return m_outputBaseWord + 1;
	else
		return -1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Serialization

bool Greenpak4DigitalComparator::CommitChanges()
{
	//no parameters
	return true;
}

bool Greenpak4DigitalComparator::Load(bool* /*bitstream*/)
{
	LogError("Unimplemented\n");
	return false;
}

bool Greenpak4DigitalComparator::Save(bool* /*bitstream*/)
{
	//no configuration - output only
	return true;
}
