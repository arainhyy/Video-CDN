#!/usr/bin/env python

import os
import unittest
from grader_super import Project3Test, emit_scores

TEST_VALUES = {
    'test_proxy_simple': 9,
    'test_proxy_adaptation': 9,
    'test_writeup_exists': 3,
    'test_dns_simple': 8,
    'test_dns_rr': 9,
    'test_dns_lsa_topo1': 3,
    'test_dns_integration': 9
}

TEST_CATEGORIES = {
    'test_proxy_simple': 'proxy',
    'test_proxy_adaptation': 'proxy',
    'test_writeup_exists': 'writeup',
    'test_dns_simple': 'dns',
    'test_dns_rr': 'dns',
    'test_dns_lsa_topo1': 'dns',
    'test_dns_integration': 'dns'
}

class Project3Checkpoint2Test(Project3Test):

    ########### SETUP/TEARDOWN ##########

    # Run once per test suite
    @classmethod
    def setUpClass(cls):
        super(Project3Checkpoint2Test, cls).setUpClass()

    # Run once per test suite
    @classmethod
    def tearDownClass(cls):
        super(Project3Checkpoint2Test, cls).tearDownClass()

    # Run once per test
    def setUp(self):
        super(Project3Checkpoint2Test, self).setUp()

    # Run once per test
    def tearDown(self):
        super(Project3Checkpoint2Test, self).tearDown()


if __name__ == '__main__':
    suite = unittest.TestSuite()
    suite.addTest(Project3Checkpoint2Test('test_proxy_simple', './topos/one-client'))
    suite.addTest(Project3Checkpoint2Test('test_proxy_adaptation', './topos/one-client'))
    suite.addTest(Project3Checkpoint2Test('test_writeup_exists'))
    suite.addTest(Project3Checkpoint2Test('test_dns_simple', './topos/simple-dns'))
    suite.addTest(Project3Checkpoint2Test('test_dns_rr', './topos/rr-dns'))
    suite.addTest(Project3Checkpoint2Test('test_dns_lsa_topo1', './topos/topo1'))
    suite.addTest(Project3Checkpoint2Test('test_dns_integration', './topos/topo3'))
    results = unittest.TextTestRunner(verbosity=2).run(suite)

    emit_scores(results, TEST_VALUES, TEST_CATEGORIES)
