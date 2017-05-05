/***********************************************************************************************************************
*                                                                                                                      *
* ANTIKERNEL v0.1                                                                                                      *
*                                                                                                                      *
* Copyright (c) 2012-2016 Andrew D. Zonenberg                                                                          *
* All rights reserved.                                                                                                 *
*                                                                                                                      *
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that the     *
* following conditions are met:                                                                                        *
*                                                                                                                      *
*    * Redistributions of source code must retain the above copyright notice, this list of conditions, and the         *
*      following disclaimer.                                                                                           *
*                                                                                                                      *
*    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the       *
*      following disclaimer in the documentation and/or other materials provided with the distribution.                *
*                                                                                                                      *
*    * Neither the name of the author nor the names of any contributors may be used to endorse or promote products     *
*      derived from this software without specific prior written permission.                                           *
*                                                                                                                      *
* THIS SOFTWARE IS PROVIDED BY THE AUTHORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED   *
* TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL *
* THE AUTHORS BE HELD LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES        *
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR       *
* BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT *
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE       *
* POSSIBILITY OF SUCH DAMAGE.                                                                                          *
*                                                                                                                      *
***********************************************************************************************************************/

/**
	@file 
	@author Andrew D. Zonenberg
	@brief Implementation of main application window class
 */

#include "fcplan.h"
#include "MainWindow.h"

using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////
// Construction / destruction

/**
	@brief Initializes the main window
 */
MainWindow::MainWindow(std::string fname)
	: m_planner(this)
{
	//Load the bitstream (TODO: support blank/creation/viewing?)
	FILE* fp = fopen(fname.c_str(), "rb");
	if(!fp)
	{
		throw JtagExceptionWrapper(
			string("Failed to open firmware image ") + fname,
			"",
			JtagException::EXCEPTION_TYPE_GIGO);
	}
	fseek(fp, 0, SEEK_END);
	size_t len = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	unsigned char* image = new unsigned char[len];
	if(len != fread(image, 1, len, fp))
	{
		fclose(fp);
		delete[] image;
		
		throw JtagExceptionWrapper(
			"Failed to read firmware image",
			"",
			JtagException::EXCEPTION_TYPE_GIGO);
	}
	fclose(fp);
	m_bitstream = new XilinxCPLDBitstream;
	CPLD::ParseJEDFile(m_bitstream, image, len);
	delete[] image;
	
	//Create a CPLD object around it
	m_device = FCDevice::CreateDevice(m_bitstream);
	m_planner.SetDevice(m_device);
	
	//Set window title
	char title[256];
	snprintf(title, sizeof(title), "Flying Crowbar floorplanner - %s (device %s)",
		fname.c_str(),
		m_bitstream->devname.c_str()
		);
	set_title(title);
	
	//Initial setup
	set_reallocate_redraws(true);
	set_default_size(1400, 800);

	//Add widgets
	CreateWidgets();
	
	//Done adding widgets
	show_all();
}

/**
	@brief Application cleanup
 */
MainWindow::~MainWindow()
{
}

/**
	@brief Helper function for creating widgets and setting up signal handlers
 */
void MainWindow::CreateWidgets()
{	
	//Set up window hierarchy
	add(m_vbox);
		m_vbox.pack_start(m_scroller);
			m_scroller.add(m_planner);
					
	//Set dimensions
	m_scroller.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
}
