# Copyright (c) 2013-2014, The Linux Foundation. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 and
# only version 2 as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

import os
import platform
import glob
import re
import string
import sys

_parsers = []


class ParserConfig(object):

    """Class to encapsulate a RamParser its desired setup (command-line
    options, etc)."""

    def __init__(self, cls, longopt, desc, shortopt, optional):
        self.cls = cls
        self.longopt = longopt
        self.desc = desc
        self.shortopt = shortopt
        self.optional = optional

def cleanupString(unclean_str):
    if unclean_str is None:
        return unclean_str
    else:
        return ''.join([c for c in unclean_str if c in string.printable])

def register_parser(longopt, desc, shortopt=None, optional=False):
    """Decorator to register a parser class (a class that inherits from
    RamParser) with the parsing framework. By using this decorator
    your parser will automatically be hooked up to the command-line
    parsing code.

    This makes it very easy and clean to add a new parser:

      o Drop a new file in parsers that defines a class that inherits
        from RamParser

      o Decorate your class with @register_parser

      o Define a `parse' method for your class

    All of the command line argument handling and invoking the parse
    method of your parser will then be handled automatically.

    Required arguments:

    - longopt:: The longopt command line switch for this parser

    - desc:: A short description of the parser (also shown in the
      help-text associated with the longopt)

    Optional arguments:

    - shortopt:: The shortopt command line switch for this parser

    - optional:: Indicates the parser is optional and should not be run with
      --everything

    """
    def wrapper(cls):
        if cls in [p.cls for p in _parsers]:
            raise Exception(cls + ' is already registered!')
        _parsers.append(ParserConfig(cls, longopt, desc, shortopt, optional))
        return cls
    return wrapper


def get_parsers():
    """Imports everyone under the `parsers' directory. It is expected that
    the parsers under the parsers directory will be a collection of
    classes that subclass RamParser and use the register_parser
    decorator to register themselves with the parser
    framework. Therefore, importing all the modules under `parsers'
    should have the side-effect of populating the (internal to
    parser_util) _parsers list with the discovered parsers.

    Returns the list of ParserConfig instances built as a side-effect
    of the importing.

    """
    parsers_dir = os.path.join(os.path.dirname(__file__), 'parsers')
    for f in sorted(glob.glob(os.path.join(parsers_dir, '*.py'))):
        modname_ext = os.path.basename(f)
        if modname_ext == '__init__.py':
            continue
        modname = 'parsers.' + os.path.splitext(modname_ext)[0]
        # if the module contains a class (or classes) that are
        # decorated with `register_parser' then the following import
        # will have the side-effect of adding that class (encapsulated
        # in a ParserConfig object) to the _parsers list. Note that
        # this import is effectively a noop if the module has already
        # been imported, so there's no harm in calling get_parsers
        # multiple times.
        __import__(modname)
    return _parsers


class RamParser(object):

    """Base class for implementing ramdump parsers. New parsers should inherit
    from this class and define a `parse' method.

    Interesting properties that will be set for usage in derived
    classes:

    - ramdump:: The RamDump instance being parsed

    """

    def __init__(self, ramdump):
        self.ramdump = ramdump

    def parse(self):
        raise NotImplementedError


def which(program):
    """Just like which(1).

    Searches the PATH environment variable for a directory containing
    program.

    """
    for path in os.environ['PATH'].split(os.pathsep):
        exe_file = os.path.join(path, program)
        if os.access(exe_file, os.X_OK):
            return exe_file

    return None


def get_system_type():
    """Returns a "normalized" version of platform.system (transforming CYGWIN
    to Windows, for example).

    Returns None if not a supported platform.

    """
    plat = platform.system()
    if plat == 'Windows':
        return 'Windows'
    if re.search('CYGWIN', plat) is not None:
        # On certain installs, the default windows shell
        # runs cygwin. Treat cygwin as windows for this
        # purpose
        return 'Windows'
    if plat == 'Linux':
        return 'Linux'
    if plat == 'Darwin':
        return 'Darwin'

def _get_printable(n, fillchar='.'):
    if n is None:
        return
    c = chr(n)
    if c in string.printable[:string.printable.index(' ') + 1]:
        return c
    return fillchar

def _xxd_line(addr, data):
    printable = [_get_printable(d) for d in data]
    data = ['{:02x}'.format(d) for d in data]
    printable += [' '] * (16 - len(printable))
    data += ['  '] * (16 - len(data))
    return "{:08x}: {:}{:} {:}{:} {:}{:} {:}{:} {:}{:} {:}{:} {:}{:} {:}{:}  {:}{:}{:}{:}{:}{:}{:}{:}{:}{:}{:}{:}{:}{:}{:}{:}\n".format(
        addr, *(data + printable)
    )

def xxd(address, data, file_object=None):
    """Dumps data to stdout, in the format of `xxd'. data should be a list
    of integers.

    >>> xxd(0x1000, [0xde, 0xad, 0xbe, 0xef, 112, 105, 122, 122, 97, 0, 0, 42, 43, 44, 45, 90])
    00001000: dead beef 7069 7a7a 6100 002a 2b2c 2d5a  ....pizza..*+,-Z

    """
    f = file_object or sys.stdout
    bb = []
    n = 0
    for i in data:
        bb.append(i)
        if n == 15:
            f.write(_xxd_line(address, bb))
            bb = []
            n = 0
            address += 16
        else:
            n += 1
    if len(bb):
        f.write(_xxd_line(address, bb))

if __name__ == "__main__":
    import doctest
    doctest.testmod()
