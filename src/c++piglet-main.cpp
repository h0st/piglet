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
 *  c++piglet-main.c
 *
 *  Author: Ora Lassila mailto:ora.lassila@nokia.com
 *  Copyright (c) 2001-2008 Nokia. All Rights Reserved.
 *
 *  Sample program making use of Piglet's C++ API.
 */

#include "piglet.h"

using namespace Piglet;

class ShowNode : public NodeAction {
public:
  ShowNode(DB *db) : NodeAction(db) {}
  bool operator()(Node n) MAYFAIL { std::cout << n << "\n"; return true; }
};

class ShowTriple : public TripleAction {
public:
  ShowTriple(DB *db) : TripleAction(db) {}
  bool operator()(Triple *t) MAYFAIL;
};

bool ShowTriple::operator()(Triple *t) MAYFAIL
{
  std::cout << t->o()._id << t->p()._id << t->s()._id << "\n";
  delete t;
  return true;
}

int main(int argc, char *argv[])
{

  Piglet::DB db(argv[1], true);
  db.load("http://www.w3.org/1999/02/22-rdf-syntax-ns#", true);
  db.load("http://www.w3.org/2000/01/rdf-schema#", true);
  ShowTriple show(&db);
  db.query(db.node(db.qName2URI("rdf:type")), NULL_NODE, NULL_NODE, NULL_NODE, &show);
  exit(0);
}
