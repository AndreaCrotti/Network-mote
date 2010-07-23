#!/usr/bin/env python
"""
Usage:
Run normally "python simulation.py", wait that the motes are booted and then, pressing C-c it will ask interactively to build a packet and will send it over the serial channel
ow the instructions

We can also inspect variables while we run the program.
If this script doesn't work make sure you have
the correct TOSROOT variable set.

We can also do some sort of unit testing using variables.
"""

import sys
import os
import subprocess
import random
import readline
import rlcompleter
from re import match

from inter import MenuMaker
from packet import *

from TOSSIM import Tossim, SerialForwarder, Throttle
from tinyos.tossim.TossimApp import NescApp

from gen_network import bin_tree, rand_graph

TOSROOT = os.getenv("TOSROOT")
if TOSROOT is None:
    print "you need at least to setup your $TOSROOT variable correctly"
    sys.exit(1)

else:
    sdk = os.path.join(TOSROOT, "suppport", "sdk", "python")
    sys.path.append(sdk)

RUNTIME = 2

MAX_NODES = 2**10
SERIAL_PORT = 9001

# channels used for debug messages
# TODO: give different colors to the various debug messages

CHANNELS = ("Serial", "Boot", "Radio", "Routing", "Rel", "Sensor")

MODULES_REGEXP = "Blink.*|Easy.*|Rulti.*"

def print_var_table(vars):
    "print a filtered variable list"
    print "\nvariable list\n"
    for v in vars:
        if match(MODULES_REGEXP, v):
            print v
    print "\n"

class RadioNetwork(object):
    "Manage the network topology"
    def __init__(self, radio, symmetric=True):
        self.symmetric = symmetric
        self.radio = radio
        self.topology = set()

    def __len__(self):
        return len(self.topology)

    def __str__(self):
        return "\n".join("%d - %d" % (x[0], x[1]) for x in sorted(self.topology))

    def __iter__(self):
        return iter(self.topology)

    # Adding and removing from our local data strucure AND the radio topology
    def add_connection(self, node1, node2, link=-56.0):
        "Add a connection between two nodes"
        if self.symmetric:
            self.radio.add(node2, node1, link)
            self.topology.add((node2, node1))

        self.topology.add((node1, node2))
        self.radio.add(node1, node2, link)

    def remove_connection(self, node1, node2):
        if self.symmetric:
            self.radio.remove(node2, node1)
            self.topology.remove((node2, node1))

        self.topology.remove((node1, node2))
        self.radio.remove(node1, node2)

    def disconnect_node(self, node):
        "remove all the connections of node"
        asList = list(self.topology)
        for couple in asList:
            if node in couple:
                try:
                    self.remove_connection(*couple)
                except KeyError:
                    pass
    
    def topo_to_radio(self, topology, nodes):
        "Update the real radio world given the topology"
        from itertools import combinations
        # remove what is connected and should not be and viceversa
        for x, y in combinations(nodes, 2):
            # can be simply like that since adding something already there
            # or removing something not present doesn't create problems
            if (x,y) in self.topology:
                self.add_connection(x, y)
            else:
                self.remove_connection(x, y)                

    def connected(self, node1, node2):
        "Return True if the two nodes are connected"
        r = self.radio.connected(node1, node2)
        t = (node1, node2) in self.topology
        # just to check that we're working correctly on both
        assert(r == t)
        return r

