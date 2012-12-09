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
 * AQLToSQLTranslator.cpp
 *
 *  Created on: May 27, 2009
 *      Author: skiminki
 */

#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <limits>
#include <map>
#include <set>
#include <string>

#include "AQLToSQLTranslator.h"
#include "AQLSupport.h"
#include "Condition.h"
#include "DB.h"
#include "SQLExecutor.h"

namespace {
using namespace Piglet;

struct FunctionMapEntry
{
  const char *aqlFunctionName;
  const char *sqlFunctionName;
  std::string (*formatFunction)(void *, const std::list<std::string> &args);
  void *formatFunctionArg; // first arg of format function
};

std::string sqlFunctionFormatter(void *functionName, const std::list<std::string> &args)
{
  std::string ret=static_cast<const char *>(functionName);
  ret.push_back('(');
  bool first=true;
  for (std::list<std::string>::const_iterator i=args.begin(); i!=args.end(); ++i) {
    if (!first) ret+=", ";
    ret+=*i;
    first=false;
  }
  ret.push_back(')');
  return ret;
}

std::string sqlConcatenateFormatter(void *, const std::list<std::string> &args)
{
  if (args.empty()) return "''";
  bool first=true;
  std::string ret;
  ret+='(';
  for (std::list<std::string>::const_iterator i=args.begin(); i!=args.end(); ++i) {
    if (!first) ret+=" || ";
    ret+='(';
    ret+=*i;
    ret+=')';
    first=false;
  }
  ret+=')';
  return ret;
}

FunctionMapEntry sqlite3FunctionMap[]=
{
    // NOTE: THESE MUST BE IN ALPHABETICAL ORDER

    { "abs", "abs" },
    { "coalesce", "coalesce" },
    { "concatenate", 0, sqlConcatenateFormatter, 0 },
    { "length", "length" },
    { "random", "random" },
    { "to-lower", "lower" },
    { "to-upper", "upper" },
    { "type-of", "typeof" },
};
// comparator for std::find
bool sqlite3FunctionMapLessThan(const FunctionMapEntry &i, const FunctionMapEntry &j)
{
  return strcmp(i.aqlFunctionName, j.aqlFunctionName) < 0;
}
const FunctionMapEntry *findSqlite3Function(const char *aqlName)
{
  const FunctionMapEntry *begin=&sqlite3FunctionMap[0];
  const FunctionMapEntry *end=&sqlite3FunctionMap[sizeof(sqlite3FunctionMap)/sizeof(sqlite3FunctionMap[0])];

  FunctionMapEntry value;
  value.aqlFunctionName=aqlName;

  const FunctionMapEntry *ret=
    std::lower_bound<const FunctionMapEntry *, const FunctionMapEntry>
      (begin, end, value, sqlite3FunctionMapLessThan);

  if (ret==end) return 0;
  if (strcmp(ret->aqlFunctionName, aqlName)!=0) return 0;
  return ret;
}

// this is used for function -> SQL syntax transformations, such as concatenate(a,b) => a || b
struct FunctionContext
{
  const FunctionMapEntry *function;
  std::list<std::string> arguments;
  std::string oldQueryString;
};

struct TranslatorContext
{
  typedef std::map<std::string, std::string> join_map_type;

  // - this is triple join name -> sql alias map
  // - root join name in rdf 'root'
  // - the SQL table joined (and aliased) is 'triple'
  join_map_type tripleJoinMap;

  // - used node joins per triple join
  std::map<std::string, std::set<AQLPropertyExpr::Property> > usedNodeJoins;

  std::string queryString;

  std::list<FunctionContext> functionContextStack;

  const std::string &ensureTripleJoin(const std::string &tripleJoinName) {
    join_map_type::const_iterator i=tripleJoinMap.find(tripleJoinName);
    if (i!=tripleJoinMap.end()) return i->second;

    char name[16];
    snprintf(name, 16, "t%zu", tripleJoinMap.size());
    name[15]='\0';

    return tripleJoinMap[tripleJoinName]=name;
  }

