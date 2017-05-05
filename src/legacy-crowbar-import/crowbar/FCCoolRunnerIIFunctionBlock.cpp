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
	@author Andrew D.Zonenberg
	@brief Implementation of FCCoolRunnerIIFunctionBlock
 */

#include "crowbar.h"
#include "FCCoolRunnerIIDevice.h"
#include "FCCoolRunnerIIFunctionBlock.h"
#include "FCCoolRunnerIIMacrocell.h"
#include "../jtaghal/JEDFileWriter.h"
#include <unordered_set>

using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Construction / destruction

FCCoolRunnerIIFunctionBlock::FCCoolRunnerIIFunctionBlock(FCCoolRunnerIIDevice* device, int blocknum, int zia_width)
	: m_device(device)
	, m_blocknum(blocknum)
	, m_ziaWidth(zia_width)
{
	//Create macrocells
	for(int i=0; i<MACROCELLS_PER_FB; i++)
		m_macrocells.push_back(new FCCoolRunnerIIMacrocell(this, i+1));
		
	//Fill the PLA with "blank"
	for(int i=0; i<PLA_PRODUCT_TERMS; i++)
	{
		for(int j=0; j<PLA_INPUT_WIDTH; j++)
			m_plaAnd[i][j] = true;	
		for(int j=0; j<MACROCELLS_PER_FB; j++)
			m_plaOr[i][j] = true;
	}
	
	//Initialize the ZIA
	for(int i=0; i<INPUT_TERM_COUNT; i++)
	{
		m_zia[i] = new bool[zia_width];
		for(int j=0; j<zia_width; j++)
			m_zia[i][j] = true;
	}
}

