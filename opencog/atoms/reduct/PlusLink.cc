/*
 * opencog/atoms/reduct/PlusLink.cc
 *
 * Copyright (C) 2015 Linas Vepstas
 * All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License v3 as
 * published by the Plus Software Foundation and including the exceptions
 * at http://opencog.org/wiki/Licenses
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program; if not, write to:
 * Plus Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <opencog/atomspace/atom_types.h>
#include <opencog/atomspace/ClassServer.h>
#include <opencog/atoms/NumberNode.h>
#include "PlusLink.h"
#include "TimesLink.h"

using namespace opencog;

PlusLink::PlusLink(const HandleSeq& oset,
                   TruthValuePtr tv,
                   AttentionValuePtr av)
    : ArithmeticLink(PLUS_LINK, oset, tv, av)
{
	init();
}

PlusLink::PlusLink(Type t, const HandleSeq& oset,
                   TruthValuePtr tv,
                   AttentionValuePtr av)
    : ArithmeticLink(t, oset, tv, av)
{
	if (not classserver().isA(t, PLUS_LINK))
		throw InvalidParamException(TRACE_INFO, "Expecting a PlusLink");
	init();
}

PlusLink::PlusLink(const Handle& a, const Handle& b,
                   TruthValuePtr tv,
                   AttentionValuePtr av)
    : ArithmeticLink(PLUS_LINK, a, b, tv, av)
{
	init();
}

PlusLink::PlusLink(Type t, const Handle& a, const Handle& b,
                   TruthValuePtr tv,
                   AttentionValuePtr av)
    : ArithmeticLink(t, a, b, tv, av)
{
	if (not classserver().isA(t, PLUS_LINK))
		throw InvalidParamException(TRACE_INFO, "Expecting a PlusLink");
	init();
}

PlusLink::PlusLink(Link& l)
    : ArithmeticLink(l)
{
	Type tscope = l.getType();
	if (not classserver().isA(tscope, PLUS_LINK))
		throw InvalidParamException(TRACE_INFO, "Expecting a PlusLink");
	init();
}

static double plus(double a, double b) { return a+b; }

static inline double get_double(const Handle& h)
{
	NumberNodePtr nnn(NumberNodeCast(h));
	if (NULL == nnn)
		nnn = createNumberNode(*NodeCast(h));

	return nnn->getValue();
}

static Handle hplus(const Handle& fi, const Handle& fj)
{
	// Are they numbers?
	if (NUMBER_NODE == fi->getType() and
	    NUMBER_NODE == fj->getType())
	{
		double sum = get_double(fi) + get_double(fj);
		return Handle(createNumberNode(sum));
	}

	// Is fi identical to fj? If so, then replace by 2*fi
	if (fi == fj)
	{
		Handle two(createNumberNode("2"));
		return Handle(createTimesLink(fi, two));
	}

	// If j is (TimesLink x a) and i is identical to x,
	// then create (TimesLink x (a+1))
	//
	// If j is (TimesLink x a) and i is (TimesLink x b)
	// then create (TimesLink x (a+b))
	//
	if (fj->getType() == TIMES_LINK)
	{
		bool do_add = false;
		HandleSeq rest;

		LinkPtr ilp = LinkCast(fi);
		LinkPtr jlp = LinkCast(fj);
		Handle exx = jlp->getOutgoingAtom(0);

		// Handle the (a+1) case described above.
		if (fi == exx)
		{
			Handle one(createNumberNode("1"));
			rest.push_back(one);
			do_add = true;
		}

		// Handle the (a+b) case described above.
		else if (ilp->getOutgoingAtom(0) == exx)
		{
			const HandleSeq& ilpo = ilp->getOutgoingSet();
			size_t ilpsz = ilpo.size();
			for (size_t k=1; k<ilpsz; k++)
				rest.push_back(ilpo[k]);
			do_add = true;
		}

		if (do_add)
		{
			const HandleSeq& jlpo = jlp->getOutgoingSet();
			size_t jlpsz = jlpo.size();
			for (size_t k=1; k<jlpsz; k++)
				rest.push_back(jlpo[k]);

			// a_plus is now (a+1) or (a+b) as described above.
			PlusLinkPtr ap = createPlusLink(rest);
			Handle a_plus(ap->reduce());

			return Handle(createTimesLink(exx, a_plus));
		}
	}

	// If we are here, we've been asked to add two things of the same
	// type, but they are not of a type that we know how to add.
	// For example, fi anf fj might be two different VariableNodes.
	return Handle(createPlusLink(fi, fj));
}

void PlusLink::init(void)
{
	knild = 0.0;
	konsd = plus;
	kons = hplus;
}

// ============================================================

/// re-order the contents of a PlusLink into "lexicographic" order.
///
/// The goal of the re-ordering is to simplify the reduction code,
/// by placing atoms where they are easily found.  For now, this
/// means:
/// first, all of the variables,
/// next, all compound expressions,
/// last, all number nodes (of which there should be only zero or one.)
/// We do not currently sort the variables, but maybe we should...?
/// The FoldLink::reduce() method already returns expressions that are
/// almost in the correct order.
Handle PlusLink::reorder(void)
{
	HandleSeq vars;
	HandleSeq exprs;
	HandleSeq numbers;

	for (const Handle& h : _outgoing)
	{
		if (h->getType() == VARIABLE_NODE)
			vars.push_back(h);
		else if (h->getType() == NUMBER_NODE)
			numbers.push_back(h);
		else
			exprs.push_back(h);
	}

	if (1 < numbers.size())
		throw RuntimeException(TRACE_INFO,
		      "Expecting the plus link to have already been reduced!");

	HandleSeq result;
	for (const Handle& h : vars) result.push_back(h);
	for (const Handle& h : exprs) result.push_back(h);
	for (const Handle& h : numbers) result.push_back(h);

	return Handle(createPlusLink(result));
}

// ============================================================