  static char getCharForNode(AQLPropertyExpr::Property node)
  {
    switch (node)
    {
    case AQLPropertyExpr::SUBJECT:   return 's';
    case AQLPropertyExpr::PREDICATE: return 'p';
    case AQLPropertyExpr::OBJECT:    return 'o';
    default:
      throw Condition("getCharForNode: Internal error, node=%d", int(node));
    }
  }


  std::string getAndUseNodeJoinName(const std::string &tripleJoinName, AQLPropertyExpr::Property node, bool allowCreate) {
    const std::string &tripleJoinAlias=ensureTripleJoin(tripleJoinName);
    char c=TranslatorContext::getCharForNode(node);

    std::string nodeJoinAlias=tripleJoinAlias+'_';
    nodeJoinAlias+=c;

    if (allowCreate)
    {
      usedNodeJoins[tripleJoinName].insert(node);
      return nodeJoinAlias;
    }
    else {
      if (usedNodeJoins[tripleJoinName].find(node)==usedNodeJoins[tripleJoinName].end())
      {
        throw Condition("Internal error: we don't have alias for %s.%c", tripleJoinName.c_str(), c);
      }
      return nodeJoinAlias;
    }

  }

};

/**
 * Escapes SQL-string, but DOES NOT add surrounding '-characters
 */
std::string escapeSqlString(const std::string &string)
{
  std::string ret;
  for (std::string::const_iterator i=string.begin(); i!=string.end(); ++i)
  {
    const char c=*i;
    if (c=='\'')
      ret.push_back(c);
    ret.push_back(c);
  }
  return ret;
}

std::string integerToString(int i)
{
  char buffer[16];
  snprintf(buffer, 16, "%d", i);
  buffer[15]='\0';
  return std::string(buffer);
}

class AQLPropertyAliasVisitor : public AQLOptionalVisitor {
private:
  TranslatorContext &context;

public:
  AQLPropertyAliasVisitor(TranslatorContext &_context) : context(_context) {}

  void visit(AQLPropertyExpr &expr)
  {
    context.getAndUseNodeJoinName(expr.joinName, expr.property, true);
  }

};

class AQLPropertyToPropertyReferenceVisitor : public AQLOptionalVisitor {
public:

  void visitBeforeChildren(AQLComparisonCriterion &c)
  {
    if (dynamic_cast<AQLPropertyExpr *>(c.left) && dynamic_cast<AQLPropertyExpr *>(c.right)) {
      // left and right sides are both properties, so streamline this to use references instead of real values

      AQLPropertyReferenceExpr *left=new AQLPropertyReferenceExpr;
      left->joinName=static_cast<AQLPropertyExpr *>(c.left)->joinName;
      left->property=static_cast<AQLPropertyExpr *>(c.left)->property;

      AQLPropertyReferenceExpr *right=new AQLPropertyReferenceExpr;
      right->joinName=static_cast<AQLPropertyExpr *>(c.right)->joinName;
      right->property=static_cast<AQLPropertyExpr *>(c.right)->property;

      delete c.left;
      delete c.right;

      c.left=left;
      c.right=right;
    }
  }

};


class AQLToSQLVisitor : public AQLOptionalVisitor {
private:
  TranslatorContext &context;
  int selects;

  std::string getSqlAliasForNode(const std::string &tripleJoin, AQLPropertyExpr::Property node, bool allowCreate=false)
  {
    std::string ret;
    ret+=context.getAndUseNodeJoinName(tripleJoin, node, allowCreate);
    ret+=".str";
    return ret;
  }

