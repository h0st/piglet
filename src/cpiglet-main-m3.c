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
 *  cpiglet-main.c
 *
 *  Author: Ora Lassila mailto:ora.lassila@nokia.com
 *  Copyright (c) 2001-2008 Nokia. All Rights Reserved.
 *
 *  Sample program making use of Piglet's C API.
 */

#include "cpiglet.h"
#include <stdlib.h>

bool callback(DB db, void *userdata, Node s, Node p, Node o)
{
  printf("%s\n", piglet_triple_tostring(db, s, p, o));
  return true;
}

int main(int argc, char *argv[])
{
  DB db = piglet_open(argv[1]);

  char* c ="<?xml version=\"1.0\"?><rdf:RDF xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\" xmlns:contact=\"http://www.w3.org/2000/10/swap/pim/contact#\"><contact:Person rdf:about=\"http://www.w3.org/People/EM/contact#me\"><contact:fullName>Eric Miller</contact:fullName><contact:mailbox rdf:resource=\"mailto:em@w3.org\"/><contact:personalTitle>Dr.</contact:personalTitle></contact:Person></rdf:RDF>\0";
  printf("Parsed string is\n%s\n", c);
  if (db) {
    piglet_load_m3(db, piglet_node(db, "http://www.m3.com/TestSS#"), c, true);
    piglet_query(db, piglet_node(db, "http://www.m3.com/TestSS#"), 0, 0, 0, NULL, callback);
    piglet_close(db);
  }
  exit(0);
}



