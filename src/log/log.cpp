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
#include <cstdarg>
#include <cstdlib>
#include <string>

std::vector<std::unique_ptr<LogSink>> g_log_sinks;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Convenience functions that log into all configured sinks

void LogFatal(const char *format, ...)
{
	va_list va;
	for(auto &sink : g_log_sinks) {
		sink->Log(LogSink::FATAL, "INTERNAL ERROR: ");

		va_start(va, format);
		sink->Log(LogSink::FATAL, format, va);
		va_end(va);

		sink->Log(LogSink::FATAL, 
			"    This indicates a bug in openfpga, please file a report via\n"
			"        https://github.com/azonenberg/openfpga/issues/new.\n");
	}

	abort();
}

void LogError(const char *format, ...)
{
	va_list va;
	for(auto &sink : g_log_sinks) {
		sink->Log(LogSink::ERROR, "ERROR: ");

		va_start(va, format);
		sink->Log(LogSink::ERROR, format, va);
		va_end(va);
	}
}

void LogWarning(const char *format, ...)
{
	va_list va;
	for(auto &sink : g_log_sinks) {
		sink->Log(LogSink::WARNING, "WARNING: ");

		va_start(va, format);
		sink->Log(LogSink::WARNING, format, va);
		va_end(va);
	}
}

void LogNotice(const char *format, ...)
{
	va_list va;
	for(auto &sink : g_log_sinks) {
		va_start(va, format);
		sink->Log(LogSink::NOTICE, format, va);
		va_end(va);
	}
}

void LogVerbose(const char *format, ...)
{
	va_list va;
	for(auto &sink : g_log_sinks) {
		va_start(va, format);
		sink->Log(LogSink::VERBOSE, format, va);
		va_end(va);
	}
}

void LogDebug(const char *format, ...)
{
	va_list va;
	for(auto &sink : g_log_sinks) {
		va_start(va, format);
		sink->Log(LogSink::DEBUG, format, va);
		va_end(va);
	}
}