  void writeUsedNodeJoins(const std::string &tripleJoin, AQLJoin::JoinType joinType)
  {
    if (context.usedNodeJoins.find(tripleJoin)==context.usedNodeJoins.end()) return;
    const std::set<AQLPropertyExpr::Property> &usedSet=context.usedNodeJoins.at(tripleJoin);
    for (std::set<AQLPropertyExpr::Property>::const_iterator i=usedSet.begin();
         i!=usedSet.end(); ++i)
    {
      AQLPropertyExpr::Property node=*i;
      char c=TranslatorContext::getCharForNode(node);

      context.queryString+="\n     ";
      if (joinType==AQLJoin::LEFT_OUTER)
      {
        context.queryString+="LEFT ";
      }
      else {
        context.queryString+="INNER";
      }
      context.queryString+=" JOIN node AS ";
      context.queryString+=context.tripleJoinMap.at(tripleJoin);
      context.queryString.push_back('_');
      context.queryString.push_back(c);
      context.queryString+=" ON (";
      context.queryString+=context.tripleJoinMap.at(tripleJoin);
      context.queryString.push_back('.');
      context.queryString.push_back(c);
      context.queryString.push_back('=');
      context.queryString+=context.tripleJoinMap.at(tripleJoin);
      context.queryString.push_back('_');
      context.queryString.push_back(c);
      context.queryString+=".id)";
    }
  }

public:
  AQLToSQLVisitor(TranslatorContext &_context) : context(_context) {}

  void visitBeforeSelects(AQLQuery &)
  {
    context.queryString+="SELECT";
    selects=0;
  }

  void visitAfterSelects(AQLQuery &)
  {
    context.queryString+="\n  FROM triple AS ";
    context.queryString+=context.tripleJoinMap.at("root");
    writeUsedNodeJoins(std::string("root"), AQLJoin::INNER);
  }

  void visitBeforeChildren(AQLJoin &join)
  {
    switch (join.joinType)
    {
    case AQLJoin::LEFT_OUTER:
      context.queryString+="\n  LEFT  JOIN triple AS ";
      break;
    case AQLJoin::INNER:
      context.queryString+="\n  INNER JOIN triple AS ";
      break;
    }
    context.queryString+=context.tripleJoinMap.at(join.name);
    if (join.criterion) {
      context.queryString+=" ON (";
    }
  }

  void visitAfterChildren(AQLJoin &join)
  {
    if (join.criterion) {
      context.queryString+=")";
    }
    writeUsedNodeJoins(join.name, join.joinType);
  }

  void visitBeforeChildren(AQLSelect &)
  {
    if (selects==0)
      context.queryString+=' ';
    else
      context.queryString+=", ";
  }
  void visitAfterChildren(AQLSelect &)
  {
    char num[16];
    context.queryString+=" AS col";
    snprintf(num, 16, "%d", selects);
    num[15]='\0';
    context.queryString+=num;
    ++selects;
  }


  void visitBeforeCriterion(AQLQuery &q)
  {
    if (q.criterion) context.queryString+="\n WHERE ";
  }

  void visit(AQLPropertyExpr &expr)
  {
    context.queryString+=getSqlAliasForNode(expr.joinName, expr.property);
  }
  void visit(AQLPropertyReferenceExpr &expr)
  {
    char c=TranslatorContext::getCharForNode(expr.property);
    context.queryString+=context.tripleJoinMap.at(expr.joinName);
    context.queryString.push_back('.');
    context.queryString.push_back(c);
  }
  void visit(AQLLiteralExpr &expr)
  {
    context.queryString.push_back('\'');
    context.queryString+=escapeSqlString(expr.stringLiteral);
    context.queryString.push_back('\'');
  }
  void visitBeforeChildren(AQLFunctionExpr &expr)
  {
    const FunctionMapEntry *function=findSqlite3Function(expr.functionName.c_str());
    if (!function) throw Condition("Unknown function: %s", expr.functionName.c_str());

    // push out function context
    FunctionContext functionContext;
    functionContext.function=function;
    functionContext.oldQueryString=context.queryString;
    context.queryString.clear();

    context.functionContextStack.push_back(functionContext);
  }
  void visitBetweenChildren(AQLFunctionExpr &, int)
  {
    context.functionContextStack.back().arguments.push_back(context.queryString);
    context.queryString.clear();
  }
  void visitAfterChildren(AQLFunctionExpr &fexpr)
  {
    if (!fexpr.arguments.empty())
      context.functionContextStack.back().arguments.push_back(context.queryString);

    FunctionContext &functionContext=context.functionContextStack.back();

    context.queryString=functionContext.oldQueryString;

    if (!functionContext.function->formatFunction)
    {
      // standard function
      context.queryString+=sqlFunctionFormatter(const_cast<char *>(functionContext.function->sqlFunctionName), functionContext.arguments);
    }
    else {
      context.queryString+=functionContext.function->formatFunction(functionContext.function->formatFunctionArg, functionContext.arguments);
    }
    context.functionContextStack.pop_back();
  }
  void visitBeforeChildren(AQLComparisonCriterion &)
  {
    context.queryString+="(";
  }
  void visitBetweenChildren(AQLComparisonCriterion &comparison)
  {
    switch (comparison.comparisonType)
    {
    case AQLComparisonCriterion::EQUAL:     context.queryString+="="; break;
    case AQLComparisonCriterion::NOT_EQUAL: context.queryString+="<>"; break;
    default:
      throw Condition("AQLToSQLTranslator: Internal error, comparisonType=%d", int(comparison.comparisonType));
    }
  }
  void visitAfterChildren(AQLComparisonCriterion &)
  {
    context.queryString+=")";
  }

