#!/usr/bin/python

import sys
import random
import argparse


# Return topo in dictionary:
#    KEY = node id
#    VALUE = list of neighbors
def read_links(links_file):
    links = {}
    with open(links_file, 'r') as f:
        for line in f:
            nodes = line.strip().split(' ')

            # if we haven't seen these nodes yet...
            if nodes[0] not in links:
                links[nodes[0]] = []
            if nodes[1] not in links:
                links[nodes[1]] = []

            # Add the edge
            links[nodes[0]].append(nodes[1])
            links[nodes[1]].append(nodes[0])
    f.closed
    return links


# Return a link state announcement as a string in the follwing format:
# <sender> <seqnum> <neighbors (CSV)>
def lsa_string(sender, seqnum, neighbors):
    return '%s %i %s' % (sender, seqnum, ','.join(neighbors))


# Generate a possible series of link state announcements as heard by sink_node.
# LSAs are presented in BFS order starting at the sink node to provide some 
# level of realism.
def generate_LSAs(links, sink_node, round_num):
    visited = set([sink_node])  # nodes we've visited already in BFS
    to_visit = list(links[sink_node])  # start with neighbors of sink
    while len(to_visit) > 0:
        current_node = to_visit.pop(0)
        visited.add(current_node)

        # add any neighbors of the current node to the queue
        # if we haven't already visited them
        for neighbor in links[current_node]:
            if neighbor not in visited and neighbor not in to_visit:
                to_visit.append(neighbor)

        # generate the current node's LSA (if it doesn't get "lost"!)
        if (random.random() > args.loss_rate):
            print lsa_string(current_node, round_num, links[current_node])


def main():
    random.seed()
    links = read_links(args.link_file)
    for i in range(0, args.rounds):
        generate_LSAs(links, args.sink_node, i)

if __name__ == "__main__":
    # set up command line args
    parser = argparse.ArgumentParser(description='Generate simulated link state advertisements.')
    parser.add_argument('link_file', help='the file containing the network links')
    parser.add_argument('sink_node', help='the node receiving the simulated LSAs')
    parser.add_argument('-l', '--loss-rate', type=float, default=0, help='the probability each LSA is lost')
    parser.add_argument('-n', '--rounds', type=int, default=10, help='the number of rounds of announcements to simulate')
    args = parser.parse_args()

    main()
