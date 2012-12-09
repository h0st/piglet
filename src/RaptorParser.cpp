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
 *  RaptorParser.cpp
 *
 *  Author: Ora Lassila mailto:ora.lassila@nokia.com
 *  Copyright (c) 2001-2008 Nokia. All Rights Reserved.
 */

#include "RaptorParser.h"
#include "Useful.h"

namespace Piglet {

static void parser_error_handler(Parser *parser, raptor_locator* locator, const char *message)
{
  parser->terminate(message);
}

// Replicated here because the definition of raptor_namespace_s is not in the "public" headers
typedef struct {
  void *next;
  void *nstack;
  const char *prefix;
  int prefix_length;
  raptor_uri *uri;
  // ...and other stuff we do not care about
} faux_raptor_namespace;

static void parser_namespaces_handler(Parser *parser, faux_raptor_namespace *nspace)
{
  parser->addNamespace((char *)nspace->prefix, (char *)raptor_uri_as_string(nspace->uri));
}

static void parser_triples_handler(ParserTripleAction *action, const raptor_statement* triple)
{
  Node s = NULL_NODE;
  switch (triple->subject_type) {
    case RAPTOR_IDENTIFIER_TYPE_RESOURCE:
      s = action->db()->node((char *)(triple->subject));
      break;
    case RAPTOR_IDENTIFIER_TYPE_ANONYMOUS:
      s = action->db()->node((char *)(triple->subject), true);
      break;
    default:
      FAIL("Unhandled subject_type");
  }
  if (s == NULL_NODE) FAIL("NULL subject in triple");
  Node p = action->db()->node((char *)(triple->predicate));
  Node o = NULL_NODE;
  Node dt = NULL_NODE;
  switch (triple->object_type) {
    case RAPTOR_IDENTIFIER_TYPE_RESOURCE:
      o = action->db()->node((char *)(triple->object));
      break;
    case RAPTOR_IDENTIFIER_TYPE_LITERAL:
      if (triple->object_literal_datatype)
        dt = action->db()->node((char *)(triple->object_literal_datatype));
      o = action->db()->literal((char *)(triple->object), dt,
                                (char *)triple->object_literal_language);
      break;
    case RAPTOR_IDENTIFIER_TYPE_ANONYMOUS:
      o = action->db()->node((char *)(triple->object), true);
      break;
    default:
      FAIL("Unhandled object_type");
  }
  if (o == NULL_NODE) FAIL("NULL object in triple");
  (*action)(s, p, o);
}

RaptorParser::RaptorParser(DB *db) : Parser(db)
{
  nativeParser = raptor_new_parser_for_content(NULL, "application/rdf+xml", NULL, 0, NULL);
  raptor_set_error_handler(nativeParser, this, (raptor_message_handler)parser_error_handler);
  raptor_set_namespace_handler(nativeParser, this,
                               (void (*)(void *, raptor_namespace *))parser_namespaces_handler);
  tripleAction = new ParserTripleAction(this, db);
  raptor_set_statement_handler(nativeParser, tripleAction,
                               (raptor_statement_handler)parser_triples_handler);
  raptor_set_feature(nativeParser, RAPTOR_FEATURE_SCANNING, 1);
  raptor_set_feature(nativeParser, RAPTOR_FEATURE_ALLOW_NON_NS_ATTRIBUTES, 1);
  // raptor_parser_set_feature_string(nativeParser, RAPTOR_FEATURE_WWW_HTTP_USER_AGENT,
  //                                 (const unsigned char *)"Piglet 1.0");
}

RaptorParser::~RaptorParser(void)
{
  if (tripleAction)
    delete tripleAction;
  if (nativeParser)
    raptor_free_parser(nativeParser);
}

bool RaptorParser::parse(Node source) MAYFAIL
{
  _terminated = false;
  _source = source;
  TemporaryString u(db()->info(source));
  raptor_uri *uri = raptor_new_uri((unsigned char *)u.string());
  int result = raptor_parse_uri(nativeParser, uri, NULL);
  raptor_free_uri(uri);
  return (result == 0);
}

bool RaptorParser::parse(Node source, FILE *stream) MAYFAIL
{
  _terminated = false;
  _source = source;
  TemporaryString u(db()->info(source));
  raptor_uri *uri = raptor_new_uri((unsigned char *)u.string());
  int result = raptor_parse_file_stream(nativeParser, stream, NULL, uri);
  raptor_free_uri(uri);
  return (result == 0);
}


  /*
    Parse a string containing RDFXML content
    Needed for M3 RDF/XML support
   */
bool RaptorParser::parse(Node source, unsigned char *content) MAYFAIL
{
  _terminated = false;
  _source = source;
  size_t length = strlen((const char *)content);
  TemporaryString u(db()->info(source));
  raptor_uri *uri = raptor_new_uri((unsigned char *)u.string());
  raptor_start_parse(nativeParser, uri);
  int result = raptor_parse_chunk(nativeParser, content, length, 1);
  raptor_free_uri(uri);
  return (result == 0);
}

void RaptorParser::terminate(const char *message)
{
  // This is a bit of a hack, but gets us through some common broken schemata
  if (strcmp(message, "Using an element 'RDF' without a namespace is forbidden.") != 0) {
    _terminated = true;
    std::cout << "Parser terminated with message\n" << message;
    raptor_parse_abort(nativeParser);
  }
}

void RaptorParser::init(void)
{
  raptor_init();
}

void RaptorParser::finish(void)
{
  raptor_finish();
}

}
