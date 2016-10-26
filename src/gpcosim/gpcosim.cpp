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
#include <vpi_user.h>

void cosim_register();

int hello_compiletf(char* data);
int hello_calltf(char* data);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Table of functions used by iverilog

extern "C"
{
	void (*vlog_startup_routines[])() =
	{
		cosim_register,
		NULL
	};
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Exported stuff called by iverilog

void cosim_register()
{
	//Set up logging
	g_log_sinks.emplace(g_log_sinks.begin(), new STDLogSink(Severity::VERBOSE));

	//Register stuff
	s_vpi_systf_data tf_data;
	tf_data.type      = vpiSysFunc;
	tf_data.tfname    = "$hello";
	tf_data.calltf    = hello_calltf;
	tf_data.compiletf = hello_compiletf;
	tf_data.sizetf    = 0;
	tf_data.user_data = 0;
	vpi_register_systf(&tf_data);
}

int hello_compiletf(char* /*data*/)
{
	return 0;
}

int hello_calltf(char* /*data*/)
{
	LogNotice("function called!!!\n");

	return 0;
}
