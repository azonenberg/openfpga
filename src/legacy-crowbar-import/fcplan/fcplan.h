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
	@brief Main project include file
 */
#ifndef fcplan_h
#define fcplan_h

#include <cassert>

#include "../jtaghal/jtaghal.h"
#include "../jtaghal/XilinxCPLDBitstream.h"
#include "../jtaghal/XilinxCoolRunnerIIDevice.h"

#include <gtkmm/box.h>
#include <gtkmm/comboboxtext.h>
#include <gtkmm/dialog.h>
#include <gtkmm/frame.h>
#include <gtkmm/layout.h>
#include <gtkmm/listviewtext.h>
#include <gtkmm/main.h>
#include <gtkmm/messagedialog.h>
#include <gtkmm/notebook.h>
#include <gtkmm/paned.h>
#include <gtkmm/progressbar.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/separator.h>
#include <gtkmm/separatortoolitem.h>
#include <gtkmm/statusbar.h>
#include <gtkmm/stock.h>
#include <gtkmm/toolbar.h>
#include <gtkmm/treestore.h>
#include <gtkmm/treeview.h>
#include <gtkmm/window.h>

#include "FCDevice.h"
#include "FCXilinxDevice.h"
#include "FCCoolRunnerIIDevice.h"

void DrawString(float x, float y, const Cairo::RefPtr<Cairo::Context>& cr, std::string str, std::string font);
void DrawStringCentered(
	float left, float right, float top, float bottom,
	const Cairo::RefPtr<Cairo::Context>& cr, std::string str, std::string font);
void DrawStringCenteredVertical(
	float left, float right, float top, float bottom,
	const Cairo::RefPtr<Cairo::Context>& cr, std::string str, std::string font);

#endif
