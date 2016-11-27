#!/usr/bin/python

import sys
sys.path.append('../common')
sys.path.append('../dns')

import os
import json
import unittest
import requests
import hashlib
import time
import random
from threading import Thread
from util import check_output, check_both, run_bg
from dns_common import sendDNSQuery

NETSIM = '../netsim/netsim.py'
VIDEO_SERVER_NAME = 'video.cs.cmu.edu'
PROXY = '../../proxy'
NAMESERVER = '../../nameserver'
WRITEUP = '../../writeup.pdf'
LARGE_FOLDER = '/var/www/vod/large'

class Project3Test(unittest.TestCase):

    def __init__(self, test_name, topo_dir=None):
        super(Project3Test, self).__init__(test_name)
        self.topo_dir = topo_dir
        self.exc_info = []

    ########### SETUP/TEARDOWN ##########

    # Run once per test suite
    @classmethod
    def setUpClass(cls):
        pass

    # Run once per test suite
    @classmethod
    def tearDownClass(cls):
        pass

    # Run once per test
    def setUp(self):
        check_both('killall -9 proxy', False, False)
        check_both('killall -9 nameserver', False, False)
        self.start_netsim()
        self.proxyport1 = random.randrange(1025, 60000)
        self.proxyport2 = random.randrange(1025, 60000)
        self.dnsport = random.randrange(1025, 60000)

    # Run once per test
    def tearDown(self):
        check_both('killall -9 proxy', False, False)
        check_both('killall -9 nameserver', False, False)
        self.stop_netsim()


    ########## HELPER FUNCTIONS ##########

    def run_proxy(self, log, alpha, listenport, fakeip, dnsip, dnsport, serverip=''):
        check_both('rm %s' % log, False, False)
        run_bg('%s %s %s %s %s %s %s %s'\
            % (PROXY, log, alpha, listenport, fakeip, dnsip, dnsport, serverip))

    def run_dns(self, rr, log, listenip, listenport, serverfile, lsafile):
        run_bg('%s %s %s %s %s %s %s'\
            % (NAMESERVER, rr, log, listenip, listenport, serverfile, lsafile))

    def run_events(self, events_file=None, bg=False):
        cmd = '%s %s run' % (NETSIM, self.topo_dir)
        if events_file:
            cmd += ' -e %s' % events_file
        if bg:
            run_bg(cmd)
        else:
            check_output(cmd)

    def start_netsim(self):
        if self.topo_dir:
            check_output('%s %s start' % (NETSIM, self.topo_dir))
    
    def stop_netsim(self):
        if self.topo_dir:
            check_output('%s %s stop' % (NETSIM, self.topo_dir))

    # Returns log entries as lists, one at a time. Use in for loop,
    # e.g., "for entry in iter_log(file_path):".
    def iter_log(self, log_file):
        with open(log_file, 'r') as logf:
            for line in logf:
                line = line.strip()
                if line:
                    yield line.split()
        logf.closed



    def check_gets(self, ip, port, num_gets, log_file, link_bw, expect_br, use=5, alpha=1.0, tput_margin=0.3, bitrate_margin=0.1, large=False):
        # TODO: better way to do this?
        if use == -1:
            use = 5

        if large:
            HASH_VALUE = {500: 'b1931364d7933ae90da7c6de423faf51b81503f4dfeb04da4be53dfb980c671e'}
        else:
            HASH_VALUE = {500: 'af29467f6793789954242d0430ce25e2fd2fc3a1aac5495ba7409ab853b1cdfa', 1000: 'f1ee215199d6c495388e2ac8470c83304e0fc642cb76fffd226bcd94089c7109'}
        

        # send a few gets (until we think their estimate should have stabilized)
        try: # this try is here so an exception will be thrown (and saved in self.exc_info) if we can't connect to proxy
            if large:
                r = requests.get('http://%s:%s/vod/large/big_buck_bunny.f4m' % (ip, port))
            else:
                r = requests.get('http://%s:%s/vod/big_buck_bunny.f4m' % (ip, port))

            for i in xrange(num_gets):
                if large:
                    r = requests.get('http://%s:%s/vod/large/1000Seg2-Frag3' %(ip, port))
                else:
                    r = requests.get('http://%s:%s/vod/1000Seg2-Frag7' %(ip, port))
            # check what bitrate they're requesting
            tputs = []
            tput_avgs = []
            bitrates = []
            for entry in self.iter_log(log_file):
                tputs.append(float(entry[2]))
                tput_avgs.append(float(entry[3]))
                bitrates.append(int(float(entry[4])))
            tputs = tputs[-use:]
            tput_avgs = tput_avgs[-use:]
            bitrates = bitrates[-use:]
            tput = float(sum(tputs))/len(tputs)
            tput_avg = float(sum(tput_avgs))/len(tput_avgs)
            bitrate = float(sum(bitrates))/len(bitrates)
            print tput, tput_avg, bitrate
        except Exception, e:
            self.exc_info = sys.exc_info()
            return

        print 'STATS: tput=%g, tput_avg=%g, bitrate=%g, expect_br=%g, link_bw=%g'\
            % (tput, tput_avg, bitrate, expect_br, link_bw)

        try: 
            self.assertTrue(abs(tput - link_bw) < tput_margin*link_bw)
            self.assertTrue(abs(tput_avg - link_bw) < (1.0/float(alpha))*tput_margin*link_bw)
            self.assertTrue(abs(bitrate - expect_br) < (1.0/float(alpha))*bitrate_margin*expect_br)

            # check the hash of the last chunk we requested
            chunkhash = hashlib.sha256(r.content).hexdigest()
            print 'Hash of last chunk: %s' % chunkhash
            self.assertTrue(chunkhash == HASH_VALUE[expect_br])
        except Exception, e:
            self.exc_info = sys.exc_info()

    def check_errors(self):
        if self.exc_info:
            raise self.exc_info[1], None, self.exc_info[2]


    def get_log_switch_len(self, log, num_trials, start_br, end_br):
        entries = [e for e in self.iter_log(log)]
        entries = entries[num_trials:]
        switch = 0
        for i,e in enumerate(entries):
            if float(e[4]) == end_br and switch == 0:
                switch = i
            if float(e[4]) == start_br:
                switch = 0
        return switch

    def print_log(self, log):
        print '\n\n#################### %s ####################' % log
        check_output('cat %s' % log)
        print '\n'



    ########### TEST CASES ##########

    def test_proxy_simple(self):
        PROXY_LOG = 'proxy.log'
        self.run_proxy(PROXY_LOG, '1', self.proxyport1, '1.0.0.1', '0.0.0.0', '0', '2.0.0.1')
        self.run_events(os.path.join(self.topo_dir, 'simple.events'))
        self.check_gets('1.0.0.1', self.proxyport1, 10, 'proxy.log', 900, 500)
        self.print_log(PROXY_LOG)
        self.check_errors()
        print 'done test_proxy_simple'
    
    def test_proxy_adaptation(self):
        PROXY_LOG = 'proxy.log'
        self.run_proxy(PROXY_LOG, '1', self.proxyport1, '1.0.0.1', '0.0.0.0', '0', '2.0.0.1')
        self.run_events(os.path.join(self.topo_dir, 'adaptation-2000.events')) 
        self.check_gets('1.0.0.1', self.proxyport1, 10, PROXY_LOG, 2000, 1000)
        self.run_events(os.path.join(self.topo_dir, 'adaptation-900.events')) 
        self.check_gets('1.0.0.1', self.proxyport1, 10, PROXY_LOG, 900, 500)
        self.print_log(PROXY_LOG)
        self.check_errors()
        print 'done test_proxy_adaptation'
    
    def test_proxy_multiple_clients(self):
        PROXY1_LOG = 'proxy1.log'
        PROXY2_LOG = 'proxy2.log'
        self.run_proxy(PROXY1_LOG, '1', self.proxyport1, '1.0.0.1', '0.0.0.0', '0', '3.0.0.1')
        self.run_proxy(PROXY2_LOG, '1', self.proxyport2, '2.0.0.1', '0.0.0.0', '0', '3.0.0.1')
        self.run_events(os.path.join(self.topo_dir, 'multiple.events'))
        large = True if os.path.isdir(LARGE_FOLDER) else False
        ts = []
        ts.append(Thread(target=self.check_gets, args= ('1.0.0.1', self.proxyport1, 10, PROXY1_LOG, 950, 500, 7, 1.0, 0.6, 5, large)))
        ts.append(Thread(target=self.check_gets, args= ('2.0.0.1', self.proxyport2, 10, PROXY2_LOG, 950, 500, 7, 1.0, 0.6, 5, large)))
        for t in ts:
            t.start()
        for t in ts:
            t.join()
        self.print_log(PROXY1_LOG)
        self.print_log(PROXY2_LOG)
        self.check_errors()
        print 'done test_proxy_multiple_clients'
    
def emit_scores(test_results, test_values, test_categories):

    # Initialization
    test_scores = {}
    category_scores = {}
    for test, value in test_values.iteritems():
        test_scores[test] = test_values[test]  # start w/ max; deduct later
        category_scores[test_categories[test]] = 0  # init category scores to 0

    # Deduct points for failed tests
    failed = test_results.failures + test_results.errors
    for testcase in failed:
        test = testcase[0].id().split('.')[-1]
        test_scores[test] = 0


    # Sum category scores
    for test, score in test_scores.iteritems():
        category_scores[test_categories[test]] += score

    print test_scores  # for student's log
    autolab_wrapper = { 'scores':category_scores }
    print json.dumps(autolab_wrapper)  # for autolab
