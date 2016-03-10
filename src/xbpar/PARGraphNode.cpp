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
 
#include "xbpar.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Construction / destruction

PARGraphNode::PARGraphNode(uint32_t label, void* pData)
	: m_label(label)
	, m_pData(pData)
	, m_mate(NULL)
{
}

PARGraphNode::~PARGraphNode()
{
	
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors

void PARGraphNode::MateWith(PARGraphNode* mate)
{
	//Clear our prior mate, if any
	if(m_mate != NULL)
		m_mate->MateWith(NULL);

	//Valid mate, set up back pointer
	//Clear out old partner of the new mating node, of any
	if(mate != NULL)
	{
		mate->MateWith(NULL);
		mate->m_mate = this;
	}
	
	//and set the forward pointer regardless
	m_mate = mate;
}
