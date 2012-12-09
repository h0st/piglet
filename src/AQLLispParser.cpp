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
 * AQLLispParser.cpp
 *
 *  Author: Sami Kiminki, skiminki@users.sourceforge.net
 */

#include <cassert>
#include <cerrno>
#include <cstdlib>
#include <cstdio>
#include <list>
#include <limits>
#include <iostream>

#include "AQLModel.h"
#include "AQLLispParser.h"
#include "Condition.h"
#include "Useful.h"

#define DEBUG_AQL_PARSER (1)

#ifdef DEBUG_AQL_PARSER
#define DEBUG(a) a
#else
#define DEBUG(a)
#endif

namespace {

using namespace Piglet;

std::string possiblyEscape(char c)
{
  if (unsigned(c)<0x20 || unsigned(c)>=0x7F)
  {
    std::string s="\\x";
    s+=intToHexString(unsigned(c), 2);
    return s;
  }
  else {
    std::string s;
    s+=c;
    return s;
  }
}

bool isWhiteSpace(int i)
{
  switch (i)
  {
  case '\n':
  case '\r':
  case ' ':
  case '\t':
    return true;
  default:
    return false;
  }
}

bool isKeywordCharacter(int c)
{
  if (isWhiteSpace(c)) return false;
  switch (c)
  {
  case '(':
  case ')':
    return false;
  default:
    return true;
  }
}

bool isStringCharacter(int c)
{
  return c>=0x20 && c!='"';
}

class AQLParserException : public Condition {
public:
  AQLParserException(int line, int col, const char *message) : Condition("Line %d column %d: %s", line, col, message)  {}
  AQLParserException(int line, int col, const std::string &message) : Condition("Line %d column %d: %s", line, col, message.c_str())  {}
};


class AQLLispParserImpl
{
protected:
  std::istream &is;
  int line;
  int col;
  char lastchar; // used to detect "\r\n" and "\n\r" patterns when updating line

  const int EOF_CHAR;  // = std::istream::traits_type::eof()



public:
  AQLLispParserImpl(std::istream &_is) : is(_is), line(1), col(1),  EOF_CHAR(std::istream::traits_type::eof())
  {
  }

protected:
  int peek()
  {
    return is.peek();
  }

  char get()
  {
    int c = is.get();
    expectNotEof();

    if ((c == '\r' && lastchar != '\n') || (c == '\n' && lastchar != '\r'))
    {
      // newline
      ++line;
    }

    if (c == '\r' || c == '\n')
      col = 1;
    else
      ++col;

    lastchar = c;
    return c;
  }

  void throwParseException(const char *fmt, ...)
  {
    va_list ap;
    va_start(ap, fmt);
    char *message=NULL;
    int ret=vasprintf(&message, fmt, ap);
    va_end(ap);

    std::string _message;

    if (ret>=0) {
      _message=message;
      free(message);
    } else {
      _message="Format failure!";
    }

    throw AQLParserException(line, col, _message.c_str());
  }

  void expectNotEof()
  {
    if (!is) throw AQLParserException(line, col, "Unexpected end of file");
  }

  void expectEof()
  {
    int c = peek();
    if (c != std::istream::traits_type::eof())
    {
      std::string message = "Expected EOF but got '";
      message += possiblyEscape(c);
      message.push_back('\'');
      throw AQLParserException(line, col, message.c_str());
    }
  }


  void skipWhiteSpaces()
  {
    do {
      int c = is.peek();
      if (c == std::istream::traits_type::eof()) return;
      if (!isWhiteSpace(c)) return;
      get();
    } while (true);

  }

  std::string readString()
  {
    std::string ret;
    readExpectedCharacter('"');
    while (isStringCharacter(peek()))
    {
      char c=get();

      if (c=='\\')
      {
        c=get();
        switch (c)
        {
        case 'n':   c='\n'; break;
        case 'r':   c='\r'; break;
        case 'x':   throw AQLParserException(line, col, "\\x escape not implemented");
        case 'u':   throw AQLParserException(line, col, "\\u escape not implemented");
        case 'U':   throw AQLParserException(line, col, "\\X escape not implemented");
        case '\\':  break;
        case '"':   break;
        default:    throwParseException("Bad escape \\%c", c);
        }
      }
      ret.push_back(c);
    }
    readExpectedCharacter('"');
    return ret;
  }

  std::string readKeyword()
  {
    std::string ret;
    while (isKeywordCharacter(peek()))
    {
      char c=get();
      ret.push_back(c);
    }
    return ret;
  }

