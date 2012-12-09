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
 *  Action.h
 *
 *  Author: Ora Lassila mailto:ora.lassila@nokia.com
 *  Copyright (c) 2001-2008 Nokia. All Rights Reserved.
 */

#pragma once

#include <list>
#include "Node.h"
#include "Triple.h"
#include "Condition.h"

namespace Piglet {

class DB;

class Action {
public:
  Action(DB *db) { _db = db; }
  virtual ~Action(void) {}
  DB *db(void) const { return _db; }
private:
  DB *_db;
};

class NodeAction : public Action {
public:
  NodeAction(DB *db) : Action(db) {}
  virtual bool operator()(Node n) MAYFAIL = 0;
};

class TripleAction : public Action {
public:
  TripleAction(DB *db) : Action(db) {}
  virtual bool operator()(Node s, Node p, Node o) MAYFAIL;
  virtual bool operator()(Triple *t) MAYFAIL = 0;
};

class DebugTripleAction : public TripleAction {
public:
  DebugTripleAction(DB *db) : TripleAction(db) {}
  virtual bool operator()(Triple *t) MAYFAIL;
};

class Triples : public TripleAction, public std::list<Triple *> {
public:
  Triples(DB *db) : TripleAction(db), std::list<Triple *>() {}
  virtual bool operator()(Triple *t) MAYFAIL;
};

class Nodes : public NodeAction, public std::list<Node> {
public:
  Nodes(DB *db) : NodeAction(db), std::list<Node>() {}
  virtual bool operator()(Node n) MAYFAIL;
};

class TripleSelectorAction : public TripleAction {
public:
  enum Element { ELEM_S, ELEM_P, ELEM_O };
  TripleSelectorAction(DB *db, Element element, NodeAction *action);
  bool operator()(Node s, Node p, Node o) MAYFAIL;
  bool operator()(Triple *triple) MAYFAIL;
private:
  Element _element;
  NodeAction *_action;
};

class GenericAction : public Action {
public:
  GenericAction(DB *db) : Action(db) {}
  virtual bool operator()(int argc, char **argv, char **cols) = 0;
};

}
