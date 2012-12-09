#!/usr/bin/python

#
#   makesql.py
#
#   Author: Ora Lassila mailto:ora.lassila@nokia.com
#   Copyright (c) 2001-2008 Nokia. All Rights Reserved.
#

import sys

def makeStringConstant(output, name, input):
    i = open(sys.argv[1] + input)
    try:
        output.write('\nstatic const char *%s =\n"%s";\n'
                     % (name, i.read().rstrip('\n').replace('"', "'").replace("\n", "\\\n")))
    finally:
        i.close()

if __name__ == "__main__":
    o = open(sys.argv[1] + "sqlconst.h", "w")
    try:
        o.write("#pragma once\n\nnamespace Piglet {\n")
        makeStringConstant(o, "SQL_CREATE_TEMP_DB", "createTempDB.sql")
        makeStringConstant(o, "SQL_CREATE_DB", "createDB.sql")
        o.write("\n}\n")
    finally:
        o.close()
