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
 *  cpiglet.h
 *
 *  Author: Ora Lassila mailto:ora.lassila@nokia.com
 *  Copyright (c) 2001-2008 Nokia. All Rights Reserved.
 */

#pragma once

#include <stdio.h>

#if defined(__cplusplus)
#else
typedef unsigned short bool;
#define true 1
#define false 0
#endif

/*!
@typedef    Node
@abstract   represents resource and literal nodes in an RDF graph
*/
typedef int Node;

typedef void *DB;

typedef bool (*TripleCallback)(DB db, void *userdata, Node s, Node p, Node o);

typedef bool (*NodeCallback)(DB db, void *userdata, Node node);

typedef enum { PigletFalse, PigletTrue, PigletError } PigletStatus;

extern const char *piglet_error_message;


// Open triple store or create a new one, then make it "current"
DB piglet_open(char *name);

// Close triple store
PigletStatus piglet_close(DB db);

// Query for triples
PigletStatus piglet_query(DB db, Node s, Node p, Node o, Node source, void* userdata, TripleCallback callback);

// Query for triple sources
PigletStatus piglet_sources(DB db, Node s, Node p, Node o, void *userdata, NodeCallback callback);

// Count triples in a triple store
int piglet_count(DB db, Node s, Node p, Node o, Node source, bool temporary);

// Add triple to triple store
PigletStatus piglet_add(DB db, Node s, Node p, Node o, Node source, bool temporary);

// Do simple post processing for the RDF++ reasoner
PigletStatus piglet_add_post_process(DB db, Node s, Node p, Node o);

// Remove triple from triple store
PigletStatus piglet_del(DB db, Node s, Node p, Node o, Node source, bool temporary);

// Remove an entire source from triple store
PigletStatus piglet_del_source(DB db, Node source, bool triplesOnly);

// Load triples from source node's URL
PigletStatus piglet_load(DB db, Node source, bool append, bool verbose, char* script, char *argv[]);

// Load triples from string
PigletStatus piglet_load_m3(DB db, Node source, unsigned char* content, bool verbose);

// Return the URI of node (or string if node is a literal)
char *piglet_info(DB db, Node node, Node *datatype, char *language);

// Return Node for given URI
Node piglet_node(DB db, const char *uri);

// Return literal Node for given string, datatype (or 0), xml:lang (or NULL)
Node piglet_literal(DB db, const char *string, Node datatype, const char *lang);

// ...
PigletStatus piglet_augment_literal(DB db, Node literal, Node datatype);

// Get Wilbur representation of node as a string
char *piglet_node_tostring(DB db, Node node);

// Get Wilbur debugging representation of triple as a string
char *piglet_triple_tostring(DB db, Node s, Node p, Node o);

// Expand a QName into a URI string
char *piglet_expand(DB db, const char *qname);

// Expand a QName into a URI string
// Returns the current QName if no abbreviated namespace found
char *piglet_expand_m3(DB db, const char *qname);

// "Reverse expand" a URI into a QName, if possible
char *piglet_abbreviate(DB db, const char *uri);

// Add a namespace prefix for a given URI
PigletStatus piglet_add_namespace(DB db, const char *prefix, const char *uri);

// Delete a namespace
PigletStatus piglet_del_namespace(DB db, const char *prefix);

// Match node URIs and literal strings
PigletStatus piglet_match(DB db, const char *pattern, void* userdata, NodeCallback callback);

// ...
PigletStatus piglet_transaction(DB db);

// ...
PigletStatus piglet_commit(DB db);

// ...
PigletStatus piglet_rollback(DB db);
