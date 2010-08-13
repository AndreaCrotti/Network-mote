"""
Some nice menu creation facilitites
TODO: use cmdloop instead it should be nicer
"""
import rlcompleter
import readline
import cmd
from collections import namedtuple

class MenuMaker(object):
    def __init__(self, options):
        "Pass a list of couples in form ((string, method), ...)"
        self.options = options

    def __str__(self):
        # only enumerate the things
        menu_str = ["%d) %s" % (x, y[0]) for x, y in enumerate(self.options)]
        return "\n" + "\n".join(menu_str) + "\n"

    # maybe one other thing like __call__ would be better
    def call_option(self):
        # not really needed since it's only a number
        print str(self)
        ch = input()
        # calling the right function, should work anywhere given the correct scope
        try:
            self.options[ch][1]()
        except IndexError:
            print "%d is not a valid choice" % ch
            self.call_option()


# check if this is really working
def complete_on(completion_list):
    "Activate tab completion list over the list given"
    readline.parse_and_bind("tab: complete")
    c = rlcompleter.Completer(dict(zip(completion_list, completion_list)))
    readline.set_completer(c.complete)
