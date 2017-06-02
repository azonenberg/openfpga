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

/**
	@brief A single named variable
 */
class EquationVariable
{
public:
	EquationVariable(std::string n)
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
class Equation
{
public:
	Equation(float sum = 0)
	: m_sum(sum)
	{}

	void AddVariable(EquationVariable& v, float coeff = 1)
	{ m_variables[&v] = coeff; }

	std::map<EquationVariable*, float> m_variables;
	float m_sum;
};

/**
	@brief A system of linear equations of the form a*v1 + b*v2 + c*v3 = n
 */
class EquationSystem
{
public:
	void AddEquation(Equation& e)
	{ m_equations.emplace(&e); }

	bool Solve();

	std::set<Equation*> m_equations;
};

#endif
