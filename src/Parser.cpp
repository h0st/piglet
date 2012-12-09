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
 *  Parser.cpp
 *
 *  Author: Ora Lassila mailto:ora.lassila@nokia.com
 *  Copyright (c) 2001-2008 Nokia. All Rights Reserved.
 */

#include "Parser.h"
#include <cstdio>
#include <iostream>
#include <unistd.h>

namespace Piglet {

Parser::Parser(DB *db)
{
  _source = NULL_NODE;
  _db = db;
}

void Parser::addNamespace(const char *prefix, const char *uri) MAYFAIL
{
  db()->addNamespace(prefix, uri);
}

bool ParserTripleAction::operator()(Node s, Node p, Node o) MAYFAIL
{
  Triple t(s, p, o);
  db()->add(&t, parser()->source());
  return true;
}

bool Parser::parseFromScript(Node base, const char *path, char *argv[]) MAYFAIL
{
  int fd[2];
  pipe(fd);
  if (fork() == 0) { // new process, will execv the actual script
    close(fd[0]);
    dup2(fd[1], 1); // connect pipe to stdout of the new process
    return (execv(path, argv) != -1);
  }
  else { // current process, will read results of the script
    close(fd[1]);
    FILE *input = fdopen(fd[0], "r");
    bool result = parse(base, input);
    close(fd[0]);
    return result;
  }
}

}
