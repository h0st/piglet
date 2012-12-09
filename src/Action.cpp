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
 *  Action.cpp
 *
 *  Author: Ora Lassila mailto:ora.lassila@nokia.com
 *  Copyright (c) 2001-2008 Nokia. All Rights Reserved.
 */

#include <iostream>
#include "Action.h"

namespace Piglet {

bool TripleAction::operator()(Node s, Node p, Node o) MAYFAIL
{
  return operator()(new Triple(s, p, o));
}

bool DebugTripleAction::operator()(Triple *t) MAYFAIL
{
  std::cerr << t << "\n";
  delete t;
  return true;
}

bool Triples::operator()(Triple *t) MAYFAIL
{
  push_back(t);
  return true;
}

bool Nodes::operator()(Node n) MAYFAIL
{
  push_back(n);
  return true;
}

TripleSelectorAction::TripleSelectorAction(DB *db, Element element, NodeAction *action)
: TripleAction(db)
{
  _element = element;
  _action = action;
}

bool TripleSelectorAction::operator()(Node s, Node p, Node o) MAYFAIL
{
  switch (_element) {
    case ELEM_S: return (*_action)(s);
    case ELEM_P: return (*_action)(p);
    case ELEM_O: return (*_action)(o);
    default: FAIL("Unknown triple element specifier");
  }
}

bool TripleSelectorAction::operator()(Triple *triple) MAYFAIL
{
  bool result;
  switch (_element) {
    case ELEM_S: result = (*_action)(triple->s()); break;
    case ELEM_P: result = (*_action)(triple->p()); break;
    case ELEM_O: result = (*_action)(triple->o()); break;
    default: FAIL("Unknown triple element specifier");
  }
  delete triple;
  return result;
}

}