  void visitBeforeChildren(AQLNotExpression &)
  {
    context.queryString+="NOT ";
  }

  void visitBeforeChildren(AQLJunctionCriterion &junction)
  {
    context.queryString+="(";
    if (junction.terms.empty())
    {
      switch (junction.junctionType)
      {
      case AQLJunctionCriterion::CONJUNCTION: context.queryString+="1=1"; break;
      case AQLJunctionCriterion::DISJUNCTION: context.queryString+="0=1"; break;
      default:
        throw Condition("AQLToSQLTranslator: Internal error, junctionType=%d", int(junction.junctionType));
      }
    }
  }
  void visitBetweenChildren(AQLJunctionCriterion &junction, int pos)
  {
    switch (junction.junctionType)
    {
    case AQLJunctionCriterion::CONJUNCTION: context.queryString+=" AND "; break;
    case AQLJunctionCriterion::DISJUNCTION: context.queryString+=" OR "; break;
    default:
      throw Condition("AQLToSQLTranslator: Internal error, junctionType=%d", int(junction.junctionType));
    }
  }
  void visitAfterChildren(AQLJunctionCriterion &)
  {
    context.queryString+=")";
  }
  void visitAfterChildren(AQLSort &sort)
  {
    if (sort.ascending)
      context.queryString+=" ASC";
    else
      context.queryString+=" DESC";
  }
  void visitBeforeSorts(AQLQuery &query)
  {
    if (!query.sorts.empty())
      context.queryString+="\nORDER BY ";
  }
  void visitBetweenSorts(AQLQuery &, int)
  {
    context.queryString+=", ";
  }
  void visitAfterChildren(AQLQuery &query)
  {
    if (query.maxRows >=0 || query.rowOffset >= 0)
    {
      context.queryString+="\nLIMIT ";
      if (query.rowOffset>=0)
      {
        context.queryString+=integerToString(query.rowOffset);
        context.queryString+=", ";
      }
      if (query.maxRows>=0)
      {
        context.queryString+=integerToString(query.maxRows);
      }
      else {
        context.queryString+=integerToString(std::numeric_limits<int>::max());
      }
    }
  }
};

}

namespace Piglet {

AQLToSQLTranslator::AQLToSQLTranslator()
{

}

AQLToSQLTranslator::~AQLToSQLTranslator()
{

}

void AQLToSQLTranslator::optimize(AQLQuery &aql)
{
  AQLPropertyToPropertyReferenceVisitor v;
  aql.accept(v);
}

SQLQuery *AQLToSQLTranslator::translateToSql(AQLQuery &aql, bool optimizeQuery)
{
  if (optimizeQuery) optimize(aql);

  TranslatorContext context;

  context.ensureTripleJoin("root");
  for (AQLQuery::join_list_type::iterator i=aql.joins.begin();
       i!=aql.joins.end(); ++i)
  {
    AQLJoin *join=*i;
    context.ensureTripleJoin(join->name);
  }

  AQLPropertyAliasVisitor propertyAliasVisitor(context);
  aql.accept(propertyAliasVisitor);

  AQLToSQLVisitor visitor(context);

  aql.accept(visitor);

  SQLQuery *query=new SQLQuery();
  query->sqlQuery=context.queryString;

  return query;

}


}