  int readInt()
  {
    char *end = 0;
    errno=0;
    std::string s=readKeyword();
    const char *start=s.c_str();
    long l=strtol(start, &end, 10);
    if (start+s.size()!=end) throwParseException("Bad integer value %s", start);

    if (errno!=0 || l>std::numeric_limits<int>::max() || l<std::numeric_limits<int>::min())
    {
      throwParseException("Number out of range: %s. Must be within [%d, %d]",
          start, std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
    }

    return static_cast<int>(l);
  }

  void readExpectedCharacter(char c)
  {
    int d=get();
    if (c!=d) {
      std::string message="Unexpected character '";
      message+=possiblyEscape(d);
      message+="'. Expected '";
      message+=possiblyEscape(c);
      message+="'";

      throw AQLParserException(line, col, message.c_str());
    }
  }

  void readExpectedKeyword(const char *exp)
  {
    std::string k=readKeyword();
    if (k!=exp)
    {
      std::string message="Expected keyword '";
      message+=+exp;
      message+=+'\'';
      throw AQLParserException(line, col, message);
    }
  }

  AQLExpr *parseExpr()
  {
    AQLExpr *expr=0;

    try {
      skipWhiteSpaces();
      readExpectedCharacter('(');
      skipWhiteSpaces();
      std::string keyword=readKeyword();
      if (keyword=="literal")
      {
        skipWhiteSpaces();
        std::string literal=readString();
        AQLLiteralExpr *literalExpr=new AQLLiteralExpr;
        expr=literalExpr;
        literalExpr->stringLiteral=literal;
      }
      else if (keyword=="property")
      {
        // property "joinname" "propertyname"
        skipWhiteSpaces();
        std::string joinName=readString();
        skipWhiteSpaces();
        std::string propertyKeyword=readKeyword();
        AQLPropertyExpr *propertyExpr=new AQLPropertyExpr();
        expr=propertyExpr;
        propertyExpr->joinName=joinName;
        if (propertyKeyword=="subject") {
          propertyExpr->property=AQLPropertyExpr::SUBJECT;
        } else if (propertyKeyword=="predicate") {
          propertyExpr->property=AQLPropertyExpr::PREDICATE;
        } else if (propertyKeyword=="object") {
          propertyExpr->property=AQLPropertyExpr::OBJECT;
        } else {
          throwParseException("Expected node part keyword (subject, predicate or object) but got \"%s\"", propertyKeyword.c_str());
        }
      }
      else if (keyword=="function")
      {
        // function "functionname" [argument]*
        skipWhiteSpaces();
        AQLFunctionExpr *functionExpr=new AQLFunctionExpr();
        expr=functionExpr;
        functionExpr->functionName=readString();
        while (true)
        {
          skipWhiteSpaces();
          if (peek()=='(')
          {
            // argument for function
            functionExpr->arguments.push_back(parseExpr());
          } else {
            break;
          }
        }
      }
      else if (keyword=="comp-eq")
      {
        AQLComparisonCriterion *comp=new AQLComparisonCriterion;
        expr=comp;
        comp->comparisonType=AQLComparisonCriterion::EQUAL;
        comp->left=parseExpr();
        comp->right=parseExpr();
      }
      else if (keyword=="comp-ne")
      {
        AQLComparisonCriterion *comp=new AQLComparisonCriterion;
        expr=comp;
        comp->comparisonType=AQLComparisonCriterion::NOT_EQUAL;
        comp->left=parseExpr();
        comp->right=parseExpr();
      }
      else if (keyword=="and" || keyword=="or")
      {
        AQLJunctionCriterion *junction=new AQLJunctionCriterion;
        expr=junction;

        if (keyword=="and")
          junction->junctionType=AQLJunctionCriterion::CONJUNCTION;
        else
          junction->junctionType=AQLJunctionCriterion::DISJUNCTION;

        skipWhiteSpaces();

        while (peek()=='(')
        {
          junction->terms.push_back(parseCriterion());
          skipWhiteSpaces();
        }
      }
      else if (keyword=="not")
      {
        AQLNotExpression *notExpression=new AQLNotExpression;
        expr=notExpression;
        notExpression->expr=parseExpr();
      }
      else {
        throwParseException("Expected: expression keyword but got \"%s\"", keyword.c_str());
      }
      skipWhiteSpaces();
      readExpectedCharacter(')');
      return expr;
    }
    catch (...) {
      delete expr;
      throw;
    }
  }

  AQLLogicalExpr *parseCriterion()
  {
    AQLExpr *expr=parseExpr();
    AQLLogicalExpr *booleanExpr=dynamic_cast<AQLLogicalExpr *>(expr);

    if (booleanExpr) return booleanExpr;

    std::string typeName=expr->getTypeName();

    delete expr;

    throwParseException("Expected logical expression but got %s expression", typeName.c_str());
    return 0;
  }

  AQLQuery *parseAQLQuery()
  {
    AQLQuery *q=new AQLQuery;

    try {
      skipWhiteSpaces();
      readExpectedCharacter('(');
      skipWhiteSpaces();
      readExpectedKeyword("aql-query");
      skipWhiteSpaces();

      while (peek()=='(')
      {
        get(); // reads peeked '('

        skipWhiteSpaces();
        std::string keyword=readKeyword();

        if (keyword=="select")
        {
          skipWhiteSpaces();
          std::string label=readString();
          AQLExpr *expr=parseExpr();
          AQLSelect *aqlSelect=new AQLSelect;

          aqlSelect->label=label;
          aqlSelect->expr=expr;

          q->selects.push_back(aqlSelect);
        }
        else if (keyword=="join")
        {
          skipWhiteSpaces();
          std::string joinTypeKeyword=readKeyword();

          AQLJoin::JoinType joinType=AQLJoin::JoinType();
          if (joinTypeKeyword=="left") {
            joinType=AQLJoin::LEFT_OUTER;
          } else if (joinTypeKeyword=="inner") {
            joinType=AQLJoin::INNER;
          } else {
            throwParseException("Bad join type '%s'. Expected 'left' or 'inner'", joinTypeKeyword.c_str());
          }

          skipWhiteSpaces();
          std::string joinName=readString();
          skipWhiteSpaces();
          AQLLogicalExpr *joinCriterion=parseCriterion();
          AQLJoin *join=new AQLJoin;
          join->criterion=joinCriterion;
          join->joinType=joinType;
          join->name=joinName;
          q->joins.push_back(join);
        }
        else if (keyword=="criterion")
        {
          AQLLogicalExpr *criterion=parseCriterion();
          if (!q->criterion)
          {
            q->criterion=criterion;
          }
          else if (dynamic_cast<AQLJunctionCriterion *>(q->criterion) &&
                   static_cast<AQLJunctionCriterion *>(q->criterion)->junctionType==AQLJunctionCriterion::CONJUNCTION) {
            // root is conjunction, so add this criterion as a new term
            static_cast<AQLJunctionCriterion *>(q->criterion)->terms.push_back(criterion);
          }
          else {
            // root is not a conjunction, create conjunction and add old root
            // and this criterion as terms
            AQLJunctionCriterion *newRoot=new AQLJunctionCriterion;
            newRoot->junctionType=AQLJunctionCriterion::CONJUNCTION;
            newRoot->terms.push_back(q->criterion);
            newRoot->terms.push_back(criterion);
            q->criterion=newRoot;
          }
        }
        else if (keyword=="sort")
        {
          AQLSort *sort=new AQLSort;
          q->sorts.push_back(sort);
          skipWhiteSpaces();
          std::string sortDirection=readKeyword();

          if (sortDirection=="ascending") {
            sort->ascending=true;
          } else if (sortDirection=="descending") {
            sort->ascending=false;
          } else {
            throwParseException("Bad sort direction '%s'. Expected 'ascending' or 'descending'", sortDirection.c_str());
          }

          skipWhiteSpaces();
          sort->expr=parseExpr();
        }
        else if (keyword=="result-max-rows") {
          skipWhiteSpaces();
          int maxRows=readInt();
          if (maxRows<0) throwParseException("Expected non-negative numeric value");
          q->maxRows=maxRows;
        }
        else if (keyword=="result-row-offset") {
          skipWhiteSpaces();
          int offset=readInt();
          if (offset<0) throwParseException("Expected non-negative numeric value");
          q->rowOffset=offset;
        }
        else {
          throw AQLParserException(line, col, "Expected: select, join, criterion, sort, result-max-rows, result-row-offset or ')'");
        }
        skipWhiteSpaces();
        readExpectedCharacter(')'); // matches keyword
        skipWhiteSpaces();
      }
      readExpectedCharacter(')'); // matches aql-query
      skipWhiteSpaces();
      return q;
    }
    catch (...) {
      delete q;
      throw;
    }
  }


public:
  AQLQuery *parse()
  {
    AQLQuery *q=0;
    try {
      q=parseAQLQuery();
      skipWhiteSpaces();
      expectEof();
    } catch (...) {
      delete q;
      throw;
    }
    return q;
  }

};

}


namespace Piglet
{


AQLLispParser::AQLLispParser()
{
}

AQLLispParser::~AQLLispParser()
{
}

AQLQuery *AQLLispParser::parseQuery(std::istream &is)
{
  AQLLispParserImpl impl(is);
  return impl.parse();
}

}
