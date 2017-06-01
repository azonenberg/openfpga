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

#include "gp4tchar.h"
#include "solver.h"

using namespace std;

void PrintMatrix(float** rows, int numVars, int numEq);

bool KnapsackProblem::Solve()
{
	//Create mapping of variables to column IDs
	map<KnapsackVariable*, int> colmap;
	map<int, string> nmap;
	int numVars = 0;
	for(auto e : m_equations)
	{
		for(auto it : e->m_variables)
		{
			auto v = it.first;

			//already have it
			if(colmap.find(v) != colmap.end())
				continue;

			//nope, allocate new ID
			nmap[numVars] = v->m_name;
			colmap[v] = numVars;
			numVars ++;

			//LogNotice("Variable %d = %s\n", colmap[v], v->m_name.c_str());
		}
	}

	//Sanity check
	int numEq = m_equations.size();
	if(numEq < numVars)
	{
		LogError("Cannot solve knapsack problem: more variables than equations\n");
		return false;
	}

	//Create the matrix
	//Column numVars is the sum
	float** rows = new float*[numEq];
	int j = 0;
	for(auto e : m_equations)
	{
		//Clear coefficients to zero in case some are missing
		float* row = new float[numVars + 1];
		for(int i=0; i<numVars; i++)
			row[i] = 0;

		//Add variables (coefficient of 1 to start)
		for(auto it : e->m_variables)
			row[colmap[it.first]] = it.second;

		//Patch in the sum
		row[numVars] = e->m_sum;

		rows[j++] = row;
	}

	//Print out the matrix
	//LogNotice("Solving system of %d equations in %d unknowns\n", numEq, numVars);
	//PrintMatrix(rows, numEq, numVars);

	//Do the actual Gaussian elimination to produce an upper triangle matrix
	float epsilon = 0.000001f;
	for(int nvar = 0; nvar < numVars; nvar ++)
	{
		//If we have a 0 at our diagonal location, look down until we find a row that has a nonzero value
		if(rows[nvar][nvar] < epsilon)
		{
			bool found = false;
			for(int row=nvar+1; row<numEq; row ++)
			{
				if(rows[row][nvar] > epsilon)
				{
					//LogNotice("Swapping rows %d and %d\n", row, nvar);
					float* tmp = rows[nvar];
					rows[nvar] = rows[row];
					rows[row] = tmp;
					found = true;
					break;
				}
			}

			if(!found)
			{
				LogError("Cannot solve for variable %d (%s)\n", nvar, nmap[nvar].c_str());
				return false;
			}
		}

		//If we don't have a 1 at our corner cell, divide everything by that
		if( fabs(rows[nvar][nvar] - 1) > epsilon )
		{
			float scale = 1.0f / rows[nvar][nvar];
			//LogNotice("Multiplying row %d by %.3f\n", nvar, scale);
			for(int i=0; i<= numVars; i++)
				rows[nvar][i] *= scale;
		}

		//Subtract us (multiplied, if necessary) from any rows below us
		//in order to produce an upper triangle matrix
		for(int row=nvar+1; row<numEq; row++)
		{
			//It's already zero, skip it
			float rowval = rows[row][nvar];
			if(fabs(rowval) < epsilon)
				continue;

			//Multiply us by that value, then subtract
			//LogNotice("Subtracting %.3f * row %d from row %d\n", rowval, nvar, row);
			for(int i=0; i <= numVars; i++)
				rows[row][i] -= rows[nvar][i] * rowval;
		}

		//LogNotice("Variable %d done\n", nvar);
		//PrintMatrix(rows, numVars, numEq);
	}

	//Back-substitute to solve the remaining variables
	//Last row should already be in (x = y) form, so start one row above that
	//LogNotice("Back-substitution\n");
	for(int row = numVars - 2; row >= 0; row --)
	{
		//Remove all of the variables right of the diagonal
		for(int nvar = row + 1; nvar < numVars; nvar ++)
		{
			float rowval = rows[row][nvar];
			if(fabs(rowval) > epsilon)
			{
				//LogNotice("Subtracting %.3f * row %d from row %d\n", rowval, nvar, row);
				for(int i=0; i <= numVars; i++)
					rows[row][i] -= rows[nvar][i] * rowval;
			}
		}

		//LogNotice("Variable %d done\n", row);
		//PrintMatrix(rows, numVars, numEq);
	}

	//Print out final state
	//LogNotice("Final state\n");
	//PrintMatrix(rows, numVars, numEq);

	//Export matrix to the rows
	for(auto it : colmap)
		it.first->m_value = rows[it.second][numVars];

	//clean up
	for(int i=0; i<numEq; i++)
		delete[] rows[i];
	delete[] rows;

	return true;
}

void PrintMatrix(float** rows, int numVars, int numEq)
{
	for(int row=0; row<numEq; row++)
	{
		LogNotice("        ");
		for(int col=0; col <= numVars; col++)
		{
			if(col == numVars)
				LogNotice(" | ");
			LogNotice("%6.3f ", rows[row][col]);
		}
		LogNotice("\n");
	}
}
