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
 * AQLModel.h
 *
 *  Created on: Jun 4, 2009
 *      Author: skiminki
 */

#pragma once

#include <list>
#include <string>
#include <vector>

#include "AQLDebug.h"

namespace Piglet {

struct AQLVisitor;

struct AQLVisitable : protected AQLDebugBase
{
  virtual void accept(AQLVisitor &) = 0;
};

struct AQLExpr : public AQLVisitable
{
  virtual const char *getTypeName() = 0;
};

struct AQLLogicalExpr : public AQLExpr
{
};

struct AQLJunctionCriterion : public AQLLogicalExpr
{
  enum JunctionType
  {
    DISJUNCTION, // or
    CONJUNCTION, // and
  };

  JunctionType junctionType;

  std::list<AQLLogicalExpr *> terms;

  virtual const char *getTypeName();

  virtual ~AQLJunctionCriterion();
  virtual void accept(AQLVisitor &);
};

struct AQLPropertyExpr : public AQLExpr
{
  enum Property
  {
    SUBJECT,
    PREDICATE,
    OBJECT,
  };
  std::string joinName; // empty joinName = root join
  Property property;

  virtual const char *getTypeName();

  virtual void accept(AQLVisitor &);
};

struct AQLPropertyReferenceExpr : public AQLExpr
{
  std::string joinName; // empty joinName = root join
  AQLPropertyExpr::Property property;

  virtual const char *getTypeName();

  virtual void accept(AQLVisitor &);
};

struct AQLLiteralExpr : public AQLExpr
{
  std::string stringLiteral;

  virtual const char *getTypeName();

  virtual void accept(AQLVisitor &);
};

struct AQLFunctionExpr : public AQLExpr
{
  std::string functionName;
  std::list<AQLExpr *> arguments;

  virtual ~AQLFunctionExpr();
  virtual const char *getTypeName();
  virtual void accept(AQLVisitor &);
};

struct AQLNotExpression : public AQLLogicalExpr
{
  AQLExpr *expr;

  AQLNotExpression();
  virtual ~AQLNotExpression();
  virtual const char *getTypeName();
  virtual void accept(AQLVisitor &);
};

struct AQLComparisonCriterion : public AQLLogicalExpr
{
  enum ComparisonType
  {
    EQUAL, NOT_EQUAL,
  };

  AQLExpr *left;
  AQLExpr *right;

  ComparisonType comparisonType;

  virtual const char *getTypeName();
  virtual ~AQLComparisonCriterion();
  virtual void accept(AQLVisitor &);
};

struct AQLJoin : public AQLVisitable
{
  enum JoinType
  {
    INNER, // mandatory join
    LEFT_OUTER,
  // optional join
  };

  JoinType joinType;

  // join name, can be used in property expressions
  std::string name;

  // join criterion
  AQLLogicalExpr *criterion;

  virtual ~AQLJoin();
  virtual void accept(AQLVisitor &);
};

struct AQLSelect : public AQLVisitable
{
  // name of the select, used in result sets
  std::string label;

  // expression to be selected
  AQLExpr *expr;

  virtual ~AQLSelect();
  virtual void accept(AQLVisitor &);
};

struct AQLSort : public AQLVisitable
{
  bool ascending;

  // the expression used for sorting
  AQLExpr *expr;

  AQLSort();
  virtual ~AQLSort();
  virtual void accept(AQLVisitor &);
};

struct AQLQuery : public AQLVisitable
{
  // joins
  typedef std::list<AQLJoin *> join_list_type;
  std::list<AQLJoin *> joins;

  // selects
  typedef std::list<AQLSelect *> select_list_type;
  select_list_type selects;

  // criterion for search results, analogous to SQL WHERE
  AQLLogicalExpr *criterion;

  // the sort expressions
  std::list<AQLSort *> sorts;

  // result limiting
  int maxRows;
  int rowOffset;

  virtual void accept(AQLVisitor &);
  AQLQuery();
  virtual ~AQLQuery();
};

struct AQLResultItem : protected AQLDebugBase
{
  std::string value;
};

struct AQLResult : protected AQLDebugBase
{
  virtual ~AQLResult();

  virtual const std::vector<std::string> &getHeader() = 0;

  virtual bool hasNextRow() = 0;
  virtual const std::vector<AQLResultItem *> &nextRow() = 0;
};

class AQLVisitor : protected AQLDebugBase
{
public:

  // junction
  virtual void visitBeforeChildren(AQLJunctionCriterion &) = 0;
  virtual void visitBetweenChildren(AQLJunctionCriterion &, int pos) = 0;
  virtual void visitAfterChildren(AQLJunctionCriterion &) = 0;

  // expressions
  virtual void visit(AQLPropertyExpr &) = 0;
  virtual void visit(AQLPropertyReferenceExpr &) = 0;
  virtual void visit(AQLLiteralExpr &) = 0;

  virtual void visitBeforeChildren(AQLFunctionExpr &) = 0;
  virtual void visitBetweenChildren(AQLFunctionExpr &, int pos) = 0;
  virtual void visitAfterChildren(AQLFunctionExpr &) = 0;

  // boolean expressions
  virtual void visitBeforeChildren(AQLComparisonCriterion &) = 0;
  virtual void visitBetweenChildren(AQLComparisonCriterion &) = 0;
  virtual void visitAfterChildren(AQLComparisonCriterion &) = 0;

  virtual void visitBeforeChildren(AQLNotExpression &) = 0;
  virtual void visitAfterChildren(AQLNotExpression &) = 0;

  // joins
  virtual void visitBeforeChildren(AQLJoin &) = 0;
  virtual void visitAfterChildren(AQLJoin &) = 0;

  // selects
  virtual void visitBeforeChildren(AQLSelect &) = 0;
  virtual void visitBetweenChildren(AQLSelect &, int pos) = 0;
  virtual void visitAfterChildren(AQLSelect &) = 0;

  // sorts
  virtual void visitBeforeChildren(AQLSort &) = 0;
  virtual void visitAfterChildren(AQLSort &) = 0;

  // query
  virtual void visitBeforeChildren(AQLQuery &) = 0;
  virtual void visitAfterChildren(AQLQuery &) = 0;
  virtual void visitBeforeSelects(AQLQuery &) = 0;
  virtual void visitAfterSelects(AQLQuery &) = 0;
  virtual void visitBeforeJoins(AQLQuery &) = 0;
  virtual void visitAfterJoins(AQLQuery &) = 0;
  virtual void visitBeforeCriterion(AQLQuery &) = 0;
  virtual void visitAfterCriterion(AQLQuery &) = 0;
  virtual void visitBeforeSorts(AQLQuery &) = 0;
  virtual void visitBetweenSorts(AQLQuery &, int pos) = 0;
  virtual void visitAfterSorts(AQLQuery &) = 0;
};


}
