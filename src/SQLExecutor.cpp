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
 * SQLExecutor.cpp
 *
 *  Created on: Jun 4, 2009
 *      Author: skiminki
 */

#include <string>

#include <sqlite3.h>

#include "Condition.h"
#include "DB.h"
#include "SQLExecutor.h"

namespace {

using namespace Piglet;

class SQLite3Result : public SQLResult
{
protected:
  sqlite3_stmt *stmt;
  bool valid;

public:
  std::vector<std::string> columnLabels;


  SQLite3Result(sqlite3_stmt *_stmt) : stmt(_stmt), valid(true)
  {
  }

  bool isValid()
  {
    return valid;
  }

  void prepare()
  {
    columnLabels.clear();

    int cols=sqlite3_column_count(stmt);
    columnLabels.resize(cols);
    for (int col=0; col<cols; ++col) {
      const char *name=sqlite3_column_name(stmt, col);
      columnLabels[col]=name;
    }
  }

  const std::vector<std::string> &getColumnNames()
  {
    return columnLabels;
  }


  bool nextRow()
  {
    if (!isValid()) throw Condition("Result object is not valid");
    int result=sqlite3_step(stmt);
    if (result==SQLITE_ROW) return true; // got a row!
    if (result==SQLITE_DONE)
    {
      valid=false;
      return false; // ok, all rows already returned
    }

    // error condition
    sqlite3 *dbhandle=static_cast<sqlite3 *>(DB::current()->getDatabase()->getDbHandle());
    throw Condition("Error retrieving row: %s (%d)", sqlite3_errmsg(dbhandle), result);
  }

  void reexecute()
  {
    int result=sqlite3_reset(stmt);
    if (result==SQLITE_OK)
    {
      valid=true;
    }
    else {
      sqlite3 *dbhandle=static_cast<sqlite3 *>(DB::current()->getDatabase()->getDbHandle());
      throw Condition("Error resetting statement: %s (%d)", sqlite3_errmsg(dbhandle), result);
    }
  }


  bool isNull(int col)
  {
    return (sqlite3_column_type(stmt, col)==SQLITE_NULL);
  }

  const char *getString(int col)
  {
    return (const char *)sqlite3_column_text(stmt, col);
  }

  void close()
  {
    int result=sqlite3_finalize(stmt);

    if (result!=SQLITE_OK)
    {
      sqlite3 *dbhandle=static_cast<sqlite3 *>(DB::current()->getDatabase()->getDbHandle());
      throw Condition("Error finalizing statement: %s (%d)", sqlite3_errmsg(dbhandle), result);
    }

    stmt=0;
  }

};

}


namespace Piglet
{

SQLResult::~SQLResult() {}



SQLResult *SQLQueryExecutor::execute(const SQLQuery &sqlQuery)
{
  if (!DB::current())
  {
    throw Condition("Current database not open");
  }
  sqlite3 *dbhandle=static_cast<sqlite3 *>(DB::current()->getDatabase()->getDbHandle());
  sqlite3_stmt *stmt=0;
  int result=sqlite3_prepare_v2(dbhandle, sqlQuery.sqlQuery.c_str(), -1, &stmt, NULL);
  if (result!=SQLITE_OK)
    throw Condition("Error preparing query: %s (%d). Query string was '%s'", sqlite3_errmsg(dbhandle), result, sqlQuery.sqlQuery.c_str());
  SQLite3Result *sqlResult=new SQLite3Result(stmt);
  try {
    sqlResult->prepare();
    return sqlResult;
  } catch (...) {
    delete sqlResult;
    throw;
  }
}

}
