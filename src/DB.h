/*

  Copyright (c) 2009, Nokia Corporation
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
  
    * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.  
    * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in
    the documentation and/or other materials provided with the
    distribution.  
    * Neither the name of Nokia nor the names of its contributors 
    may be used to endorse or promote products derived from this 
    software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
  COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 */
/*
 *  DB.h
 *
 *  Author: Ora Lassila mailto:ora.lassila@nokia.com
 *  Copyright (c) 2001-2008 Nokia. All Rights Reserved.
 */

#pragma once

#include <iostream>
#include <stdarg.h>
#include <cstdlib>
#include "Useful.h"
#include "SQL.h"
#include "Node.h"
#include "Action.h"
#include "Parser.h"
#include "Mutex.h"

namespace Piglet {

typedef int (*TripleQueryCallback)(void *, Triple *);

const Node Node_rdf_type = 1;
const Node Node_rdf_Property = 2;
const Node Node_rdfs_Resource = 3;
const Node Node_rdfs_Class = 4;
const Node Node_rdfs_subClassOf = 5;

class Parser;

class DB {
public:
  DB(char* name, bool verbose = false) MAYFAIL;
  ~DB(void) MAYFAIL;
  static DB *current(void) { return DB::_current; }
  SQL::Database *getDatabase() { return _db; }
  inline bool& verboseOps(void) { return _verboseOps; }
  virtual Node node(const char *uri, bool bnode = false) MAYFAIL;
  virtual Node literal(const char *str, Node datatype = NULL_NODE, const char *lang = NULL) MAYFAIL;
  virtual bool augmentLiteral(Node literal, Node datatype) MAYFAIL;
  virtual Triples *query(Node subject, Node predicate, Node object, Node source=NULL_NODE) MAYFAIL;
  virtual bool query(Node subject, Node predicate, Node object, Node source, TripleAction *action) MAYFAIL;
  virtual bool queryUsingSQL(char *condition, GenericAction *action) MAYFAIL;
  virtual bool exists(Node s, Node p, Node o, Node source = NULL_NODE, bool temporary = false) MAYFAIL;
  virtual Triple *add(Triple *triple, Node source = NULL_NODE, bool temporary = false) MAYFAIL;
  virtual bool addPostProcess(Triple *t) MAYFAIL;
  virtual Triple *del(Triple *triple, Node source = NULL_NODE, bool temporary = false) MAYFAIL;
  virtual int count(Node s, Node p, Node o, Node source, bool temporary) MAYFAIL;
  virtual bool sources(Triple *triple, NodeAction *action) MAYFAIL;
  virtual Nodes *sources(Triple *triple) MAYFAIL;
  virtual bool load(Node source, unsigned char* content, bool verbose) MAYFAIL;
  virtual bool load(Node source, bool append = false, bool verbose = false, char *path = NULL, char *argv[] = NULL) MAYFAIL;
  virtual bool load(const char *source, bool append = false, bool verbose = false, char *path = NULL, char *argv[] = NULL) MAYFAIL;
  virtual bool addNamespace(const char *prefix, const char *uri) MAYFAIL;
  virtual void delNamespace(const char *prefix) MAYFAIL;
  virtual char *toString(const Node n) MAYFAIL;
  virtual char *toString(const Triple *t) MAYFAIL;
  virtual char *info(Node id, Node *datatype = NULL, char *language = NULL) MAYFAIL;
  virtual bool delSourceTriples(Node source) MAYFAIL;
  virtual bool delSource(Node source) MAYFAIL;
  virtual Nodes *allSources(void) MAYFAIL;
  virtual char *nodeQName(Node n) MAYFAIL;
  virtual char *nodeQName(const char *uri) MAYFAIL;
  virtual char *qName2URI(const char *qname) MAYFAIL;
  virtual char *tryQName2URI_m3(const char *qname);
  virtual bool match(const char *pattern, NodeAction *action) MAYFAIL;
  virtual bool transaction(void) MAYFAIL;
  virtual bool commit(void) MAYFAIL;
  virtual bool rollback(void) MAYFAIL;
protected:
  void addQuick(Node subject, Node predicate, Node object) MAYFAIL;
  inline bool isLiteral(Node n) { return n < NULL_NODE; }
  bool db(const char *query, const char *msg = NULL,
          void *arg = NULL, SQL::Callback callback = NULL) MAYFAIL;
  char* makeWildcardQuery(const char *prefix, Node s, Node p, Node o, Node source);
  int newNodeID(void) MAYFAIL;
  int newLiteralID(void) MAYFAIL;
  Parser *createParser(void);
  virtual char *prefix2namespace(const char *prefix) MAYFAIL;
  virtual char *namespace2prefix(const char *uri) MAYFAIL;
private:
  mutex::Mutex _mutex;
  const char *_name;
  SQL::Database *_db;
  static DB *_current;
  bool _verboseOps;
};

}
