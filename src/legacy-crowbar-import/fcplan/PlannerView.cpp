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
	@brief Implementation of PlannerView
 */

#include "fcplan.h"
#include "PlannerView.h"
#include "MainWindow.h"

using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Construction / destruction
PlannerView::PlannerView(MainWindow* parent)
	: m_parent(parent)
	, m_device(NULL)
{
	add_events(
		Gdk::EXPOSURE_MASK |
		Gdk::BUTTON_PRESS_MASK |
		Gdk::BUTTON_RELEASE_MASK);
		
	m_scale = 1;
	
	//default init values to avoid cppcheck warnings... always overwritten before use
	m_width = 0;
	m_height = 0;
}

PlannerView::~PlannerView()
{
	
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Initialization / sizing

void PlannerView::SetDevice(FCDevice* device)
{
	m_device = device;
	Resize();
}

void PlannerView::Resize()
{
	auto size = m_device->GetSize(m_scale);
	set_size(size.first, size.second);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Rendering

bool PlannerView::on_draw(const Cairo::RefPtr<Cairo::Context>& cr)
{
	Glib::RefPtr<Gdk::Window> window = get_bin_window();
	if(window)
	{		
		//Get dimensions of the virtual canvas (max of requested size and window size)
		Gtk::Allocation allocation = get_allocation();
		//int width = allocation.get_width();
		int height = allocation.get_height();
		/*if(m_width > width)
			width = m_width;*/
		if(m_height > height)
			m_height = height;
		auto size = m_device->GetSize(m_scale);
		if(size.first > m_width)
			m_width = size.first;
		if(size.second > m_height)
			m_height = size.second;

		//Set up drawing context
		cr->save();
		cr->set_identity_matrix();
		
		//Fill background
		cr->set_source_rgb(0, 0, 0);
		cr->rectangle(0, 0, m_width, m_height);
		cr->fill();
		
		//Draw the actual device
		cr->scale(m_scale, m_scale);
		
		//double tstart = GetTime();
		m_device->Render(cr, m_scale);
		//double dt = GetTime() - tstart;
		//printf("Frame time = %.2f ms\n", dt*1000);
		
		//Done			
		cr->restore();
	}
	
	return true;
}

void DrawString(float x, float y, const Cairo::RefPtr<Cairo::Context>& cr, string str, string font)
{
	cr->save();
	
		Glib::RefPtr<Pango::Layout> tlayout = Pango::Layout::create (cr);
		cr->move_to(x, y);
		Pango::FontDescription pfont(font);
		pfont.set_weight(Pango::WEIGHT_NORMAL);
		tlayout->set_font_description(pfont);
		tlayout->set_text(str);
		tlayout->update_from_cairo_context(cr);
		tlayout->show_in_cairo_context(cr);
		
	cr->restore();
}

void DrawStringCentered(
	float left, float right, float top, float bottom,
	const Cairo::RefPtr<Cairo::Context>& cr, string str, string font)
{
	cr->save();
	
		//Create the text stuff
		Glib::RefPtr<Pango::Layout> tlayout = Pango::Layout::create (cr);
		Pango::FontDescription pfont(font);
		pfont.set_weight(Pango::WEIGHT_NORMAL);
		tlayout->set_font_description(pfont);
		tlayout->set_text(str);
		
		//Figure out where to put it
		int width;
		int height;
		tlayout->get_pixel_size(width, height);
		float x = left + ( (right-left)/2 ) - width/2;
		float y = top + ( (bottom-top)/2 ) - height/2;
		cr->move_to(x, y);
		
		tlayout->update_from_cairo_context(cr);
		tlayout->show_in_cairo_context(cr);
		
	cr->restore();
}

void DrawStringCenteredVertical(
	float left, float right, float top, float bottom,
	const Cairo::RefPtr<Cairo::Context>& cr, string str, string font)
{
	cr->save();
	
		//Create the text stuff
		Glib::RefPtr<Pango::Layout> tlayout = Pango::Layout::create (cr);
		Pango::FontDescription pfont(font);
		pfont.set_weight(Pango::WEIGHT_NORMAL);
		tlayout->set_font_description(pfont);
		tlayout->set_text(str);
		
		//Figure out where to put it
		int width;
		int height;
		tlayout->get_pixel_size(width, height);
		float x = left + ( (right-left)/2)  - height/4;
		float y = top + ( (bottom-top)/2 ) - width/4;
		cr->move_to(x, y);
		cr->rotate(-M_PI/2);
		
		tlayout->update_from_cairo_context(cr);
		tlayout->show_in_cairo_context(cr);
		
	cr->restore();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Message handlers

bool PlannerView::on_button_press_event(GdkEventButton* event)
{
	//Left button?
	if(event->button == 1)
	{
		
	}
	
	return true;
}

bool PlannerView::on_scroll_event(GdkEventScroll* ev)
{
	//Desired scroll center before zooming: location of the mouse event
	double sv = ev->y;
	double sh = ev->x;
	
	//Do the zoom
	double dscale = 1.5;
	if(ev->delta_y > 0)
		dscale = 1 / 1.5;
	m_scale *= dscale;
	Resize();
	
	//Scale the scroll offsets
	sh *= dscale;
	sv *= dscale;
	
	//Subtract half a page offset to get the upper left location
	auto vadj = m_parent->GetScroller()->get_vadjustment();
	auto hadj = m_parent->GetScroller()->get_hadjustment();
	auto ph = hadj->get_page_size();
	auto pv = vadj->get_page_size();
	sv -= pv/2;
	sh -= ph/2;
	
	hadj->set_value(sh);
	vadj->set_value(sv);
	
	//Done, refresh display	
	queue_draw();
	
	return true;
}
