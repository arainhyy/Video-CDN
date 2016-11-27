#!/usr/bin/python
from collections import defaultdict

#G[0] is a list of strings defining each node
#G[1] is a dictionary of nodes --> list of neighbors
def Dijkstra(Graph, source):
    dist = defaultdict(lambda: -1)
    visited = defaultdict(lambda: False)
    previous = defaultdict(lambda: -1)

    dist[source] = 0 #Distance from source to source
    Q = [source] #Start off with just the source node

    while Q: #The main loop
        u = -1
        # vertex in Q with smallest distance in dist and not visited
        for v in Q:
            if not visited[v] and (u not in dist or dist[v] < dist[u]): 
                # source node in the first case
                u = v
        Q.remove(u)
        visited[u] = True

        for v in Graph[1][u]: #each neighbor v of u:   
            alt = dist[u] + 1 #accumulate shortest dist from source
            if not visited[v] and (v not in dist or alt < dist[v]):
                dist[v] = alt #keep the shortest dist from src to v
                previous[v] = u
                Q.append(v) #Add unvisited v into the Q to be process
    return dist;

def getBestServer(client, servers, lsa_file):
    CURRENT_STATUS = {}
    lsas = open(lsa_file).read().split('\n')
    for lsa in lsas:
        if lsa == '': continue
        lsa = lsa.split(' ')
        lsa[2] = lsa[2].split(',')
        if lsa[0] not in CURRENT_STATUS or CURRENT_STATUS[lsa[0]][0] < lsa[1]:
            CURRENT_STATUS[lsa[0]] = lsa[1:]

    NODES = []
    NEIGHBORS = defaultdict(list)
    for k,v in CURRENT_STATUS.items():
        NODES.append(k)
        NEIGHBORS[k] = v[1]

    for n,v in NEIGHBORS.items():
        for b in v:
            if b not in NODES:
                NODES.append(b)

    for n in NODES:
        if n not in NEIGHBORS:
            for b,v in NEIGHBORS.items():
                if n in v:
                    NEIGHBORS[n].append(b)

    # clients = ['1.0.0.1','2.0.0.1','3.0.0.1']
    clients = [client]
    # servers = ['4.0.0.1','5.0.0.1','6.0.0.1']

    DISTS = {}
    for c in clients:
        DISTS[c] = Dijkstra([NODES,NEIGHBORS], c)

    BESTS = {}
    for c in clients:
        best = (0,0)
        for s in servers:
            if not best[0] or DISTS[c][s] < best[1]:
                best = (s, DISTS[c][s])
        BESTS[c] = best[0]

    print BESTS.items()
    return BESTS[client]
