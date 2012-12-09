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
 *  cpiglet.cpp
 *
 *  Author: Ora Lassila mailto:ora.lassila@nokia.com
 *  Copyright (c) 2001-2008 Nokia. All Rights Reserved.
 */

extern "C" {
#include "cpiglet.h"
}

#include <cstdlib>
#include "Curl.h"
#include "DB.h"

const char *piglet_error_message;

static PigletStatus piglet_error(Piglet::Condition &c)
{
  std::cerr << c;
  piglet_error_message = c.message();
  return PigletError;
}

static PigletStatus piglet_success(bool status)
{
  return (status ? PigletTrue : PigletFalse);
}

DB piglet_open(char *name)
{
  try {
    return (DB)new Piglet::DB(name, false);
  }
  catch (Piglet::Condition &c) {
    piglet_error(c);
    return NULL;
  }
}

PigletStatus piglet_close(DB db)
{
  try {
    delete ((Piglet::DB *)db);
    return PigletTrue;
  }
  catch (Piglet::Condition &c) {
    return piglet_error(c);
  }
}

class CallbackTripleAction : public Piglet::TripleAction {
public:
  CallbackTripleAction(Piglet::DB *db, void* userdata, TripleCallback callback)
  : Piglet::TripleAction(db) { _callback = callback; _userdata = userdata; }
  bool operator()(Piglet::Node s, Piglet::Node p, Piglet::Node o) throw (Piglet::Condition &);
  bool operator()(Piglet::Triple *t) throw (Piglet::Condition &);
private:
  TripleCallback _callback;
  void *_userdata;
};

bool CallbackTripleAction::operator()(Piglet::Node s, Piglet::Node p, Piglet::Node o) throw (Piglet::Condition &)
{
  Piglet::Triple t(s, p, o);
  return operator()(&t);
}

bool CallbackTripleAction::operator()(Piglet::Triple *t) throw (Piglet::Condition &)
{
  return (_callback)((DB)db(), _userdata, id(t->s()), id(t->p()), id(t->o()));
}

PigletStatus piglet_query(DB db, Node s, Node p, Node o, Node source, void* userdata, TripleCallback callback)
{
  try {
    CallbackTripleAction action((Piglet::DB *)db, userdata, callback);
    return piglet_success(((Piglet::DB *)db)->query(Piglet::Node(s),
                                                    Piglet::Node(p),
                                                    Piglet::Node(o),
                                                    Piglet::Node(source),
                                                    &action));
  }
  catch (Piglet::Condition &c) {
    return piglet_error(c);
  }
}

class CallbackNodeAction : public Piglet::NodeAction {
public:
  CallbackNodeAction(Piglet::DB *db, void *userdata, NodeCallback callback)
  : Piglet::NodeAction(db) { _callback = callback; _userdata = userdata; }
  bool operator()(Piglet::Node node) throw (Piglet::Condition &);
private:
  NodeCallback _callback;
  void *_userdata;
};

bool CallbackNodeAction::operator()(Piglet::Node node) throw (Piglet::Condition &)
{
  return (_callback)((DB)db(), _userdata, id(node));
}

PigletStatus piglet_sources(DB db, Node s, Node p, Node o, void* userdata, NodeCallback callback)
{
  try {
    CallbackNodeAction action((Piglet::DB *)db, userdata, callback);
    Piglet::Triple t(s, p, o);
    return piglet_success(((Piglet::DB *)db)->sources(&t, &action));
  }
  catch (Piglet::Condition &c) {
    return piglet_error(c);
  }
}

PigletStatus piglet_add(DB db, Node s, Node p, Node o, Node source, bool temporary)
{
  try {
    Piglet::Triple t(Piglet::Node((int)s), Piglet::Node(p), Piglet::Node(o));
    return piglet_success(((Piglet::DB *)db)->add(&t, Piglet::Node(source), temporary));
  }
  catch (Piglet::Condition &c) {
    return piglet_error(c);
  }
}

PigletStatus piglet_add_post_process(DB db, Node s, Node p, Node o)
{
  try {
    Piglet::Triple t(Piglet::Node((int)s), Piglet::Node(p), Piglet::Node(o));
    return piglet_success(((Piglet::DB *)db)->addPostProcess(&t));
  }
  catch (Piglet::Condition &c) {
    return piglet_error(c);
  }
}

PigletStatus piglet_del(DB db, Node s, Node p, Node o, Node source, bool temporary)
{
  try {
    Piglet::Triple triple(Piglet::Node((int)s), Piglet::Node(p), Piglet::Node(o));
    return piglet_success(((Piglet::DB *)db)->del(&triple, Piglet::Node(source), temporary));
  }
  catch (Piglet::Condition &c) {
    return piglet_error(c);
  }
}

PigletStatus piglet_del_source(DB db, Node source, bool triplesOnly)
{
  try {
    if (triplesOnly)
      return piglet_success(((Piglet::DB *)db)->delSourceTriples(source));
    else
      return piglet_success(((Piglet::DB *)db)->delSource(source));
  }
  catch (Piglet::Condition &c) {
    return piglet_error(c);
  }
}

