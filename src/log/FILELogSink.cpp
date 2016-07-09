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

#include "log.h"
#include <string>
#include <cstdio>
#include <cstdarg>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Construction / destruction

FILELogSink::FILELogSink(FILE *f, bool line_buffered, Severity min_severity)
	: m_file(f)
	, m_min_severity(min_severity)
{
	if(line_buffered) 
		setvbuf(f, NULL, _IOLBF, 0);
}

FILELogSink::~FILELogSink()
{
	fclose(m_file);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Logging 

void FILELogSink::Log(Severity severity, const std::string &msg)
{
	if(severity <= m_min_severity)
		fputs(msg.c_str(), m_file);
}

void FILELogSink::Log(Severity severity, const char *format, va_list va) 
{
	if(severity <= m_min_severity)
		vfprintf(m_file, format, va);
}
