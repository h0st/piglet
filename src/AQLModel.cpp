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
 * AQLModel.cpp
 *
 *  Created on: Jun 4, 2009
 *      Author: skiminki
 */

#include <algorithm>

#include "AQLModel.h"

namespace {

using namespace Piglet;

class AcceptVisitor
{
protected:
  AQLVisitor &visitor;
public:
  AcceptVisitor(AQLVisitor &_visitor) : visitor(_visitor) {}
  void operator() (AQLVisitable *pvisitable)
  {
    pvisitable->accept(visitor);
  }
};

template <class TVisitable> class AcceptBetweenVisitor
{
protected:
  AQLVisitor &visitor;
  TVisitable &visitable;
  int pos;
public:
  AcceptBetweenVisitor(AQLVisitor &_visitor, TVisitable &_visitable) : visitor(_visitor), visitable(_visitable), pos(0) {}
  void operator() (AQLVisitable *pvisitable)
  {
    if (pos>=1) visitor.visitBetweenChildren(visitable, pos);
    ++pos;
    pvisitable->accept(visitor);
  }
};

template<class T> void deleteObject(T *o)
{
  delete o;
}

}

namespace Piglet
{

AQLJunctionCriterion::~AQLJunctionCriterion()
{
  std::for_each(terms.begin(), terms.end(), deleteObject<AQLLogicalExpr>);
}
const char *AQLJunctionCriterion::getTypeName()
{
  return "junction";
}
void AQLJunctionCriterion::accept(AQLVisitor &v)
{
  v.visitBeforeChildren(*this);
  std::for_each(terms.begin(), terms.end(), AcceptBetweenVisitor<AQLJunctionCriterion>(v, *this));
  v.visitAfterChildren(*this);
}

void AQLPropertyExpr::accept(AQLVisitor &v)
{
  v.visit(*this);
}

const char *AQLPropertyExpr::getTypeName()
{
  return "property value";
}

void AQLPropertyReferenceExpr::accept(AQLVisitor &v)
{
  v.visit(*this);
}

const char *AQLPropertyReferenceExpr::getTypeName()
{
  return "property reference";
}

void AQLLiteralExpr::accept(AQLVisitor &v)
{
  v.visit(*this);
}

const char *AQLLiteralExpr::getTypeName()
{
  return "literal";
}

AQLFunctionExpr::~AQLFunctionExpr()
{
  std::for_each(arguments.begin(), arguments.end(), deleteObject<AQLExpr>);
}
const char *AQLFunctionExpr::getTypeName()
{
  return "function";
}
void AQLFunctionExpr::accept(AQLVisitor &v)
{
  v.visitBeforeChildren(*this);
  std::for_each(arguments.begin(), arguments.end(), AcceptBetweenVisitor<AQLFunctionExpr>(v, *this));
  v.visitAfterChildren(*this);
}

AQLComparisonCriterion::~AQLComparisonCriterion()
{
  delete left;
  delete right;
}
const char *AQLComparisonCriterion::getTypeName()
{
  return "comparison";
}
void AQLComparisonCriterion::accept(AQLVisitor &v)
{
  v.visitBeforeChildren(*this);
  if (left) left->accept(v);
  v.visitBetweenChildren(*this);
  if (right) right->accept(v);
  v.visitAfterChildren(*this);
}

AQLNotExpression::AQLNotExpression() : expr(0)
{
}
AQLNotExpression::~AQLNotExpression()
{
  delete expr;
}
const char *AQLNotExpression::getTypeName()
{
  return "not";
}
void AQLNotExpression::accept(AQLVisitor &v)
{
  v.visitBeforeChildren(*this);
  if (expr) expr->accept(v);
  v.visitAfterChildren(*this);
}


AQLJoin::~AQLJoin()
{
  delete criterion;
}

void AQLJoin::accept(AQLVisitor &v)
{
  v.visitBeforeChildren(*this);
  if (criterion) criterion->accept(v);
  v.visitAfterChildren(*this);
}

AQLSelect::~AQLSelect()
{
  delete expr;
}

void AQLSelect::accept(AQLVisitor &v)
{
  v.visitBeforeChildren(*this);
  if (expr) expr->accept(v);
  v.visitAfterChildren(*this);
}

AQLSort::AQLSort() : ascending(true), expr(0)
{
}

AQLSort::~AQLSort()
{
  delete expr;
}

void AQLSort::accept(AQLVisitor &v)
{
  v.visitBeforeChildren(*this);
  if (expr) expr->accept(v);
  v.visitAfterChildren(*this);
}

AQLQuery::AQLQuery() : criterion(0), maxRows(-1), rowOffset(-1)
{
}

AQLQuery::~AQLQuery()
{
  std::for_each(selects.begin(), selects.end(), deleteObject<AQLSelect>);
  std::for_each(joins.begin(), joins.end(), deleteObject<AQLJoin>);
  delete criterion;
  std::for_each(sorts.begin(), sorts.end(), deleteObject<AQLSort>);
}

void AQLQuery::accept(AQLVisitor &v)
{
  v.visitBeforeChildren(*this);
  v.visitBeforeSelects(*this);
  std::for_each(selects.begin(), selects.end(), AcceptVisitor(v));
  v.visitAfterSelects(*this);
  v.visitBeforeJoins(*this);
  std::for_each(joins.begin(), joins.end(), AcceptVisitor(v));
  v.visitAfterJoins(*this);
  v.visitBeforeCriterion(*this);
  if (criterion)
    criterion->accept(v);
  v.visitAfterCriterion(*this);
  v.visitBeforeSorts(*this);
  int pos=0;
  for (std::list<AQLSort *>::iterator i=sorts.begin();
       i!=sorts.end(); ++i)
  {
    AQLSort *sort=*i;
    if (pos>=1) v.visitBetweenSorts(*this, pos);
    sort->accept(v);
    ++pos;
  }
  v.visitAfterSorts(*this);
  v.visitAfterChildren(*this);
}

AQLResult::~AQLResult() {}

}
