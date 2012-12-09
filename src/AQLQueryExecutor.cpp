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
 * AQLQueryExecutor.cpp
 *
 *  Created on: Jun 4, 2009
 *      Author: skiminki
 */

#include "AQLQueryExecutor.h"

#include "AQLModel.h"
#include "SQLExecutor.h"

namespace {

using namespace Piglet;

struct AQLSQLResult : AQLResult {

private:
  std::vector<std::string> header;
  SQLResult *sqlResult;
  std::vector<AQLResultItem *> currentRow;
  bool nextRowExists;

public:
  AQLSQLResult(std::vector<std::string> &_header, SQLResult *_sqlResult) :
    header(_header), sqlResult(_sqlResult), currentRow(_header.size())
  {
    advanceSqlResult();
  }

  ~AQLSQLResult()
  {
    sqlResult->close();
    delete sqlResult;

    // release current row items
    for (size_t i=0; i<header.size(); ++i) delete currentRow[i];
  }

  const std::vector<std::string> &getHeader()
  {
    return header;
  }

  bool hasNextRow()
  {
    return nextRowExists;
  }

  const std::vector<AQLResultItem *> &nextRow()
  {
    loadCurrentRowFromSqlResult();
    advanceSqlResult();
    return currentRow;
  }

protected:
  void loadCurrentRowFromSqlResult()
  {
    for (size_t i=0; i<header.size(); ++i)
    {
      AQLResultItem *&cell=currentRow[i];
      delete cell;

      if (!sqlResult->isNull(i))
      {
        cell=new AQLResultItem;
        cell->value=sqlResult->getString(i);
      }
      else {
        // null value
        cell=0;
      }
    }
  }

  void advanceSqlResult()
  {
    nextRowExists=sqlResult->nextRow();
  }

};

}

namespace Piglet
{

AQLResult *AQLQueryExecutor::executeQueryWithPreformedResult(AQLQuery &aqlQuery, SQLResult *sqlResult)
{
  std::vector<std::string> header;
  for (AQLQuery::select_list_type::iterator i=aqlQuery.selects.begin();
       i!=aqlQuery.selects.end(); ++i)
  {
    AQLSelect *select=*i;
    header.push_back(select->label);
  }

  AQLSQLResult *ret=new AQLSQLResult(header, sqlResult);

  return ret;
}


}