# add to the local variables also the variables in the 
class Simulation(object):
    def __init__(self, port, channels, test=False):
        self.app = NescApp()
        # only take the variable names without
        self.vars = self.app.variables.variables()
        # I pass the variables to the simulator
        self.sim = Tossim(self.vars)
        self.nodes = {}
        # the serial forwarder stuff is needed only by the java interaface
        # or the listen.py script
        # in testing mode this is only a problem
        self.test = test
        if not(self.test):
            self.sf = SerialForwarder(port)

        self.throttle = Throttle(self.sim, 10)
        self.seqno = 0
        # operations on the topology and the radio channel
        self.topology = RadioNetwork(self.sim.radio())
        self.channels = list(channels)
        # adding all the channels
        for c in channels:
            self.add_channel(c)

    def single_node(self):
        print "adding a simple lonely node"
        self.add_node(0)
        
    def process(self):
        if not(self.test):
            self.sf.process()

    # handle separately colors and moreoveor
    def add_channel(self, channel):
        self.sim.addChannel(channel, sys.stdout)
        if not(channel in self.channels):
            self.channels.append(channel)

    def remove_channel(self, channel):
        self.sim.removeChannel(channel, sys.stdout)
        if channel in self.channels:
            self.channels.remove(channel)

    def add_node(self, idx):
        # otherwise add to the dictionary the correct node
        if not(idx in self.nodes):
            if len(self.nodes) == MAX_NODES:
                print "Not possible to add more nodes, reached limit"

            else:
                self.nodes[idx] = self.sim.getNode(idx)

    def start(self, batch=False):
        "Starts the simulation"
        for n in self.nodes.values():
            n.bootAtTime(random.randint(100001, 900009))

        self.process()
        self.throttle.initialize()

        # make sure they all boot
        self.run_some_events()
        if not(batch):
            self.cycle()

    def cycle(self):
        "Loop at infinite runnign all the events in the queue"
        print "start cycling, use C-c to send data interactively"
        while True:
            try:
                self.throttle.checkThrottle()
                self.sim.runNextEvent()
                self.process()
                # processing what it's got from it
            except KeyboardInterrupt:
                # with the first interrupt we go in interactive mode, the second quits the program
                try:
                    self.interactive()
                    print "\nrestarting the cycle, running other events...\n"
                    continue
                except KeyboardInterrupt:
                    sys.exit()

    def connected(self, node1, node2):
        return self.topology.connected(node1, node2)

    def make_topology(self, topo_file):
        "Creates the topology from the given file"
        for line in open(topo_file):
            vals = line.split()
            vals = (int(vals[0]), int(vals[1]), float(vals[2]))
            self.add_connection(*vals)

    def make_rand_graph(self):
        "Creates a random graph"
        for vals in rand_graph(MAX_NODES, 5):
            self.add_connection(*vals)

    def make_bin_tree(self, len):
        "Creates a binary tree structure for the topology"
        for vals in bin_tree(len):
            self.add_connection(*vals)
        print self.topology

    def make_given_topology(self, couples):
        for x, y in couples:
            self.add_connection(x, y)

    def setup_noise(self, noise_file):
        for line in open(noise_file):
            val = int(line.strip())
            for n in self.nodes.values():
                n.addNoiseTraceReading(val)

        for n in self.nodes.values():
            n.createNoiseModel()

    def add_connection(self, n1, n2, distance=-59):
        """
        Add to the radio channel a connection between the two nodes
        If the given nodes are not present already add them
        """
        self.add_node(n1)
        self.add_node(n2)
        self.topology.add_connection(n1, n2, distance)

    def remove_connection(self, n1, n2):
        # here we don't need to remove the nodes themselves
        self.topology.remove_connection(n1, n2)

    def interactive(self):
        # Use a dictionary and function calls instead
        def send_interactive():
            packet = make_packet()
            self.send_packet(packet)

        choices = (("topology management" , self.manipulate_topology),
                   ("packet creation" , send_interactive),
                   ("variable inspection" , self.inspect_variable),
                   ("node inspection" , self.inspect_node),
                   ("channel management" , self.manage_channels))

        MenuMaker(choices).call_option()

    def run_some_events(self):
        "Run some of the events"
        # TODO: pass some arguments to make sure they're enough
        time = self.sim.time()
        while(time + RUNTIME * 10000000000 > self.sim.time()):
            self.throttle.checkThrottle()
            self.sim.runNextEvent()

        self.throttle.printStatistics()

    def check_vars_all_nodes(self, var, value):
        return self.check_vars_nodes(sorted(self.nodes.keys()), var, value)

    def check_vars_nodes(self, nodes, var, value):
        "Check that all the variables of nodes have that value"
        for n in nodes:
            val = self.get_variable(n, var)
            print "node %d = %d" % (n, val)
            if val != value:
                return False

        return True

    def inspect_variable(self):
        "Ask for a variable to inspect and returns it"
        readline.parse_and_bind("tab: complete")
        node = input("which node you want to inspect?\n")
        print_var_table(self.vars)
        # see if this is actually correct
        c = rlcompleter.Completer(dict(zip(self.vars, self.vars)))
        readline.set_completer(c.complete)
        var = raw_input("which variable do you want to inspect?\n")
        print "node %d:var %s = %s" % (node, var, self.get_variable(node, var))

    def inspect_node(self):
        "Show all the variables in a certain node"
        node = input("which node you want to inspect?\n")
        self.print_node_vars(node)

    def get_variable(self, node, var):
        return self.nodes[node].getVariable(var).getData()

    def filter_variable(self, mod=MODULES_REGEXP):
        for v in self.vars:
            if match(mod, v):
                yield v

    def print_node_vars(self, node):
        for v in self.filter_variable():
            print self.get_variable(node, v)

    def print_var_nodes(self, var):
        for x in sorted(self.nodes):
            print "%d -> %s" % (x, self.get_variable(x, var))

    def inactive_channels(self):
        "returns the inactive debug channels"
        return list(set(CHANNELS) - set(self.channels))

    def manage_channels(self):
        def add_channel():
            # only give the remaining to add not all of them
            readline.parse_and_bind("tab: complete")
            c = rlcompleter.Completer(dict(zip(CHANNELS, CHANNELS)))
            readline.set_completer(c.complete)
            channel = raw_input("what channel you want to add?\n")
            # TODO: see how to add for only one mote
            self.add_channel(channel)

        def rem_channel():
            readline.parse_and_bind("tab: complete")
            c = rlcompleter.Completer(dict(zip(self.channels, self.channels)))
            readline.set_completer(c.complete)
            channel = raw_input("what channel you want to remove?\n")
            self.remove_channel(channel)

        def activate_all():
            for c in self.inactive_channels():
                self.add_channel(c)

        def see_channels():
            print self.channels

        choices = (("Add a new debug channel",  add_channel),
                   ("Remove a debug channel", rem_channel),
                   ("See debug channels activated", see_channels),
                   ("Activate all debug channels", activate_all))

        MenuMaker(choices).call_option()


    def manipulate_topology(self):
        print_out = lambda: sys.stdout.write(str(self.topology))
        def add_nodes():
            try:
                n1, n2 = input("first node\n"), input("second node\n")
            except Exception:
                print "input error"
                self.manipulate_topology()

            else:
                self.add_connection(n1, n2)

        def rem_nodes():
            nodes = raw_input("what are the nodes to remove (symmetrically) write X Y?\n")
            try:
                n1, n2 = map(int, nodes.split(" "))
            except ValueError:
                print "write better values"
                self.manipulate_topology()
            else:
                self.remove_connection(n1, n2)

        def disconnect_node():
            try:
                node = input("what node you want to disconnect\n")
            except Exception:
                self.manipulate_topology()
            else:
                self.topology.disconnect_node(node)

        choices = (("see topology", print_out),
                   ("add one connection", add_nodes),
                   ("remove one connection", rem_nodes),
                   ("disconnect one node", disconnect_node))

        MenuMaker(choices).call_option()

    def send_packet(self, msg):
        "Takes a BlinkMsg already generated and sends it via serial"
        serialpkt = self.sim.newSerialPacket();
        serialpkt.setData(msg.get_data())
        serialpkt.setType(msg.am_type)
        # TODO: this 0 is ok to be 
        serialpkt.setDestination(0)
        serialpkt.deliver(0, self.sim.time() + 3)
        self.run_some_events()

        print "sended packet:\n%s" % str(msg)
        # TODO: the seqno is not actually used here
        self.seqno += 1

if __name__ == '__main__':
    sim = Simulation(SERIAL_PORT, CHANNELS)
    # topo_file = "topo.txt"

    # if len(sys.argv) == 2:
    #     # TODO: use some automated testing stuff if possible
    #     topo_file = sys.argv[1]
    
    # TODO: only creates the number of nodes present our file
    # sim.make_topology(topo_file)
    # sim.setup_noise("noise.txt")
    sim.single_node()
    sim.start()

