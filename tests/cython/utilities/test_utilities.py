import os
import random
import tempfile

from unittest import TestCase

from opencog.type_constructors import *
from opencog.atomspace import AtomSpace
from opencog.utilities import initialize_opencog, finalize_opencog, load_file

__author__ = 'Curtis Faith'


class UtilitiesTest(TestCase):

    def setUp(self):
        self.atomspace = AtomSpace()
 
    def tearDown(self):
        del self.atomspace

    def test_initialize_finalize(self):
        initialize_opencog(self.atomspace)
        finalize_opencog()

    def test_fast_load(self):
        gen_atoms(self.atomspace)
        with tempfile.TemporaryDirectory() as tmpdirname:
            tmp_file = os.path.join(tmpdirname, 'tmp.scm')
            with open(tmp_file, 'wt') as f:
                for atom in self.atomspace:
                    f.write(str(atom))
                    f.write('\n')
            new_space = AtomSpace()
            load_file(tmp_file, new_space)
            self.assertTrue(len(new_space) == len(self.atomspace))


def gen_atoms(atomspace, num=100000):
    predicates = [atomspace.add_node(types.PredicateNode, 'predicate' + str(x)) for x in range(1)]
    concepts = [atomspace.add_node(types.ConceptNode, 'concept' + str(x)) for x in range(1000)]
    link_types = [types.ListLink, types.InheritanceLink, types.MemberLink]
    while(len(atomspace) < num):
        c1 = random.choice(concepts)
        c2 = random.choice(concepts)
        if c1 == c2:
            continue
        link_type = random.choice(link_types)
        arg = atomspace.add_link(link_type, [c1, c2])
        predicate = random.choice(predicates)
        atomspace.add_link(types.EvaluationLink,
                [predicate,
                arg])
    return atomspace

