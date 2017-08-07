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

#include <log.h>
#include <Greenpak4.h>

using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Construction / destruction

Greenpak4PowerDetector::Greenpak4PowerDetector(
	Greenpak4Device* device,
	unsigned int matrix,
	unsigned int oword)
	: Greenpak4BitstreamEntity(device, matrix, -1, oword, -1)
{
}

Greenpak4PowerDetector::~Greenpak4PowerDetector()
{

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors

string Greenpak4PowerDetector::GetDescription() const
{
	return "PWRDET0";
}

vector<string> Greenpak4PowerDetector::GetInputPorts() const
{
	vector<string> r;
	//no inputs
	return r;
}

void Greenpak4PowerDetector::SetInput(string /*port*/, Greenpak4EntityOutput /*src*/)
{
	//no inputs
}

Greenpak4EntityOutput Greenpak4PowerDetector::GetInput(string /*port*/) const
{
	//no inputs
	return Greenpak4EntityOutput(NULL);
}

vector<string> Greenpak4PowerDetector::GetOutputPorts() const
{
	vector<string> r;
	r.push_back("VDD_LOW");
	return r;
}

unsigned int Greenpak4PowerDetector::GetOutputNetNumber(string port)
{
	if(port == "VDD_LOW")
		return m_outputBaseWord;
	else
		return -1;
}

string Greenpak4PowerDetector::GetPrimitiveName() const
{
	return "GP_PWRDET";
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Serialization

bool Greenpak4PowerDetector::CommitChanges()
{
	//no parameters
	return true;
}

bool Greenpak4PowerDetector::Load(bool* /*bitstream*/)
{
	//no configuration - output only
	return true;
}

bool Greenpak4PowerDetector::Save(bool* /*bitstream*/)
{
	//no configuration - output only
	return true;
}
