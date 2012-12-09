#
#   pigletserver.py
#
#   Author: Ora Lassila mailto:ora.lassila@nokia.com
#   Copyright (c) 2001-2008 Nokia. All Rights Reserved.
#
#   This file implements a simple HTTP server for Piglet.
#

import BaseHTTPServer
import SimpleHTTPServer
import piglet
import sys
import urlparse
import urllib
import json # http://sourceforge.net/projects/json-py/

class PigletServer(BaseHTTPServer.HTTPServer):
    def __init__(self, address, dbfile):
        BaseHTTPServer.HTTPServer.__init__(self, address, PigletRequestHandler)
        self.db = piglet.open(dbfile)
        self.json = json.JsonWriter()
        self.helper = PigletHelper(self.db)
        self.methods = {
            '/query':         lambda p: self.db.query(int(p.get('s', 0)),
                                                      int(p.get('p', 0)),
                                                      int(p.get('o', 0))),
            '/sources':       lambda p: self.db.sources(int(p.get('s', 0)),
                                                        int(p.get('p', 0)),
                                                        int(p.get('o', 0))),
            '/add':           lambda p: self.db.count(int(p['s']),
                                                      int(p['p']),
                                                      int(p['o']),
                                                      int(p.get('source', 0)),
                                                      int(p.get('temporary', 0))),
            '/del':           lambda p: self.db.count(int(p['s']),
                                                      int(p['p']),
                                                      int(p['o']),
                                                      int(p.get('source', 0)),
                                                      int(p.get('temporary', 0))),
            '/count':         lambda p: self.db.count(int(p.get('s', 0)),
                                                      int(p.get('p', 0)),
                                                      int(p.get('o', 0)),
                                                      int(p.get('source', 0)),
                                                      int(p.get('temporary', 0))),
            '/info':          lambda p: self.db.info(int(p['id'])),
            '/node':          lambda p: self.db.node(p['uri']),
            '/literal':       lambda p: self.db.literal(p['string'],
                                                        int(p.get('dt', 0)),
                                                        p.get('lang', "")),
            '/load':          lambda p: self.db.load(int(p['source'])),
            '/addNamespace':  lambda p: self.db.addNamespace(p['prefix'],
                                                             p['uri']),
            '/delNamespace':  lambda p: self.db.delNamespace(p['prefix']),
            '/expand':        lambda p: self.db.expand(p['qname']),
            '/reverseExpand': lambda p: self.db.reverseExpand(p['uri']),
            '/values':        lambda p: self.helper.values(int(p['node']),
                                                           int(p['path'])),
            '/label':         lambda p: self.helper.label(int(p['id']))
}

class PigletRequestHandler(SimpleHTTPServer.SimpleHTTPRequestHandler):
    def do_GET(self):
        parts = urlparse.urlsplit(self.path)
        method = parts[2]
        try:
            result = self.server.methods[method](self.parseParams(parts[3]))
            self.send_response(200)
            self.send_header('Content-type:', 'text/plain')
            self.end_headers()
            self.wfile.write(json.write(result) + '\r\n')
        except KeyError, key:
            self.send_response(400)
            self.send_header('Content-type:', 'text/html')
            self.end_headers()
            self.wfile.write("Parameter %s missing from '%s'" % (unicode(key), method))

    def parseParams(self, query):
        params = {}
        for param in query.split('&'):
            (key, value) = param.split('=')
            params[key] = urllib.unquote_plus(value)
        return params

class PigletHelper:
    def __init__(self, db):
        self.db = db
        self.nodes = {}

    def node(self, qname):
        n = self.nodes.get(qname)
        if (n):
            return n
        else:
            n = self.db.node(self.db.expand(qname))
            self.nodes[qname] = n
            return n

    def values(self, node, property):
        return [o for (s, p, o) in self.db.query(node, property, 0)]

    def label(self, id, quoteLiteral=True):
        if (id > 0):
            labels = self.values(id, self.node('rdfs:label'))
            if (len(labels) > 0):
                return self.label(labels[0], False)
            else:
                uri = self.db.info(id)
                return self.db.reverseExpand(uri) or uri
        else:
            str = self.db.info(id)[0]
            if (quoteLiteral):
                return '"%s"' % str
            else:
                return str

if __name__ == "__main__":
    server = None
    try:
        server = PigletServer(('', int(sys.argv[2])), sys.argv[1])
        sys.stdout.write("PigletServer serving on port %s...\n" % (sys.argv[2]))
        server.serve_forever()
    except IndexError:
        sys.stderr.write("usage: %s dbfile port\n" % sys.argv[0])
    except KeyboardInterrupt:
        sys.stderr.write("\nPigletServer is done\n")
    finally:
        if (server != None):
            server.db.close()
