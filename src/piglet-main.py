#!/usr/bin/python

import sys
import piglet

db = pyglet.open(sys.argv[1])
db.load(db.node("http://www.w3.org/1999/02/22-rdf-syntax-ns"))
db.load(db.node("http://www.w3.org/2000/01/rdf-schema"))
db.load(db.node("http://purl.org/dc/elements/1.1/"))
for (s, p, o) in db.query(db.node("http://purl.org/dc/elements/1.1/creator"), 0, 0) :
  print db.tripleToString(s, p, o)
db.close()
