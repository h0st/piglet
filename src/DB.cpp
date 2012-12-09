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
 *  DB.cpp
 *
 *  Author: Ora Lassila mailto:ora.lassila@nokia.com
 *  Copyright (c) 2001-2008 Nokia. All Rights Reserved.
 */
#include <stdint.h>
#include <iostream>
#include <stdio.h>
#include <raptor.h>
#include <time.h>
#include <string.h>
#include "Curl.h"
#include "Messages.h"
#include "DB.h"
#include "RaptorParser.h"
#include "sqlconst.h"

namespace Piglet {

DB *DB::_current = NULL;

DB::DB(char* name, bool verbose) MAYFAIL
{
  verboseOps() = verbose;
  RaptorParser::init(); // implies: uses RaptorParser, only one database per program (!)
  _db = new SQL::Database(name, PIGLET_DEBUG);
  check(_db->isOpen(), ERR_DB_OPEN);
  db("ATTACH ':memory:' AS cache;");
  db((char *)SQL_CREATE_TEMP_DB);
  DB::_current = this;
  char *version = NULL;
  try {
    db("SELECT version FROM info", NULL, &version, (SQL::Callback)SQL::oneStringCallback);
    if (verboseOps())
      std::cerr << "Existing database, version \"" << version << "\"\n";
  }
  catch (Condition &c) {
    if (verboseOps())
      std::cerr << "Creating a new database\n";
    db((char *)SQL_CREATE_DB);
  }
  free(version);
}

DB::~DB(void) MAYFAIL
{
  if (_db)
    delete _db; // closes native db connection
  RaptorParser::finish();
}

int DB::newNodeID(void) MAYFAIL
{
  int id = 0;
  db("SELECT max(id) FROM node;", ERR_NODE_ID, &id, (SQL::Callback)SQL::oneIntCallback);
  return id + 1;
}

int DB::newLiteralID(void) MAYFAIL
{
  int id = 0;
  db("SELECT min(id) FROM node;", ERR_NODE_ID, &id, (SQL::Callback)SQL::oneIntCallback);
  return id - 1;
}

struct LiteralDetails {
  char *str;
  int datatype;
  char *language;
};

static int literalDetailsCallback(LiteralDetails *ld, int argc, char **argv, char **cols)
{
  if (argv[0])
    ld->str = strdup(argv[0]);
  ld->datatype = atoi(argv[1]);
  if (ld->language && argv[2])
    strcpy(ld->language, argv[2]);
  return 0;
}

char *DB::info(Node n, Node *datatype, char *language) MAYFAIL
{
  if (isLiteral(n)) {
    LiteralDetails ld;
    ld.str = NULL;
    ld.datatype = 0;
    ld.language = language;
    db(tempsql(SQL::query("SELECT str, datatype, lang FROM node WHERE id = %d", id(n))),
       ERR_NODE_DETAILS, (void *)&ld, (SQL::Callback)literalDetailsCallback);
    if (datatype)
      *datatype = Node(ld.datatype);
    return ld.str;
  }
  else {
    char *uri = NULL;
    db(tempsql(SQL::query("SELECT str FROM node WHERE id = %d", id(n))),
       ERR_NODE_DETAILS, &uri, (SQL::Callback)SQL::oneStringCallback);
    return uri;
  }
}

char *DB::toString(const Node n) MAYFAIL
{
  char *str = NULL;
  if (isLiteral(n)) {
    Node datatype;
    char language[256];
    language[0] = '\0';
    TemporaryString s(info(n, &datatype, language));
    if (datatype != NULL_NODE) {
      TemporaryString dtstr(toString(datatype));
      str = (char *)malloc(2 + strlen(s.string()) + 3 + strlen(dtstr.string()) - 1 + 1);
      strcpy(str, "#\"");
      strcat(str, s.string());
      strcat(str, "\"^^");
      strcat(str, dtstr.string() + 1);
    }
    else if (language[0]) {
      str = (char *)malloc(2 + strlen(s.string()) + 2 + strlen(language) + 1);
      strcpy(str, "#\"");
      strcat(str, s.string());
      strcat(str, "\"@");
      strcat(str, language);
    }
    else {
      str = (char *)malloc(2 + strlen(s.string()) + 1 + 1);
      strcpy(str, "#\"");
      strcat(str, s.string());
      strcat(str, "\"");
    }
  }
  else {
    TemporaryString uri(info(n));
    if (uri.string()) {
      TemporaryString qname(nodeQName(uri.string()));
      if (qname.string()) {
        str = (char *)malloc(1 + strlen(qname.string()) + 1);
        strcpy(str, "!");
        strcat(str, qname.string());
      }
      else {
        str = (char *)malloc(2 + strlen(uri.string()) + 1 + 1);
        strcpy(str, "!\"");
        strcat(str, uri.string());
        strcat(str, "\"");
      }
    }
    else {
      char idstr[33];
      sprintf(idstr, "%d", id(n));
      str = (char *)malloc(2 + strlen(idstr) + 1 + 1);
      strcpy(str, "!<");
      strcat(str, idstr);
      strcat(str, ">");
    }
  }
  return str;
}

char *DB::toString(const Triple *t) MAYFAIL
{
  TemporaryString sstr(toString(t->s()));
  TemporaryString pstr(toString(t->p()));
  TemporaryString ostr(toString(t->o()));
  char *s = (char *)malloc(1 + strlen(sstr.string()) + 2 + strlen(pstr.string()) + 2 + strlen(ostr.string()) + 1 + 1);
  strcpy(s, "<");
  strcat(s, sstr.string());
  strcat(s, ", ");
  strcat(s, pstr.string());
  strcat(s, ", ");
  strcat(s, ostr.string());
  strcat(s, ">");
  return s;
}

Node DB::node(const char *uri, bool bnode) MAYFAIL
{
  mutex::MutexLock lock(&_mutex);
  int id;
  if (bnode) {
    if (uri == NULL)
      return node(NULL, false);
    else {
      id = 0;
      db(tempsql(SQL::query("SELECT id FROM cache.bnode WHERE str=%Q;", uri)),
         ERR_NODE_NEW, &id, (SQL::Callback)SQL::oneIntCallback);
      if (id == 0) {
        id = id(node(NULL, false));
        db(tempsql(SQL::query("INSERT INTO cache.bnode VALUES(%d,%Q);", id, uri)), ERR_NODE_NEW);
      }
      return Node(id);
    }
  }
  else if (uri == NULL) {
    id = newNodeID();
    db(tempsql(SQL::query("INSERT INTO node VALUES(%d, NULL, 0, NULL)", id)), ERR_NODE_NEW);
    return Node(id);
  }
  else {
    int id = 0;
    db(tempsql(SQL::query("SELECT id FROM node WHERE str = %Q AND id > 0", uri)),
       ERR_NODE_FIND, &id, (SQL::Callback)SQL::oneIntCallback);
    if (id == 0) {
      id = newNodeID();
      db(tempsql(SQL::query("INSERT INTO node VALUES(%d, %Q, 0, NULL)", id, uri)), ERR_NODE_NEW);
    }
    return Node(id);
  }
}

Node DB::literal(const char *str, Node dt, const char *lang) MAYFAIL
{
  mutex::MutexLock lock(&_mutex);
  int id = 0;
  if (dt != NULL_NODE) {
    db(tempsql(SQL::query("SELECT id FROM node WHERE str=%Q AND id<0 AND datatype=%d", str, id(dt))),
       ERR_NODE_FIND, &id, (SQL::Callback)SQL::oneIntCallback);
    if (id == 0) {
      id = newLiteralID();
      db(tempsql(SQL::query("INSERT INTO node VALUES(%d, %Q, %d, NULL)", id, str, id(dt))),
         ERR_NODE_NEW);
    }
  }
  else if (lang != NULL) {
    db(tempsql(SQL::query("SELECT id FROM node WHERE str=%Q AND id<0 AND lang=%Q", str, lang)),
       ERR_NODE_FIND, &id, (SQL::Callback)SQL::oneIntCallback);
    if (id == 0) {
      id = newLiteralID();
      db(tempsql(SQL::query("INSERT INTO node VALUES(%d, %Q, 0, %Q)", id, str, lang)), ERR_NODE_NEW);
    }
  }
  else {
    db(tempsql(SQL::query("SELECT id FROM node WHERE str=%Q AND id<0", str)),
       ERR_NODE_FIND, &id, (SQL::Callback)SQL::oneIntCallback);
    if (id == 0) {
      id = newLiteralID();
      db(tempsql(SQL::query("INSERT INTO node VALUES(%d, %Q, 0, NULL)", id, str)), ERR_NODE_NEW);
    }
  }
  return Node(id);
}

bool DB::augmentLiteral(Node literal, Node datatype) MAYFAIL
{
  if (id(literal) > 0)
    return false;
  else {
    Node oldDatatype = 0;
    char *contents = info(literal, &oldDatatype, NULL);
    if (contents != NULL)
      free(contents);
    if (oldDatatype == datatype)
      return true;
    else
      return db(tempsql(SQL::query("UPDATE node SET datatype = %d WHERE id=%d",
                                   id(datatype), id(literal))),
                ERR_NODE_NEW);
  }
}

bool DB::exists(Node s, Node p, Node o, Node source, bool temporary) MAYFAIL
{
  int i = 0;
  db(tempsql(SQL::query("%s LIMIT 1",
                        tempsql(makeWildcardQuery((temporary
                                                   ? "SELECT 1 FROM cache.triple"
                                                   : "SELECT 1 FROM triple"),
                                                  s, p, o, source)))),
     ERR_NODE_FIND, &i, (SQL::Callback)SQL::oneIntCallback);
  return i == 1;
}

static int tripleCallback(TripleAction *action, int argc, char **argv, char **cols)
{
  int rval;
  Triple *t = new Triple(atoi(argv[0]), atoi(argv[1]), atoi(argv[2]));
  rval = (*action)(t) ? 0 : 1;
  delete t;
  return rval;
}

static int nodeCallback(NodeAction *nodes, int argc, char **argv, char **cols)
{
  if (argv[0])
    return (*nodes)(Node(atoi(argv[0]))) ? 0 : 1;
  else
    return 0;
}

static int genericCallback(GenericAction *action, int argc, char **argv, char **cols)
{
  return (*action)(argc, argv, cols) ? 0 : 1;
}

Triples *DB::query(Node subject, Node predicate, Node object, Node source) MAYFAIL
{
  Triples *triples = new Triples(this);
  if (query(subject, predicate, object, source, triples))
    return triples;
  else {
    delete triples;
    return NULL;
  }
}

bool DB::query(Node subject, Node predicate, Node object, Node source, TripleAction *action) MAYFAIL
{
  return db(tempsql(SQL::query("%s UNION %s", // UNION implies DISTINCT
                               tempsql(makeWildcardQuery("SELECT s,p,o FROM triple",
                                                         subject, predicate, object, source)),
                               tempsql(makeWildcardQuery("SELECT s,p,o FROM cache.triple",
                                                         subject, predicate, object, source)))),
            ERR_TRIPLE_FIND, action, (SQL::Callback)tripleCallback);
}

bool DB::queryUsingSQL(char *condition, GenericAction *action) MAYFAIL
{
  return db(tempsql(SQL::query("%s UNION %s", // UNION implies DISTINCT
                               tempsql(SQL::query("SELECT t.s,t.p,t.o FROM triple as t %s",
                                                  condition)),
                               tempsql(SQL::query("SELECT t.s,t.p,t.o FROM cache.triple as t %s",
                                                  condition)))),
            ERR_TRIPLE_FIND, action, (SQL::Callback)genericCallback);
}

bool DB::sources(Triple *triple, NodeAction *action) MAYFAIL
{
  // should this also query the temporary table?
  return db(tempsql(makeWildcardQuery("SELECT DISTINCT src FROM triple",
                                      triple->s(), triple->p(), triple->o(), NULL_NODE)),
            ERR_SRC_QUERY, action, (SQL::Callback)nodeCallback);
}

Nodes *DB::sources(Triple *triple) MAYFAIL
{
  Nodes *s = new Nodes(this);
  if (sources(triple, s))
    return s;
  else {
    delete s;
    return NULL;
  }
}

char* DB::makeWildcardQuery(const char *pre, Node s, Node p, Node o, Node source)
{
  if (s == NULL_NODE) {             // [*,?,?,?]
    if (p == NULL_NODE) {           // [*,*,?,?]
      if (o == NULL_NODE) {         // [*,*,*,?]
        if (source == NULL_NODE)    // [*,*,*,*]
          return SQL::query("%s", pre);
        else                        // [*,*,*,source]
          return SQL::query("%s WHERE src=%d", pre, id(source));
      }
      else if (source == NULL_NODE) // [*,*,o,*]
        return SQL::query("%s WHERE o=%d", pre, id(o));
      else                          // [*,*,o,source]
        return SQL::query("%s WHERE o=%d AND src=%d", pre, id(o), id(source));
    }
    else if (o == NULL_NODE) {      // [*,p,*,?]
      if (source == NULL_NODE)      // [*,p,*,*]
        return SQL::query("%s WHERE p=%d", pre, id(p));
      else
        return SQL::query("%s WHERE p=%d AND src=%d", pre, id(p), id(source));
    }
    else if (source == NULL_NODE)   // [*,p,o,*]
      return SQL::query("%s WHERE p=%d AND o=%d", pre, id(p), id(o));
    else
      return SQL::query("%s WHERE p=%d AND o=%d AND src=%d", pre, id(p), id(o), id(source));
  }
  else if (p == NULL_NODE) {        // [s,*,?,?]
    if (o == NULL_NODE) {           // [s,*,*,?]
      if (source == NULL_NODE)      // [s,*,*,*]
        return SQL::query("%s WHERE s=%d", pre, id(s));
      else
        return SQL::query("%s WHERE s=%d AND src=%d", pre, id(s), id(source));
    }
    else if (source == NULL_NODE)   // [s,*,o,*]
      return SQL::query("%s WHERE s=%d AND o=%d", pre, id(s), id(o));
    else                            // [s,*,o,source]
      return SQL::query("%s WHERE s=%d AND o=%d AND src=%d", pre, id(s), id(o), id(source));
  }
  else if (o == NULL_NODE) {        // [s,p,*,?]
    if (source == NULL_NODE)        // [s,p,*,*]
      return SQL::query("%s WHERE s=%d AND p=%d", pre, id(s), id(p));
    else                            // [s,p,*,source]
      return SQL::query("%s WHERE s=%d AND p=%d AND src=%d", pre, id(s), id(p), id(source));
  }
  else if (source == NULL_NODE)     // [s,p,o,*]
    return SQL::query("%s WHERE s=%d AND p=%d AND o=%d", pre, id(s), id(p), id(o));
  else
    return SQL::query("%s WHERE s=%d AND p=%d AND o=%d AND src=%d",
                      pre, id(s), id(p), id(o), id(source));
}

Triple *DB::add(Triple *t, Node source, bool temporary) MAYFAIL
{
  mutex::MutexLock lock(&_mutex);
  if (temporary) {
    if (exists(t->s(), t->p(), t->o(), source, true) ||
        exists(t->s(), t->p(), t->o(), source, false))
      return NULL;
    else {
      db(tempsql(SQL::query("INSERT INTO cache.triple VALUES (%d, %d, %d, %d)",
                            id(t->s()), id(t->p()), id(t->o()), id(source))),
         ERR_TRIPLE_ADD);
      return t;
    }
  }
  else {
    if (exists(t->s(), t->p(), t->o(), source, false))
      return NULL;
    else {
      db(tempsql(SQL::query("INSERT INTO triple VALUES (%d, %d, %d, %d)",
                            id(t->s()), id(t->p()), id(t->o()), id(source))),
         ERR_TRIPLE_ADD);
      return t;
    }
  }
}

void DB::addQuick(Node subject, Node predicate, Node object) MAYFAIL
{
  Triple t(subject, predicate, object);
  (void)add(&t, NULL_NODE, true);
}

bool DB::addPostProcess(Triple *t) MAYFAIL
{
  Node p = t->p();
  Node o = t->o();
  addQuick(p, Node_rdf_type, Node_rdf_Property);
  if (p == Node_rdf_type) {
    addQuick(o, Node_rdf_type, Node_rdfs_Class);
    addQuick(o, Node_rdfs_subClassOf, Node_rdfs_Resource);
    return true;
  }
  else if (p == Node_rdfs_subClassOf) {
    addQuick(o, Node_rdfs_subClassOf, Node_rdfs_Resource);
    return true;
  }
  else return false;
}

Triple *DB::del(Triple *t, Node source, bool temporary) MAYFAIL
{
  mutex::MutexLock lock(&_mutex);
  if (exists(t->s(), t->p(), t->o(), source, temporary)) {
    db(tempsql(makeWildcardQuery((temporary ? "DELETE FROM cache.triple" : "DELETE FROM triple"),
                                 t->s(), t->p(), t->o(), source)),
       ERR_TRIPLE_DEL);
    return t;
  }
  else return NULL;
}

int DB::count(Node s, Node p, Node o, Node source, bool temporary) MAYFAIL
{
  int n = 0;
  db(tempsql(makeWildcardQuery((temporary
                                ? "SELECT count(*) FROM cache.triple"
                                : "SELECT count(*) FROM triple"),
                               s, p, o, source)),
     ERR_TRIPLE_FIND, &n, (SQL::Callback)SQL::oneIntCallback);
  //db("SELECT count(*) FROM triple;", ERR_SRC_QUERY, &n, (SQL::Callback)SQL::oneIntCallback);
  return n;
}

bool DB::addNamespace(const char *prefix, const char *uri) MAYFAIL
{
  mutex::MutexLock lock(&_mutex);
  if (temp(prefix2namespace(prefix)))
    return false;
  else {
    db(tempsql(SQL::query("INSERT INTO namespace VALUES(%Q, %Q, 1)", prefix, uri)), ERR_NS_ADD);
    return true;
  }
}

void DB::delNamespace(const char *prefix) MAYFAIL
{
  mutex::MutexLock lock(&_mutex);
  db(tempsql(SQL::query("DELETE FROM namespace WHERE prefix=%Q", prefix)), ERR_NS_DEL);
}

char *DB::prefix2namespace(const char *prefix) MAYFAIL
{
  char *uri = NULL;
  db(tempsql(SQL::query("SELECT uri FROM namespace WHERE prefix=%Q", prefix)),
     ERR_NS_FIND, &uri, (SQL::Callback)SQL::oneStringCallback);
  return uri;
}

// char *DB::prefix2namespace_m3(const char *prefix) MAYFAIL
// {
//   char *uri = NULL;
//   db(tempsql(SQL::query("SELECT uri FROM namespace WHERE prefix=%Q", prefix)),
//      ERR_NS_FIND, &uri, (SQL::Callback)SQL::oneStringCallback);
//   if (uri == NULL)
//     return prefix;
//   else
//     return uri;
// }

char *DB::namespace2prefix(const char *uri) MAYFAIL
{
  char *prefix = NULL;
  db(tempsql(SQL::query("SELECT prefix FROM namespace WHERE uri=%Q", uri)),
     ERR_NS_FIND, &prefix, (SQL::Callback)SQL::oneStringCallback);
  return prefix;
}

char *DB::nodeQName(Node n) MAYFAIL
{
  TemporaryString uri(info(n));
  if (uri.string())
    return nodeQName(uri.string());
  else
    return NULL;
}

char *DB::nodeQName(const char *uri) MAYFAIL
{
  int i;
  for (i = strlen(uri) - 1; i >= 0; i--) {
    char c = uri[i];
    if (c == '/' || c == '#') break;
  }
  if (i > 6) {
    TemporaryString nsuri(i+2);
    strncpy(nsuri.string(), uri, i + 1);
    nsuri.string()[i + 1] = '\0';
    TemporaryString prefix(namespace2prefix(nsuri.string()));
    if (prefix.string()) {
      char *qname = (char *)malloc(strlen(prefix.string()) + strlen(uri) - i + 2);
      strcpy(qname, prefix.string());
      strcat(qname, ":");
      strcat(qname, (uri + i + 1));
      return qname;
    }
  }
  return NULL;
}

char *DB::qName2URI(const char *qname) MAYFAIL
{
  char prefix[128], *p, *q;
  for (p = prefix, q = (char *)qname; *q != '\0' && *q != ':'; *p++ = *q++);
  check(*q != '\0', ERR_NS_FIND);
  *p = '\0';
  char *uri = NULL;
  db(tempsql(SQL::query("SELECT uri||%Q FROM namespace WHERE prefix=%Q", q+1, prefix)),
     ERR_NS_FIND, &uri, (SQL::Callback)SQL::oneStringCallback);
  return uri;
}

char *DB::tryQName2URI_m3(const char *qname)
{
  // M3 modification
  // Try to expand a namespace if the URI has an abbreviated one
  // No namespace is not considered a failure case
  // Optimization of a common case in M3 SIB usage
  char prefix[256], *p, *q;
  int i = 0;
  // std::cerr << "tryQName2URI_m3 initial arg: " << (char *)qname << "\n";
  for (p = prefix, q = (char *)qname, i = 0; 
       *q != '\0' && *q != ':' && i < 255; 
       (*p++ = *q++), i++);
  *p = '\0';
  char *uri = NULL;
  // If no prefix or http, mailto or file, skip the DB search
  if (*q == '\0' || 
      (0 == strcmp(prefix, "http")) ||
      (0 == strcmp(prefix, "mailto")) ||
      (0 == strcmp(prefix, "file"))) {
    uri = (char*)malloc(strlen(qname) + sizeof(char));
    strcpy(uri, qname);
    // std::cerr << "tryQName2URI_m3: " << (char *)qname << "\n";
    return uri;
  }
  db(tempsql(SQL::query("SELECT uri||%Q FROM namespace WHERE prefix=%Q", q+1, prefix)),
     ERR_NS_FIND, &uri, (SQL::Callback)SQL::oneStringCallback);
  if (uri) {
    // std::cerr << "tryQName2URI_m3: expanded " << (char *)qname << " to " << (char *)uri << "\n";
    return uri;
  }
  else {
    // std::cerr << "tryQName2URI_m3: " << (char *)qname << " not expanded\n";
    uri = (char*)malloc(strlen(qname) + sizeof(char));
    strcpy(uri, qname);
    // std::cerr << "tryQName2URI_m3: returning " << (char *)uri << "\n";
    return uri;
  }    
}

bool DB::match(const char *pattern, NodeAction *action) MAYFAIL
{
  return db(tempsql(SQL::query("SELECT id FROM node WHERE str LIKE '%q%%'", pattern)),
            ERR_SRC_QUERY, action, (SQL::Callback)nodeCallback);
}

bool DB::load(const char *source, bool append, bool verbose, char *script, char *argv[]) MAYFAIL
{
  return load(node(source), append, verbose, script, argv);
}

bool DB::load(Node source, unsigned char* content, bool verbose) MAYFAIL
{
  /*
   * M3 addition to support loading RDF/XML from strings directly
   * Does not include mechanisms to detect reload as it is not necessary
   * for the M3 use case
   * -- juhonkol
   */

  mutex::MutexLock lock(&_mutex);
  bool terminated = false;
  TemporaryString uri(info(source));
  verbose = verbose | PIGLET_DEBUG | verboseOps();
  if (verbose) {
    std::cerr << "Loading: " << uri.string();
    std::cerr << "...";
    std::cerr.flush();
  }

  time_t old_filetime = -1, new_filetime;
  bool reload = true;
  db(tempsql(SQL::query("SELECT created FROM source WHERE src=%d LIMIT 1", id(source))),
     ERR_SRC_FIND, &old_filetime, (SQL::Callback)SQL::oneIntCallback);
  reload = (old_filetime != -1);

  Parser *parser = createParser();
  db("BEGIN TRANSACTION;");
  try {
    db("DELETE FROM cache.bnode;");
    parser->parse(source, content);
    if (parser->terminated()) {
      terminated = true;
      db("ROLLBACK;");
    }
    else db(tempsql(reload
		    ? SQL::query("UPDATE source SET loaded=%d, created=%d WHERE src=%d; COMMIT",
				 time(NULL), new_filetime, id(source))
		    : SQL::query("INSERT INTO source VALUES (%d, %d, %d); COMMIT",
				 id(source), new_filetime, time(NULL))),
	    ERR_SRC_TIME);
  }
  catch (Condition &c) {
    db("ROLLBACK;");
    if (verbose) std::cerr << "failed\n";
    throw;
  }
  if (verbose)
    std::cerr << (terminated? "failed\n" : "done\n");
  delete parser;
  db("DELETE FROM cache.bnode;");
  
  return !terminated;
}

bool DB::load(Node source, bool append, bool verbose, char *script, char *argv[]) MAYFAIL
{
  mutex::MutexLock lock(&_mutex);
  bool terminated = false;
  TemporaryString uri(info(source));
  verbose = verbose | PIGLET_DEBUG | verboseOps();
  if (verbose) {
    std::cerr << "Loading: " << uri.string();
    if (script != NULL)
      std::cerr << " using script " << script;
    std::cerr << "...";
    std::cerr.flush();
  }
  time_t old_filetime = -1, new_filetime;
  bool reload = true;
  if (script == NULL) {
    db(tempsql(SQL::query("SELECT created FROM source WHERE src=%d LIMIT 1", id(source))),
       ERR_SRC_FIND, &old_filetime, (SQL::Callback)SQL::oneIntCallback);
    reload = (old_filetime != -1);
  }
  if ((script == NULL) && !libcurl::Curl::getFileTime(uri.string(), &new_filetime)) {
    terminated = true;
    if (verbose) std::cerr << "failed\n";
  }
  else if ((script != NULL) || !reload || ((new_filetime != 0) && (new_filetime > old_filetime))) {
    Parser *parser = createParser();
    db("BEGIN TRANSACTION;");
    try {
      db("DELETE FROM cache.bnode;");
      if (!append) // this is still a hack (compared to Wilbur functionality)
        delSourceTriples(source);
      if (script == NULL)
        parser->parse(source);
      else
        parser->parseFromScript(source, script, argv);
      if (parser->terminated()) {
        terminated = true;
        db("ROLLBACK;");
      }
      else db(tempsql(reload
                      ? SQL::query("UPDATE source SET loaded=%d, created=%d WHERE src=%d; COMMIT",
                                   time(NULL), new_filetime, id(source))
                      : SQL::query("INSERT INTO source VALUES (%d, %d, %d); COMMIT",
                                   id(source), new_filetime, time(NULL))),
           ERR_SRC_TIME);
    }
    catch (Condition &c) {
      db("ROLLBACK;");
      std::cerr << "failed\n";
      throw;
    }
    if (verbose)
      std::cerr << (terminated ? "failed\n" : "done\n");
    delete parser;
    db("DELETE FROM cache.bnode;");
  }
  else if (verbose)
    std::cerr << "no reload needed\n";

  return !terminated;
}

Parser *DB::createParser(void)
{
  return new RaptorParser(this);
}

bool DB::delSource(Node source) MAYFAIL
{
  mutex::MutexLock lock(&_mutex);
  db("BEGIN TRANSACTION;");
  try {
    delSourceTriples(source);
    db(tempsql(SQL::query("DELETE FROM source WHERE src=%d; COMMIT;", id(source))), ERR_SRC_DEL);
    return true;
  }
  catch (Condition &c) {
    db("ROLLBACK;");
    throw c;
  }
}

bool DB::delSourceTriples(Node source) MAYFAIL
{
  mutex::MutexLock lock(&_mutex);
  return (db(tempsql(makeWildcardQuery("DELETE FROM cache.triple",
                                       NULL_NODE, NULL_NODE, NULL_NODE, id(source))),
             ERR_SRC_DEL) &&
          db(tempsql(makeWildcardQuery("DELETE FROM triple",
                                       NULL_NODE, NULL_NODE, NULL_NODE, id(source))),
             ERR_SRC_DEL));
}

Nodes *DB::allSources(void) MAYFAIL
{
  Nodes *sources = new Nodes(this);
  db("SELECT src FROM source;", ERR_SRC_QUERY, sources, (SQL::Callback)nodeCallback);
  return sources;
}

bool DB::db(const char *query, const char *msg, void *arg, SQL::Callback callback) MAYFAIL
{
  char *errmsg;
  switch (_db->exec(query, arg, callback, &errmsg)) {
    case SQL::Database::OK:    return true;
    case SQL::Database::ABORT: return false;
    default: FAIL(msg ? msg : tempsql(errmsg));
  }
}

bool DB::transaction(void) MAYFAIL
{
  return db("BEGIN", ERR_TRANSACTION);
}

bool DB::commit(void) MAYFAIL
{
  return db("COMMIT", ERR_TRANSACTION);
}

bool DB::rollback(void) MAYFAIL
{
  return db("ROLLBACK", ERR_TRANSACTION);
}

}