PigletStatus piglet_load(DB db, Node source, bool append, bool verbose, char* script, char *argv[])
{
  try {
    return piglet_success(((Piglet::DB *)db)->load(Piglet::Node(source), append, verbose,
                                                   script, argv));
  }
  catch (Piglet::Condition &c) {
    return piglet_error(c);
  }
}

PigletStatus piglet_load_m3(DB db, Node source, unsigned char* content, bool verbose)
{
  try {
    return piglet_success(((Piglet::DB *)db)->load(Piglet::Node(source), content, verbose));
  }
  catch (Piglet::Condition &c) {
    return piglet_error(c);
  }
}

char *piglet_info(DB db, Node node, Node *datatype, char *language)
{
  try {
    return ((Piglet::DB *)db)->info(Piglet::Node(node), (Piglet::Node *)datatype, language);
  }
  catch (Piglet::Condition &c) {
    piglet_error(c);
    return NULL;
  }
}

Node piglet_node(DB db, const char *uri)
{
  try {
    if ((uri != NULL) || (*uri == '\0'))
      return id(((Piglet::DB *)db)->node(uri));
    else 
      return id(((Piglet::DB *)db)->node(uri, true));
  }    catch (Piglet::Condition &c) {
      piglet_error(c);
      return 0;
  }
}

Node piglet_literal(DB db, const char *string, Node datatype, const char *lang)
{
  try {
    return id(((Piglet::DB *)db)->literal(string, Piglet::Node(datatype), lang));
  }
  catch (Piglet::Condition &c) {
    piglet_error(c);
    return 0;
  }
}

PigletStatus piglet_augment_literal(DB db, Node literal, Node datatype)
{
  try {
    return piglet_success(((Piglet::DB *)db)->augmentLiteral(Piglet::Node(literal),
                                                             Piglet::Node(datatype)));
  }
  catch (Piglet::Condition &c) {
    return piglet_error(c);
  }
}

int piglet_count(DB db, Node s, Node p, Node o, Node source, bool temporary)
{
  try {
    return ((Piglet::DB *)db)->count(Piglet::Node(s), Piglet::Node(p), Piglet::Node(o),
                                     Piglet::Node(source), temporary);
  }
  catch (Piglet::Condition &c) {
    piglet_error(c);
    return -1;
  }
}

char *piglet_node_tostring(DB db, Node node)
{
  try {
    return ((Piglet::DB *)db)->toString(Piglet::Node(node));
  }
  catch (Piglet::Condition &c) {
    piglet_error(c);
    return NULL;
  }
}

char *piglet_triple_tostring(DB db, Node s, Node p, Node o)
{
  try {
    Piglet::Triple t(Piglet::Node((int)s), Piglet::Node(p), Piglet::Node(o));
    return ((Piglet::DB *)db)->toString(&t);
  }
  catch (Piglet::Condition &c) {
    piglet_error(c);
    return NULL;
  }
}



char *piglet_expand(DB db, const char *qname)
{
  try {
    return ((Piglet::DB *)db)->qName2URI(qname);
  }
  catch (Piglet::Condition &c) {
    piglet_error(c);
    return NULL;
  }
}

char *piglet_expand_m3(DB db, const char *qname)
{
  // Uses a modified qName2URI to avoid unnecessary
  // errors when trying to expand an already
  // expanded URI
  // This is a common case in M3 SIB
  try {
    return ((Piglet::DB *)db)->tryQName2URI_m3(qname);
  }
  catch (Piglet::Condition &c) {
    piglet_error(c);
    return NULL;
  }
}

char *piglet_abbreviate(DB db, const char *uri)
{
  try {
    return ((Piglet::DB *)db)->nodeQName(uri);
  }
  catch (Piglet::Condition &c) {
    piglet_error(c);
    return NULL;
  }
}

PigletStatus piglet_add_namespace(DB db, const char *prefix, const char *uri)
{
  try {
    return piglet_success(((Piglet::DB *)db)->addNamespace(prefix, uri));
  }
  catch (Piglet::Condition &c) {
    return piglet_error(c);
  }
}

PigletStatus piglet_del_namespace(DB db, const char *prefix)
{
  try {
    ((Piglet::DB *)db)->delNamespace(prefix);
    return piglet_success(true);
  }
  catch (Piglet::Condition &c) {
    return piglet_error(c);
  }
}

PigletStatus piglet_match(DB db, const char *pattern, void* userdata, NodeCallback callback)
{
  try {
    CallbackNodeAction action((Piglet::DB *)db, userdata, callback);
    return piglet_success(((Piglet::DB *)db)->match(pattern, &action));
  }
  catch (Piglet::Condition &c) {
    return piglet_error(c);
  }
}

PigletStatus piglet_transaction(DB db)
{
  try {
    return piglet_success(((Piglet::DB *)db)->transaction());
  }
  catch (Piglet::Condition &c) {
    return piglet_error(c);
  }
}

PigletStatus piglet_commit(DB db)
{
  try {
    return piglet_success(((Piglet::DB *)db)->commit());
  }
  catch (Piglet::Condition &c) {
    return piglet_error(c);
  }
}

PigletStatus piglet_rollback(DB db)
{
  try {
    return piglet_success(((Piglet::DB *)db)->rollback());
  }
  catch (Piglet::Condition &c) {
    return piglet_error(c);
  }
}
