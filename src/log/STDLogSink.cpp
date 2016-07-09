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
#include <cstdio>
#include <cstdarg>
#include <string>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Construction / destruction

STDLogSink::STDLogSink(Severity min_severity)
	: m_min_severity(min_severity)
{
}

STDLogSink::~STDLogSink()
{
	fflush(stdout);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Logging 

void STDLogSink::Log(Severity severity, const std::string &msg)
{
	if(severity <= WARNING) {
		// Prevent newer messages on stderr from appearing before older messages on stdout
		fflush(stdout);
		fputs(msg.c_str(), stderr);
	}
	else if(severity <= m_min_severity)
		fputs(msg.c_str(), stdout);
}

void STDLogSink::Log(Severity severity, const char *format, va_list va) 
{
	if(severity <= WARNING) {
		// See above
		fflush(stdout);
		vfprintf(stderr, format, va);
	}
	else if(severity <= m_min_severity)
		vfprintf(stdout, format, va);
}
