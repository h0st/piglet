#
#   setup.py for Piglet
#
#   Author: Ora Lassila mailto:ora.lassila@nokia.com
#   Copyright (c) 2001-2008 Nokia. All Rights Reserved.
#

from distutils.core import setup, Extension

pygletmodule = Extension('piglet',
                         include_dirs = ['/usr/local/include', 'src'],
                         libraries = ['curl', 'raptor', 'sqlite3', 'piglet'],
                         library_dirs = ['/usr/local/lib', '.'],
                         sources = ['src/pygletmodule.c'])

setup(name = 'Piglet',
      description = 'Interface to Piglet RDF triplestore (libpiglet)',
      author = "Ora Lassila",
      author_email = "ora.lassila@nokia.com",
      ext_modules = [pygletmodule])
