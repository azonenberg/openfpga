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

#ifndef solver_h
#define solver_h

/*
	This file contains a solver for the "multiple knapsack problem".

	More formally, given a series of sums of the form a + b + c = 1234, where a/b/c are unknown variables that appear at
	most once in any given sum (and always with a coefficient of 1), solve for the individual variables.
 */

/**
	@brief A single named variable
 */
class KnapsackVariable
{
public:
	KnapsackVariable(std::string n)
	: m_name(n)
	, m_value(0)
	{ }

	std::string m_name;

	//Final value (only valid after solver finishes)
	float m_value;
};

/*
	@brief A single equation in the system of equations
 */
class KnapsackEquation
{
public:
	KnapsackEquation(float sum = 0)
	: m_sum(sum)
	{}

	void AddVariable(KnapsackVariable& v)
	{ m_variables.emplace(&v); }

	std::set<KnapsackVariable*> m_variables;
	float m_sum;
};

/**
	@brief A problem for the solver
 */
class KnapsackProblem
{
public:
	void AddEquation(KnapsackEquation& e)
	{ m_equations.emplace(&e); }

	bool Solve();

	std::set<KnapsackEquation*> m_equations;
};

#endif
