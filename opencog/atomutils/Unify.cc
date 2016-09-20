/**
 * Unify.cc
 *
 * Utilities for unifying atoms.
 *
 * Copyright (C) 2016 OpenCog Foundation
 * All Rights Reserved
 * Author: Nil Geisweiller
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License v3 as
 * published by the Free Software Foundation and including the exceptions
 * at http://opencog.org/wiki/Licenses
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program; if not, write to:
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Created by Linas Vepstas February 2008
 */

#include "Unify.h"

#include <opencog/util/algorithm.h>
#include <opencog/atoms/base/Atom.h>
#include <opencog/atomutils/FindUtils.h>

namespace opencog {

UnificationSolutionSet::UnificationSolutionSet(bool s,
                                               const UnificationPartitions& p)
	: satisfiable(s), partitions(p)
{
}

UnificationSolutionSet unify(const Handle& lhs, const Handle& rhs,
                             const Handle& lhs_vardecl,
                             const Handle& rhs_vardecl)
{
	// Make sure both handles are defined
	if (lhs == Handle::UNDEFINED or rhs == Handle::UNDEFINED)
		return UnificationSolutionSet(false);

	Type lhs_type(lhs->getType());
	Type rhs_type(rhs->getType());

	// Base cases
	if (lhs->isNode() or rhs->isNode()) {
		if (lhs_type == VARIABLE_NODE or rhs_type == VARIABLE_NODE) {
			return mkvarsol(lhs, rhs, lhs_vardecl, rhs_vardecl);
		} else
			return UnificationSolutionSet(lhs == rhs);
	}

	// At least one of them is a link, check if they have the same
	// type (e.i. do they match so far)
	if (lhs_type != rhs_type)
		return UnificationSolutionSet(false);

	// At this point they are both links, check that they have the
	// same arity
	Arity lhs_arity(lhs->getArity());
	Arity rhs_arity(rhs->getArity());
	if (lhs_arity != rhs_arity)
		return UnificationSolutionSet(false);

	// Recursive cases
	UnificationSolutionSet sol;
	for (Arity i = 0; i < lhs_arity; ++i) {
		auto rs = unify(lhs->getOutgoingAtom(i), rhs->getOutgoingAtom(i),
		                lhs_vardecl, rhs_vardecl);
		sol = merge(sol, rs);
		if (not sol.satisfiable)     // Stop if unification has failed
			break;
	}
	return sol;
}

UnificationSolutionSet mkvarsol(const Handle& lhs, const Handle& rhs,
                                const Handle& lhs_vardecl,
                                const Handle& rhs_vardecl)
{
	Handle inter = type_intersection(lhs, rhs, lhs_vardecl, rhs_vardecl);
	if (inter == Handle::UNDEFINED)
		return UnificationSolutionSet(false);
	else {
		OrderedHandleSet hset{lhs, rhs};
		UnificationPartitions par{{{hset, inter}}};
		return UnificationSolutionSet(true, par);
	}
}

UnificationSolutionSet merge(const UnificationSolutionSet& lhs,
                             const UnificationSolutionSet& rhs)
{
	// No need to merge if one of them is invalid
	if (not lhs.satisfiable or not rhs.satisfiable)
		return UnificationSolutionSet(false);

	// No need to merge if one of them is empty
	if (rhs.partitions.empty())
		return lhs;
	if (lhs.partitions.empty())
		return rhs;

	// Merge
	UnificationSolutionSet result;
	for (const UnificationPartition& rp : rhs.partitions) {
		UnificationPartitions sol(merge(lhs.partitions, rp));
		result.partitions.insert(sol.begin(), sol.end());
	}

	// If we get an empty merge whereas the inputs where not empty
	// then the merge has failed
	result.satisfiable = (not result.partitions.empty()) or
		(lhs.partitions.empty() and rhs.partitions.empty());

	return result;
}

UnificationPartitions merge(const UnificationPartitions& lhs,
                            const UnificationPartition& rhs)
{
	UnificationPartitions result;
	if (lhs.empty())
		result.insert(rhs);
	else {
		for (const auto& par : lhs) {
			result.insert(merge(par, rhs));
		}
	}
	return result;
}

UnificationPartition merge(const UnificationPartition& lhs,
                           const UnificationPartition& rhs)
{
	// Don't bother merging if one of them is empty
	if (lhs.empty())
		return rhs;
	if (rhs.empty())
		return lhs;

	// Do the actual merging
	UnificationPartition result(lhs);
	for (const auto& typed_block : rhs) {
		for (auto it = lhs.begin(); it != lhs.end(); ++it) {
			if (has_empty_intersection(typed_block.first, it->first)) {
				// Merely insert this independent block
				result.insert(typed_block);
			} else {
				// Merge the 2 equality related blocks
				UnificationBlock m_typed_block = merge(typed_block, *it);
				// If the resulting block is valid then replace *it
				if (is_valid(m_typed_block)) {
					result.erase(it);
					result.insert(m_typed_block);
				}
				// If the resulting block is invalid then the partition is
				// invalid as well, thus an empty partition is returned
				else {
					return UnificationPartition();
				}
			}
		}
	}
	return result;
}

UnificationBlock merge(const UnificationBlock& lhs, const UnificationBlock& rhs)
{
	return {set_union(lhs.first, rhs.first),
			type_intersection(lhs.second, rhs.second)};
}

bool is_valid(const UnificationBlock& block)
{
	return block.second == Handle::UNDEFINED;
}

// TODO: very limited type intersection, should support structural
// types, etc.
Handle type_intersection(const Handle& lhs, const Handle& rhs,
                         const Handle& lhs_vardecl, const Handle& rhs_vardecl)
{
	if (inherit(lhs, rhs, lhs_vardecl, rhs_vardecl))
		return lhs;
	if (inherit(rhs, lhs, rhs_vardecl, lhs_vardecl))
		return rhs;
	return Handle::UNDEFINED;
}

Type type_intersection(Type lhs, Type rhs)
{
	ClassServer& cs = classserver();
	if (cs.isA(lhs, rhs))
		return lhs;
	if (cs.isA(rhs, lhs))
		return rhs;
	return NOTYPE;              // represent the bottom type
}

std::set<Type> type_intersection(Type lhs, const std::set<Type>& rhs)
{
	std::set<Type> res;
	// Distribute the intersection over the union type rhs
	for (Type rhst : rhs) {
		Type ty = type_intersection(lhs, rhst);
		if (ty != NOTYPE)
			res.insert(ty);
	}
	return res;
}

std::set<Type> type_intersection(const std::set<Type>& lhs,
                                 const std::set<Type>& rhs)
{
	// Base cases
	if (lhs.empty())
		return rhs;
	if (rhs.empty())
		return lhs;

	// Recursive cases
	std::set<Type> res;
	for (Type ty : lhs) {
		std::set<Type> itr = type_intersection(ty, rhs);
		res.insert(itr.begin(), itr.end());
	}
	return res;
}

std::set<Type> simplify_type_union(std::set<Type>& type)
{
	return {}; // TODO: do we really need that?
}

std::set<Type> get_union_type(const Handle& h, const Handle& vardecl)
{
	VariableListPtr vardecl_vlp(gen_varlist(h, vardecl));
	const Variables& variables(vardecl_vlp->get_variables());
	const VariableTypeMap& vtm = variables._simple_typemap;
	auto it = vtm.find(h);
	if (it == vtm.end() or it->second.empty())
		return {ATOM};
	else {
		return it->second;
	}
}

bool inherit(const Handle& lhs, const Handle& rhs,
             const Handle& lhs_vardecl, const Handle& rhs_vardecl)
{
	if (VARIABLE_NODE == lhs->getType() and VARIABLE_NODE == rhs->getType())
		return inherit(get_union_type(lhs, lhs_vardecl),
		               get_union_type(rhs, rhs_vardecl));
	// It that necessary?
	// else if (lhs == rhs)
	// 	return true;
	else
		return gen_varlist(rhs, rhs_vardecl)->is_type(rhs, lhs);
}

bool inherit(Type lhs, Type rhs)
{
	return classserver().isA(lhs, rhs);
}

bool inherit(Type lhs, const std::set<Type>& rhs)
{
	for (Type ty : rhs)
		if (inherit(lhs, ty))
			return true;
	return false;
}

bool inherit(const std::set<Type>& lhs, const std::set<Type>& rhs)
{
	for (Type ty : lhs)
		if (not inherit(ty, rhs))
			return false;
	return true;
}

/**
 * Generate a VariableList of the free variables of a given atom h.
 */
VariableListPtr gen_varlist(const Handle& h)
{
	OrderedHandleSet vars = get_free_variables(h);
	return createVariableList(HandleSeq(vars.begin(), vars.end()));
}

/**
 * Given an atom h and its variable declaration vardecl, turn the
 * vardecl into a VariableList if not already, and if undefined,
 * generate a VariableList of the free variables of h.
 */
VariableListPtr gen_varlist(const Handle& h, const Handle& vardecl)
{
	if (vardecl == Handle::UNDEFINED)
		return gen_varlist(h);
	else {
		Type vardecl_t = vardecl->getType();
		if (vardecl_t == VARIABLE_LIST)
			return VariableListCast(vardecl);
		else {
			OC_ASSERT(vardecl_t == VARIABLE_NODE
			          or vardecl_t == TYPED_VARIABLE_LINK);
			return createVariableList(HandleSeq(1, vardecl));
		}
	}
}

std::string oc_to_string(const BoolHandleMapSetPair& bhmsp)
{
	std::stringstream ss;
	ss << "success: " << bhmsp.first << std::endl
	   << "mappings: " << oc_to_string(bhmsp.second);
	return ss.str();
}

std::string oc_to_string(const UnificationPartition& up)
{
	std::stringstream ss;
	ss << "size = " << up.size() << std::endl;
	int i = 0;
	for (const auto& p : up) {
		ss << "block[" << i << "]:" << std::endl << oc_to_string(p.first)
		   << "type[" << i << "]:" << std::endl << oc_to_string(p.second);
		i++;
	}
	return ss.str();
}

std::string oc_to_string(const UnificationPartitions& par)
{
	std::stringstream ss;
	ss << "size = " << par.size() << std::endl;
	int i = 0;
	for (const auto& el : par) {
		ss << "typed partition[" << i << "]:" << std::endl << oc_to_string(el);
		i++;
	}
	return ss.str();
}

std::string oc_to_string(const UnificationSolutionSet& sol)
{
	std::stringstream ss;
	ss << "satisfiable: " << sol.satisfiable << std::endl
	   << "partitions: " << oc_to_string(sol.partitions);
	return ss.str();
}

} // namespace opencog