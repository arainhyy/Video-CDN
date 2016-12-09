#!/usr/bin/env python

import sys
sys.path.append('../common')

import argparse
import hashlib
from util import check_output, check_both

TC='/sbin/tc'
DEFAULT_CLASS=9999
ROOT_Q_HANDLE=9999


# Return a consistent traffic class for the given pair of IP addresses
def class_for_ip_pair(ip_pair):
    # hash the IP pair to a traffic class number ("sort" them first so we 
    # always hash them in the same order). Valid class numbers are 1 - 9999,
    # but we don't allow 9999 since it's the default class
    if args.ip_pair[0] < args.ip_pair[1]:
        ip_pair_str = args.ip_pair[0] + args.ip_pair[1]
    else:
        ip_pair_str = args.ip_pair[1] + args.ip_pair[0]
    return (int(hashlib.sha1(ip_pair_str).hexdigest(), 16) % 9998) + 1


# Start traffic shaping on the specified interface by attaching a hierarchical
# token bucket to the interface (the "root" queue for that interface). We can
# then add individual classes to the "root" token bucket as needed.
def start():
    check_output('%s qdisc add dev %s root handle %i: htb default %i'\
        % (TC, args.interface, ROOT_Q_HANDLE, DEFAULT_CLASS))

    # make a default class for normal traffic
    check_output('%s class replace dev %s parent %i: classid %i:%i htb rate 1000mbit ceil 1000mbit'\
        % (TC, args.interface, ROOT_Q_HANDLE, ROOT_Q_HANDLE, DEFAULT_CLASS))



# Stop traffic shaping on the specified interface by removing the root queueing
# discipline on that interface (the token bucket we added in start())
def stop():
    out = check_both('%s qdisc del dev %s root' % (TC, args.interface), shouldPrint=False, check=False)
    if out[1] is not 0 and 'RTNETLINK answers: No such file or directory' not in out[0][0]:
        raise Exception("Error stopping traffic shaping")


# Update the traffic class associated with the pair of IP addresses specified
# as command line arguments
def update():
    # Figure out which traffic class we're updating
    if args.traffic_class:
        traffic_class = args.traffic_class
    elif args.ip_pair:
        traffic_class = class_for_ip_pair(args.ip_pair)
    else:
        traffic_class = DEFAULT_CLASS

    # Update the queues for the traffic class with the new BW/latency
    check_output('%s class replace dev %s parent %i: classid %i:%i htb rate %s ceil %s'\
        % (TC, args.interface, ROOT_Q_HANDLE, ROOT_Q_HANDLE, traffic_class,\
        args.bandwidth, args.bandwidth))
    check_output('%s qdisc replace dev %s parent %i:%i handle %i: netem delay %s'\
        % (TC, args.interface, ROOT_Q_HANDLE, traffic_class, traffic_class,\
        args.latency))

    # Update the rules mapping IP address pairs to the traffic class
    if args.ip_pair:
        U32='%s filter replace dev %s protocol ip parent %i: prio 1 u32'\
            % (TC, args.interface, ROOT_Q_HANDLE)
        check_output('%s match ip dst %s match ip src %s flowid %i:%i'
            % (U32, args.ip_pair[0], args.ip_pair[1], ROOT_Q_HANDLE, traffic_class))
        check_output('%s match ip dst %s match ip src %s flowid %i:%i'
            % (U32, args.ip_pair[1], args.ip_pair[0], ROOT_Q_HANDLE, traffic_class))


def show():
    print '=============== Queue Disciplines ==============='
    check_output('%s -s qdisc show dev %s' % (TC, args.interface))
    print '\n================ Traffic Classes ================'
    check_output('%s -s class show dev %s' % (TC, args.interface))
    print '\n==================== Filters ===================='
    check_output('%s -s filter show dev %s' % (TC, args.interface))


def main():
    if args.command == 'start':
        start()
    elif args.command == 'stop':
        stop()
    elif args.command == 'update':
        update()
    elif args.command == 'show':
        show()


if __name__ == "__main__":
    # set up command line args
    parser = argparse.ArgumentParser(description='Adjust traffic shaping settings')
    parser.add_argument('command', choices=['start','stop','show','update'], help='command: start or stop traffic shaping; show current filters; or update a filter')
    parser.add_argument('ip_pair', nargs='*', default=None, help='The pair of IP addresses between which the specified BW and latency should apply. If not provided, the class specified with -c is updated. If neither is provided, the default class is updated.')
    parser.add_argument('-i', '--interface', default='lo', help='the interface to adjust')
    parser.add_argument('-b', '--bandwidth', default='1000mbit', help='download bandwidth (e.g., 100mbit)')
    parser.add_argument('-l', '--latency', default='0ms', help='outbound latency (e.g., 20ms)')
    parser.add_argument('-c', '--traffic_class', type=int, default=0, help='traffic class number to update. If none provided, the hash of the IP pair is used. If no IP pair is provided, the default class is updated.')
    args = parser.parse_args()

    main()