FCCoolRunnerIIFunctionBlock::~FCCoolRunnerIIFunctionBlock()
{
	//Clean up macrocells
	for(size_t i=0; i<m_macrocells.size(); i++)
		delete m_macrocells[i];
	m_macrocells.clear();
	
	//PLA is statically allocated / fixed size, no cleanup necessary
	
	//Clean up ZIA
	for(int i=0; i<INPUT_TERM_COUNT; i++)
		delete[] m_zia[i];
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors

/**
	@brief Looks up a macrocell
 */
FCCoolRunnerIIMacrocell* FCCoolRunnerIIFunctionBlock::GetMacrocell(int i)
{
	if( (i < 0) || (i >= MACROCELLS_PER_FB) )
	{
		throw JtagExceptionWrapper(
			"Invalid macrocell number",
			"",
			JtagException::EXCEPTION_TYPE_GIGO);
	}
	
	return m_macrocells[i];
}

/**
	@brief Gets the index of this function block within the device
 */
int FCCoolRunnerIIFunctionBlock::GetBlockNumber()
{
	return m_blocknum;
}

/**
	@brief Determines if the OR term with a given (zero-based) index is used.
 */
bool FCCoolRunnerIIFunctionBlock::IsOrTermUsed(int macrocell)
{
	for(int pterm=0; pterm<PLA_PRODUCT_TERMS; pterm++)
	{
		if(!m_plaOr[pterm][macrocell])
			return true;
	}
	return false;
}

bool FCCoolRunnerIIFunctionBlock::IsProductTermUsed(int pterm)
{
	for(int i=0; i<PLA_INPUT_WIDTH; i++)
	{
		if(!m_plaAnd[pterm][i])
			return true;
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Debug output

void FCCoolRunnerIIFunctionBlock::Dump()
{
	//Header
	printf("\n");
	printf("    Function block %d:\n", m_blocknum);

	//PLA AND array
	//Print it rotated 90 degrees from the canonical orientation in the JED file
	//for easier readability
	printf("        ┌");
	for(int i=0; i<PLA_PRODUCT_TERMS; i++)
		printf("─");
	printf("┐\n");
	for(int i=0; i<PLA_INPUT_WIDTH; i++)
	{
		//Print the AND array row
		printf("        │");
		for(int pterm=0; pterm<PLA_PRODUCT_TERMS; pterm++)
			printf("%c", m_plaAnd[pterm][i] ? ' ' : '*');
		printf("├");
		
		//Print the corresponding blob of the ZIA (TODO: figure out where this should go?)
		//This is just a GUESS as to where the signals go. TODO figure out more details
		if((i & 1) )
		{
			printf("──o◁──┘");
		}
		else
		{
			printf("──────┬─◇ ");
			
			//Check if ZIA is all 1s, that means "not connected"
			bool all_ones = true;
			for(int j=0; j<m_ziaWidth; j++)
				all_ones &= m_zia[i/2][j];
			if(all_ones)
			{
				//not connected
			}

			else
			{
				uint8_t zia_val = 0;
				for(int j=0; j<m_ziaWidth; j++)
				{
					zia_val <<= 1;
					zia_val |= !m_zia[i/2][j];
					if( (j & 7) == 7)
					{
						//XC2C32A
						if(m_ziaWidth == 8)
						{
							char muxsel[16];
							snprintf(muxsel, sizeof(muxsel), "%02x", zia_val);
							FCCoolRunnerIINetSource* net = m_device->DecodeZIAEntry(i/2, muxsel);
							printf("%s", net->GetTopLevelNetName().c_str());
							break;
						}
						
						//Unknown
						else
							printf("%02x", zia_val);
					}
				}
			}
		}
		
		printf("\n");
	}
	printf("        └");
	for(int i=0; i<PLA_PRODUCT_TERMS; i++)
		printf("┬");
	printf("┘\n");
	
	//Wires connecting the two arrays
	printf("         ");
	for(int i=0; i<PLA_PRODUCT_TERMS; i++)
		printf("│");
	printf("      /%d product terms to OR array\n", PLA_PRODUCT_TERMS);
	
	//PLA OR array
	//Print it rotated 90 degrees from the canonical orientation in the JED file
	//for easier readability
	printf("        ┌");
	for(int i=0; i<PLA_PRODUCT_TERMS; i++)
		printf("┴");
	printf("┐\n");
	for(int i=0; i<MACROCELLS_PER_FB; i++)
	{
		printf("        │");
		for(int pterm=0; pterm<PLA_PRODUCT_TERMS; pterm++)
			printf("%c", m_plaOr[pterm][i] ? ' ' : '*');
		printf("├───────────────────◇ To macrocell %d\n", i+1);
	}	
	printf("        └");
	for(int i=0; i<PLA_PRODUCT_TERMS; i++)
		printf("─");
	printf("┘\n");

	//Dump the macrocells
	m_macrocells[0]->DumpHeader();
	for(size_t i=0; i<m_macrocells.size(); i++)
		m_macrocells[i]->Dump();
	m_macrocells[0]->DumpFooter();
}

void FCCoolRunnerIIFunctionBlock::DumpRTL()
{	
	printf("    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////\n");	
	printf("    // Function block %d\n", m_blocknum);
	printf("\n");
		
	//Generate HDL for the PLA
	printf("    // PLA AND array\n");
	for(int pterm=0; pterm<PLA_PRODUCT_TERMS; pterm++)
	{
		string vars = "";
		for(int i=0; i<PLA_INPUT_WIDTH; i++)
		{
			if(!m_plaAnd[pterm][i])
			{
				string var;
				
				bool all_ones = true;
				for(int j=0; j<m_ziaWidth; j++)
					all_ones &= m_zia[i/2][j];
				if(all_ones)
				{
					//not connected
				}
				else
				{
					uint8_t zia_val = 0;
					for(int j=0; j<m_ziaWidth; j++)
					{
						zia_val <<= 1;
						zia_val |= !m_zia[i/2][j];
						if( (j & 7) == 7)
						{
							//XC2C32A
							if(m_ziaWidth == 8)
							{
								char muxsel[16];
								snprintf(muxsel, sizeof(muxsel), "%02x", zia_val);
								FCCoolRunnerIINetSource* net = m_device->DecodeZIAEntry(i/2, muxsel);
								var = net->GetTopLevelNetName();
								if(i & 1)
									var = string("!") + var;
								break;
							}
						}
					}
				}
				
				if(vars == "")
					vars = var;
				else
					vars = vars + " & " + var;
			}
		}
		
		if(vars != "")
			printf("    wire fb%d_pterm_%d = %s;\n", m_blocknum, pterm+1, vars.c_str());
	}
	printf("\n    // PLA OR array\n");
	for(int i=0; i<MACROCELLS_PER_FB; i++)
	{
		string vars = "";
		for(int pterm=0; pterm<PLA_PRODUCT_TERMS; pterm++)
		{
			if(!m_plaOr[pterm][i])
			{
				char var[128];
				snprintf(var, sizeof(var), "fb%d_pterm_%d", m_blocknum, pterm+1);
				if(vars == "")
					vars = var;
				else
					vars = vars + " | " + var;
			}
		}
		if(vars != "")
			printf("    wire fb%d_%d_orterm = %s;\n", m_blocknum, i+1, vars.c_str());
	}
	
	//Generate HDL for the macrocells
	printf("\n    // Macrocells\n");
	for(int i=0; i<MACROCELLS_PER_FB; i++)
		m_macrocells[i]->DumpRTL();
	printf("\n");
}

void FCCoolRunnerIIFunctionBlock::DumpIOBRTL()
{
	printf("    // Function block %d I/O nets\n", m_blocknum);
	for(int i=0; i<MACROCELLS_PER_FB; i++)
		m_macrocells[i]->DumpIOBRTL();
	printf("\n");
}

/** 
	@brief Gets the names of all top-level nets we interact with
 */
void FCCoolRunnerIIFunctionBlock::GetTopLevelNetNames(unordered_set<string>& names)
{
	for(int i=0; i<MACROCELLS_PER_FB; i++)
	{
		//OUTPUTS
		FCCoolRunnerIIIOB& iob = m_macrocells[i]->m_iob;
		if(
			(iob.GetOE() == FCCoolRunnerIIIOB::OE_OUTPUT) ||
			(iob.GetOE() == FCCoolRunnerIIIOB::OE_OPENDRAIN) ||
			(iob.GetOE() == FCCoolRunnerIIIOB::OE_TRISTATE)
			)
		{
			names.emplace(iob.GetTopLevelNetName());
		}
		
		//INPUTS
		if(iob.GetInZ() == FCCoolRunnerIIIOB::INZ_INPUT)
		{
			names.emplace(iob.GetTopLevelNetName());
		}
	}
}

/**
	@brief Determines if a given (zero based) macrocell's output is used anywhere.
 */
bool FCCoolRunnerIIFunctionBlock::IsMacrocellOutputUsed(int macrocell)
{
	//If the output buffer is enabled, we're active
	int mode = m_macrocells[macrocell]->m_iob.GetOE();
	if(mode == FCCoolRunnerIIIOB::OE_OUTPUT)
		return true;
	if(mode == FCCoolRunnerIIIOB::OE_OPENDRAIN)
		return true;
	if(mode == FCCoolRunnerIIIOB::OE_TRISTATE)
		return true;
		
	//If we get here, output buffer is NOT enabled.
	//See if the flipflop or net is used anywhere.
	for(size_t i=0; i<m_device->GetBlockCount(); i++)
	{
		FCCoolRunnerIIFunctionBlock* block = m_device->GetFunctionBlock(i);
		for(size_t j=0; j<INPUT_TERM_COUNT; j++)
		{
			if(m_ziaWidth != 8)
			{
				throw JtagExceptionWrapper(
					"Larger ZIA values not decoded yet",
					"",
					JtagException::EXCEPTION_TYPE_GIGO);
			}
			
			//Skip ZIA entries with high-order bit not used (this is the valid flag)
			if(block->m_zia[j/2][0])
				continue;
			
			uint8_t zia_val = 0;
			for(int k=0; k<m_ziaWidth; k++)
			{
				zia_val <<= 1;
				zia_val |= !block->m_zia[j/2][k];
				if( k == 7)
					break;
			}
			char muxsel[16];
			snprintf(muxsel, sizeof(muxsel), "%02x", zia_val);
			FCCoolRunnerIINetSource* net = m_device->DecodeZIAEntry(j/2, muxsel);
			
			if(net == m_macrocells[macrocell])
				return true;
		}
	}
	
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Initialization

int FCCoolRunnerIIFunctionBlock::LoadFromBitstream(bool* bitstream, int start_address)
{
	//printf("    Loading function block %d, starting at fuse %d...\n", m_blocknum, start_address);

	/*
		Function block structure
		
		ZIA
		AND array
		OR array
		Macrocell configuration
	 */

	//Load ZIA
	int addr = start_address;
	//printf("        Loading ZIA (%d x %d) starting at fuse %d\n", m_ziaWidth, INPUT_TERM_COUNT, addr);
	for(int i=0; i<INPUT_TERM_COUNT; i++)
		for(int j=0; j<m_ziaWidth; j++)
			m_zia[i][j] = bitstream[addr++];
	
	//Load AND array
	/*printf("        Loading PLA AND array (%d x %d), starting at fuse %d\n",
		PLA_INPUT_WIDTH, PLA_PRODUCT_TERMS, addr);*/
	for(int i=0; i<PLA_PRODUCT_TERMS; i++)
		for(int j=0; j<PLA_INPUT_WIDTH; j++)
			m_plaAnd[i][j] = bitstream[addr++];
			
	//Load OR array
	/*printf("        Loading PLA OR array (%d x %d), starting at fuse %d\n",
		PLA_PRODUCT_TERMS, MACROCELLS_PER_FB, addr);*/
	for(int j=0; j<PLA_PRODUCT_TERMS; j++)
		for(int i=0; i<MACROCELLS_PER_FB; i++)
			m_plaOr[j][i] = bitstream[addr++];	
			
	//Load macrocells
	for(size_t i=0; i<m_macrocells.size(); i++)
		addr = m_macrocells[i]->LoadFromBitstream(bitstream, addr);
		
	//Done	
	return addr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Serialization

void FCCoolRunnerIIFunctionBlock::SaveToBitstream(CPLDSerializer* writer)
{
	//Write header
	writer->AddBodyBlankLine();
	writer->AddBodyComment("-------------------------------------------------------");
	char sblocknum[32];
	snprintf(sblocknum, sizeof(sblocknum), "Function block %2d", m_blocknum);
	writer->AddBodyComment(string("-                  ") + sblocknum + "                  -");
	writer->AddBodyComment("-------------------------------------------------------");
	writer->AddBodyBlankLine();
	
	//Write ZIA
	writer->AddBodyComment("ZIA");
	for(int i=0; i<INPUT_TERM_COUNT; i++)
		writer->AddBodyFuseData(m_zia[i], m_ziaWidth);

	//Write AND array
	writer->AddBodyBlankLine();
	writer->AddBodyComment("PLA AND array");
	for(int i=0; i<PLA_PRODUCT_TERMS; i++)
		writer->AddBodyFuseData(m_plaAnd[i], PLA_INPUT_WIDTH);
		
	//Write OR array
	writer->AddBodyBlankLine();
	writer->AddBodyComment("PLA OR array");
	for(int j=0; j<PLA_PRODUCT_TERMS; j++)
		writer->AddBodyFuseData(m_plaOr[j], MACROCELLS_PER_FB);
		
	//Write macrocells
	writer->AddBodyBlankLine();
	writer->AddBodyComment("Macrocell configuration");
	writer->AddBodyComment("Aclk ClkOp Clk:2 ClkFreq R:2 P:2 RegMod:2 INz:2 FB:2 InReg St XorIn:2 RegCom Oe:4 Tm Slw Pu");
	for(size_t i=0; i<m_macrocells.size(); i++)
		m_macrocells[i]->SaveToBitstream(writer);
}

/**
	@brief Fitting logic
 */
void FCCoolRunnerIIFunctionBlock::Fit()
{
	printf("Function block fitting (%d)\n", m_blocknum);
	
	//Clear the ZIA
	for(int i=0; i<INPUT_TERM_COUNT; i++)
		for(int j=0; j<m_ziaWidth; j++)
			m_zia[i][j] = true;
			
	//Clear the PLA AND array
	for(int i=0; i<PLA_PRODUCT_TERMS; i++)
		for(int j=0; j<PLA_INPUT_WIDTH; j++)
			m_plaAnd[i][j] = true;
			
	//Clear the PLA OR array
	for(int j=0; j<PLA_PRODUCT_TERMS; j++)
		for(int i=0; i<MACROCELLS_PER_FB; i++)
			m_plaOr[j][i] = true;	
	
	//Assign ZIA rows to nets
	printf("    Global routing\n");
	map<FCCoolRunnerIINetSource*, FCCoolRunnerIIZIAEntry> net_assignments;
	GlobalRouting(net_assignments);
	
	//Product term fitting
	printf("    Product term fitting\n");
	map<FCCoolRunnerIIProductTerm*, int> pterm_assignments;
	FitProductTerms(pterm_assignments);
	
	//We should be fully constrained at this point
	//Generate the PLA AND array
	printf("    Generating PLA AND array bits\n");
	for(auto it = pterm_assignments.begin(); it!=pterm_assignments.end(); it++)
	{
		FCCoolRunnerIIProductTerm* pterm = it->first;
		int column = it->second;
		
		for(size_t i=0; i<pterm->m_inputs.size(); i++)
		{
			FCCoolRunnerIINetSource* net = pterm->m_inputs[i];
			int ziarow = net_assignments[net].m_row;
			
			if(pterm->m_inputInvert[i])
				m_plaAnd[column][ziarow*2 + 1] = false;
			else
				m_plaAnd[column][ziarow*2] = false;
		}
	}
	
	//Generate the PLA OR array
	printf("    Generating PLA OR array bits\n");
	for(int i=0; i<MACROCELLS_PER_FB; i++)
	{
		//Skip macrocells not used in the netlist
		FCCoolRunnerIINetlistMacrocell* ncell = m_macrocells[i]->m_netlistcell;
		if(ncell == NULL)
			continue;
		FCCoolRunnerIIOrTerm* orterm = ncell->GetOrTerm();
		for(size_t j=0; j<orterm->m_pterms.size(); j++)
		{
			int npterm = pterm_assignments[orterm->m_pterms[j]];
			m_plaOr[npterm][i] = false;
		}
	}
}

void FCCoolRunnerIIFunctionBlock::GlobalRouting(map<FCCoolRunnerIINetSource*, FCCoolRunnerIIZIAEntry>& net_assignments)
{
	//See what nets are used by this FB
	unordered_set<FCCoolRunnerIINetSource*> nets;
	for(int i=0; i<MACROCELLS_PER_FB; i++)
	{
		FCCoolRunnerIINetlistMacrocell* mcell = m_macrocells[i]->m_netlistcell;
		if(mcell == NULL)
			continue;
		FCCoolRunnerIIOrTerm* orterm = mcell->GetOrTerm();
		
		//TODO: support product term C
		if( (mcell->m_xorin == FCCoolRunnerIIMacrocell::XORIN_PTC) || (mcell->m_xorin == FCCoolRunnerIIMacrocell::XORIN_NPTC) )
		{
			throw JtagExceptionWrapper(
				"Product term C not supported yet",
				"",
				JtagException::EXCEPTION_TYPE_UNIMPLEMENTED);
		}
		
		for(size_t j=0; j<orterm->m_pterms.size(); j++)
		{
			FCCoolRunnerIIProductTerm* pterm = orterm->m_pterms[j];
			for(size_t k=0; k<pterm->m_inputs.size(); k++)
				nets.emplace(pterm->m_inputs[k]);
		}
	}
	printf("        %zu net(s) assigned to function block inputs\n", nets.size());
	
	//DRC - input size has to be sane
	if(nets.size() > INPUT_TERM_COUNT)
	{
		throw JtagExceptionWrapper(
			"DRC error: Too many nets routed to this function block",
			"",
			JtagException::EXCEPTION_TYPE_GIGO);
	}
	
	//We have all of the nets
	//Figure out which one should go where
	list<FCCoolRunnerIINetSource*> row_assignments[INPUT_TERM_COUNT];		//Oversubscription permitted initially
	/*
		Pass 1: Greedy assignment
		Put each net in the first slot, allowing conflicts
	 */
	printf("        Pass 1: Greedy assignment  ");
	int unrouted = 0;
	for(auto it = nets.begin(); it != nets.end(); it++)
	{
		FCCoolRunnerIINetSource* net = *it;
		auto entries = m_device->GetZIAEntriesForNetSource(net);
		
		FCCoolRunnerIIZIAEntry entry = entries[0];
		net_assignments[net] = entry;
		if(row_assignments[entry.m_row].size())
			unrouted++;
		row_assignments[entry.m_row].push_back(net);
	}
	printf("        %d unrouted\n", unrouted);
	
	//TODO: Conflict resolution
	if(unrouted)
	{
		throw JtagExceptionWrapper(
			"Conflict resolution not yet implemented",
			"",
			JtagException::EXCEPTION_TYPE_UNIMPLEMENTED);
	}
	
	//The ZIA is now fully assigned, generate output
	printf("        Generating ZIA bits\n");
	for(int i=0; i<40; i++)
	{
		if(row_assignments[i].size() == 0)
			continue;
			
		//should never be >1, just use the first
		FCCoolRunnerIINetSource* net = *row_assignments[i].begin();
		
		//Assign the nets
		FCCoolRunnerIIZIAEntry entry = net_assignments[net];
		for(int j=0; j<m_ziaWidth; j++)
			m_zia[i][j] = !entry.m_muxsel[j];
	}
}

void FCCoolRunnerIIFunctionBlock::FitProductTerms(map<FCCoolRunnerIIProductTerm*, int>& pterm_assignments)
{
	//See what pterms are used by this FB
	unordered_set<FCCoolRunnerIIProductTerm*> pterms;
	for(int i=0; i<MACROCELLS_PER_FB; i++)
	{
		FCCoolRunnerIINetlistMacrocell* mcell = m_macrocells[i]->m_netlistcell;
		if(mcell == NULL)
			continue;
		FCCoolRunnerIIOrTerm* orterm = mcell->GetOrTerm();
		
		//TODO: support product term C
		if( (mcell->m_xorin == FCCoolRunnerIIMacrocell::XORIN_PTC) || (mcell->m_xorin == FCCoolRunnerIIMacrocell::XORIN_NPTC) )
		{
			throw JtagExceptionWrapper(
				"Product term C not supported yet",
				"",
				JtagException::EXCEPTION_TYPE_UNIMPLEMENTED);
		}
		
		for(size_t j=0; j<orterm->m_pterms.size(); j++)
			pterms.emplace(orterm->m_pterms[j]);
	}
	//TODO: Global pterms (function block clock, etc)
	if(pterms.size() > PLA_PRODUCT_TERMS)
	{
		throw JtagExceptionWrapper(
			"DRC error: Too many product terms used",
			"",
			JtagException::EXCEPTION_TYPE_GIGO);
	}
	printf("        %zu pterms used\n", pterms.size());
	
	bool slot_used[PLA_PRODUCT_TERMS] = {0};
	int slot_ptr = 0;
		
	//Constrained assignment
	printf("        Pass 1: Constrained assignment     ");
	//TODO: assign and remove from unassigned list
	printf("%zu unassigned\n", pterms.size());
	
	//Unconstrained assignment - fill in the first legal slot
	printf("        Pass 2: Unconstrained assignment   ");
	for(auto it=pterms.begin(); it!=pterms.end(); it++)
	{
		//Find the next free slot (guaranteed to be one since we checked earlier)
		for(; slot_ptr < PLA_PRODUCT_TERMS; slot_ptr++)
		{
			if(!slot_used[slot_ptr])
				break;
		}
		
		//Assign it
		pterm_assignments[*it] = slot_ptr;
		slot_used[slot_ptr] = true;
		slot_ptr++;
	}
	printf("0 unassigned\n");
}
