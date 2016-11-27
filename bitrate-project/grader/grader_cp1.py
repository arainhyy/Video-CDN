#!/usr/bin/env python

import os
import unittest
from grader_super import Project3Test, emit_scores

TEST_VALUES = {
    'test_proxy_simple': 2,
    'test_proxy_adaptation': 2,
    'test_proxy_multiple_clients': 2,
}

TEST_CATEGORIES = {
    'test_proxy_simple': 'proxy',
    'test_proxy_adaptation': 'proxy',
    'test_proxy_multiple_clients': 'proxy',
}

class Project3Checkpoint1Test(Project3Test):

    ########### SETUP/TEARDOWN ##########

    # Run once per test suite
    @classmethod
    def setUpClass(cls):
        super(Project3Checkpoint1Test, cls).setUpClass()

    # Run once per test suite
    @classmethod
    def tearDownClass(cls):
        super(Project3Checkpoint1Test, cls).tearDownClass()

    # Run once per test
    def setUp(self):
        super(Project3Checkpoint1Test, self).setUp()

    # Run once per test
    def tearDown(self):
        super(Project3Checkpoint1Test, self).tearDown()
    
if __name__ == '__main__':
    suite = unittest.TestSuite()
    suite.addTest(Project3Checkpoint1Test('test_proxy_simple', './topos/one-client'))
    suite.addTest(Project3Checkpoint1Test('test_proxy_adaptation', './topos/one-client'))
    suite.addTest(Project3Checkpoint1Test('test_proxy_multiple_clients', './topos/two-clients'))
    results = unittest.TextTestRunner(verbosity=2).run(suite)

    emit_scores(results, TEST_VALUES, TEST_CATEGORIES)
