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
 *  SQL.cpp
 *
 *  Author: Ora Lassila mailto:ora.lassila@nokia.com
 *  Copyright (c) 2001-2008 Nokia. All Rights Reserved.
 */

#include "SQL.h"
#include <sqlite3.h>
#include <iostream>

namespace SQL {

TemporaryString::~TemporaryString(void)
{
  if (_string) {
    sqlite3_free(_string);
    _string = NULL;
  }
}

Database::Database(const char *name, bool debug)
{
  _debug = debug;
  if (sqlite3_open(name, (sqlite3 **)&_database) != SQLITE_OK)
    _database = NULL;
}

Database::~Database(void)
{
  if ((_database != NULL) && (sqlite3_close((sqlite3 *)_database) == SQLITE_OK))
    _database = NULL;
}

int oneIntCallback(int *value, int argc, char **argv, char **cols)
{
  if (argv[0])
    *value = atoi(argv[0]);
  return 0;
}

int oneStringCallback(char **str, int argc, char **argv, char **cols)
{
  if (argv[0])
    *str = strdup(argv[0]);
  return 0;
}

Database::Status Database::exec(const char *query, void *arg, Callback callback, char **msg)
{
  Status status = FAILURE;
  if (_database != NULL) {
    if (_debug)
      std::cout << "Query:  " << query << "\n";
    switch (sqlite3_exec((sqlite3 *)_database, query, callback, arg, msg)) {
      case SQLITE_OK:    status = OK; break;
      case SQLITE_ABORT: status = ABORT; break;
    }
    if (_debug) {
      if (status == FAILURE) {
        std::cout << "Status: " << msg << "\n";
        free(msg);
      }
      else std::cout << "Status: OK\n";
    }
  }
  return status;
}

char *query(const char *format, ...)
{
  va_list args;
  va_start(args, format);
  char *s = sqlite3_vmprintf(format, args);
  va_end(args);
  return s;
}

}
