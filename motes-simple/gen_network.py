#!/usr/bin/env python

"""
Usage
for vals in rand_graph(10, 4):
    print vals
"""

import random

DB_RANGE = (-60, -50)

def rand_graph(num, conn):
    "Creates a random graph with n elments"
    for x in range(0, num):
        for y in range(x+1, num):
            if random.random() * conn > 1:
                val = float(random.randrange(*DB_RANGE))
                yield(x, y, val)
                yield(y, x, val + random.randrange(3))

# FIXME: if MAX_NODES not multiple of 4 something strange could happen
def bin_tree(dim):
    "Generates a binary tree connection"
    for x in range((2 ** dim) - 1):
        yield (x, x * 2 + 1)
        yield (x, (x+1) * 2)


def network_to_hops_parents(topo):
    """
    Given a topology as a list of couples return the a dictionary
    with the minimal hops and the parent
    """
    parents, hops, grid = {}, {}, {}
    for x, y in topo:
        if x in grid:
            grid[x].append(y)
        else:
            grid[x] = []

        if y in grid:
            grid[y].append(x)
        else:
            grid[y] = []

    # we build the structure starting from node 0
    for x in grid:
        parents[x] = None
        hops[x] = 255

    idx = 0
    parents[idx] = 0
    hops[idx] = 0
    # just use a BFS algorithm here

def sim_txt_to_png(topo_file):
    topo = []
    for line in open(topo_file):
        vals = line.split()
        topo.append((int(vals[0]), int(vals[1])))

    topology_to_png(topo, topo_file.split(".")[0])

def topology_to_png(topology, filename):
    try:
        import pydot
    except ImportError:
        print "you need to install pydot for this"
        return
    
    p = pydot.Dot()
    for x, y in topology:
        p.add_edge(pydot.Edge(str(x), str(y)))
    
    f = filename + ".png"
    print "writing out to %s" % f
    p.write_png(f)
    

def to_dist_dict(nodes, topo):
    # they're simply all 1 in this case, when no connection use -1
    d = {}
    for n1 in nodes:
        for n2 in nodes:
            if n1 == n2:
                d[(n1,n2)] = 0
            elif ((n1, n2) in topo) or ((n2, n1) in topo):
                d[(n1,n2)] = d[(n2,n1)] = 1
            else:
                d[(n1,n2)] = d[(n2,n1)] = -1
    
    return d

# FIXME: check this algorithm is buggy in some situations
def floyd_warshall(cities, dist):
    dim = len(cities)
    old = new = {}
    # first cycle to set the initial configuration
    for c1 in cities:
        for c2 in cities:
            old[(c1, c2)] = [dist[(c1, c2)], [c2]]

    # ranging over the distance between nodes
    for k in range(1, dim):
        for c1 in cities:
            for c2 in cities:
                diretto = old[(c1, c2)]
                before = old[(c1, cities[k-1])]
                after = old[(cities[k-1], c2)]
                if diretto[0] <= (before[0] + after[0]):
                    new[(c1, c2)] = diretto
                else:
                    new[(c1, c2)] = [before[0] + after[0], before[1]+after[1]]
        old = new
    return new

def hops_parent(min_dists, nodes, root_node):
    "Get the ouput from floyd_warshall and compute the distance and the parent"
    hops = parents = {}
    # take always the before last with s[len(s)-2]
    for n in nodes:
        inf = min_dists[(n, root_node)]
        hops[n] = inf[0]
        parents[n] = inf[1][len(inf[1])-2]

    return hops, parents

# nodes = [1,2,3]
# dist_dict = to_dist_dict(nodes, [(1,2), (2,3)])
# print dist_dict
# fl = floyd_warshall(nodes, dist_dict)
# print fl
# print hops_parent(fl, nodes, 1)
