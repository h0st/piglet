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
 *  AQL.cpp
 *
 *  Author: Sami Kiminki, skiminki@users.sourceforge.net
 */

#include <ostream>

#include "AQLSupport.h"
#include "Condition.h"
#include "Useful.h"
#include "AQLModel.h"
using namespace Piglet;

namespace Piglet {


AQLPrinterVisitor::AQLPrinterVisitor(std::ostream &_os) : os(_os)
{
  // nothing here
}

void AQLPrinterVisitor::printString(const std::string &s) const
{
  os << '"';
  for (std::string::const_iterator i=s.begin(); i!=s.end(); ++i)
  {
    unsigned int c=unsigned(*i);

    if (c>=0x20 && c!='"')
    {
      os << char(c);
      continue;
    }

    switch (c)
    {
    case '\n':
      os << "\\n";
      break;
    case '\r':
      os << "\\r";
      break;
    case '"':
      os << "\\\"";
      break;
    default:
      os << "\\x";
      os << intToHexString(c, 2);
      break;
    }
  }
  os << '"';
}

void AQLPrinterVisitor::visitBeforeChildren(AQLJunctionCriterion &junction)
{
  os << " (";
  switch (junction.junctionType)
  {
  case AQLJunctionCriterion::CONJUNCTION:
    os << "and";
    break;
  case AQLJunctionCriterion::DISJUNCTION:
    os << "or";
    break;
  }
}
void AQLPrinterVisitor::visitBetweenChildren(AQLJunctionCriterion &, int)
{
}
void AQLPrinterVisitor::visitAfterChildren(AQLJunctionCriterion &)
{
  os << ')';
}
void AQLPrinterVisitor::visit(AQLPropertyExpr &expr)
{
  os << " (property ";
  printString(expr.joinName);
  os << ' ';
  const char *property;
  switch (expr.property)
  {
  case AQLPropertyExpr::SUBJECT:    property="subject"; break;
  case AQLPropertyExpr::PREDICATE:  property="predicate"; break;
  case AQLPropertyExpr::OBJECT:     property="object"; break;
  default:
    throw Condition("Unknown property part %d", int(expr.property));
  }
  os << property;
  os << ')';
}
void AQLPrinterVisitor::visit(AQLPropertyReferenceExpr &expr)
{
  os << " (property-ref ";
  printString(expr.joinName);
  os << ' ';
  const char *property;
  switch (expr.property)
  {
  case AQLPropertyExpr::SUBJECT:    property="subject"; break;
  case AQLPropertyExpr::PREDICATE:  property="predicate"; break;
  case AQLPropertyExpr::OBJECT:     property="object"; break;
  default:
    throw Condition("Unknown property part %d", int(expr.property));
  }
  os << property;
  os << ')';
}
void AQLPrinterVisitor::visit(AQLLiteralExpr &expr)
{
  os << " (literal ";
  printString(expr.stringLiteral);
  os << ')';
}
void AQLPrinterVisitor::visitBeforeChildren(AQLComparisonCriterion &c)
{
  os << " (cmp";
  switch (c.comparisonType)
  {
  case AQLComparisonCriterion::EQUAL:
    os << "-eq";
    break;
  case AQLComparisonCriterion::NOT_EQUAL:
    os << "-ne";
    break;
  }
}
void AQLPrinterVisitor::visitBetweenChildren(AQLComparisonCriterion &)
{
}
void AQLPrinterVisitor::visitAfterChildren(AQLComparisonCriterion &)
{
  os << ')';
}
void AQLPrinterVisitor::visitBeforeChildren(AQLNotExpression &)
{
  os << " (not";
}
void AQLPrinterVisitor::visitAfterChildren(AQLNotExpression &)
{
  os << ')';
}
void AQLPrinterVisitor::visitBeforeChildren(AQLFunctionExpr &fexpr)
{
  os << " (function ";
  printString(fexpr.functionName);
}
void AQLPrinterVisitor::visitBetweenChildren(AQLFunctionExpr &, int)
{
}
void AQLPrinterVisitor::visitAfterChildren(AQLFunctionExpr &)
{
  os << ')';
}
void AQLPrinterVisitor::visitBeforeChildren(AQLJoin &j)
{
  os << "  (join ";
  switch (j.joinType)
  {
  case AQLJoin::INNER:
    os << "inner";
    break;
  case AQLJoin::LEFT_OUTER:
    os << "left";
    break;
  }
}
void AQLPrinterVisitor::visitAfterChildren(AQLJoin &)
{
  os << ')' << std::endl;
}
void AQLPrinterVisitor::visitBeforeChildren(AQLSelect &s)
{
  os << "  (select ";
  printString(s.label);
}
void AQLPrinterVisitor::visitBetweenChildren(AQLSelect &, int)
{
}
void AQLPrinterVisitor::visitAfterChildren(AQLSelect &s)
{
  os << ')' << std::endl;
}
void AQLPrinterVisitor::visitBeforeChildren(AQLSort &s)
{
  os << "  (sort ";
  if (s.ascending)
    os << "ascending";
  else
    os << "descending";
}
void AQLPrinterVisitor::visitAfterChildren(AQLSort &s)
{
  os << ')' << std::endl;
}
void AQLPrinterVisitor::visitBeforeChildren(AQLQuery &)
{
  os << "(aql-query" << std::endl;
}
void AQLPrinterVisitor::visitAfterChildren(AQLQuery &query)
{
  if (query.maxRows>=0)
    os << "  (result-max-rows " << query.maxRows << ')' << std::endl;
  if (query.rowOffset>=0)
    os << "  (result-row-offset " << query.rowOffset << ')' << std::endl;
  os << ')' << std::endl;
}
void AQLPrinterVisitor::visitBeforeSelects(AQLQuery &)
{
}
void AQLPrinterVisitor::visitAfterSelects(AQLQuery &)
{

}
void AQLPrinterVisitor::visitBeforeJoins(AQLQuery &)
{
}
void AQLPrinterVisitor::visitAfterJoins(AQLQuery &)
{
}
void AQLPrinterVisitor::visitBeforeCriterion(AQLQuery &)
{
  os << "  (criterion";
}
void AQLPrinterVisitor::visitAfterCriterion(AQLQuery &)
{
  os << ")" << std::endl;
}
void AQLPrinterVisitor::visitBeforeSorts(AQLQuery &)
{
}
void AQLPrinterVisitor::visitBetweenSorts(AQLQuery &, int)
{
}
void AQLPrinterVisitor::visitAfterSorts(AQLQuery &)
{
}
void AQLOptionalVisitor::visitBeforeChildren(AQLJunctionCriterion &) {}
void AQLOptionalVisitor::visitBetweenChildren(AQLJunctionCriterion &, int) {}
void AQLOptionalVisitor::visitAfterChildren(AQLJunctionCriterion &) {}
void AQLOptionalVisitor::visit(AQLPropertyExpr &) {}
void AQLOptionalVisitor::visit(AQLPropertyReferenceExpr &) {}
void AQLOptionalVisitor::visit(AQLLiteralExpr &) {}
void AQLOptionalVisitor::visitBeforeChildren(AQLFunctionExpr &) {}
void AQLOptionalVisitor::visitBetweenChildren(AQLFunctionExpr &, int) {}
void AQLOptionalVisitor::visitAfterChildren(AQLFunctionExpr &) {}
void AQLOptionalVisitor::visitBeforeChildren(AQLComparisonCriterion &) {}
void AQLOptionalVisitor::visitBetweenChildren(AQLComparisonCriterion &) {}
void AQLOptionalVisitor::visitAfterChildren(AQLComparisonCriterion &) {}
void AQLOptionalVisitor::visitBeforeChildren(AQLNotExpression &) {}
void AQLOptionalVisitor::visitAfterChildren(AQLNotExpression &) {}
void AQLOptionalVisitor::visitBeforeChildren(AQLJoin &) {}
void AQLOptionalVisitor::visitAfterChildren(AQLJoin &) {}
void AQLOptionalVisitor::visitBeforeChildren(AQLSelect &) {}
void AQLOptionalVisitor::visitBetweenChildren(AQLSelect &, int) {}
void AQLOptionalVisitor::visitAfterChildren(AQLSort &) {}
void AQLOptionalVisitor::visitBeforeChildren(AQLSort &) {}
void AQLOptionalVisitor::visitAfterChildren(AQLQuery &) {}
void AQLOptionalVisitor::visitBeforeChildren(AQLQuery &) {}
void AQLOptionalVisitor::visitAfterChildren(AQLSelect &) {}
void AQLOptionalVisitor::visitBeforeSelects(AQLQuery &) {}
void AQLOptionalVisitor::visitAfterSelects(AQLQuery &) {}
void AQLOptionalVisitor::visitBeforeJoins(AQLQuery &) {}
void AQLOptionalVisitor::visitAfterJoins(AQLQuery &) {}
void AQLOptionalVisitor::visitBeforeCriterion(AQLQuery &) {}
void AQLOptionalVisitor::visitAfterCriterion(AQLQuery &) {}
void AQLOptionalVisitor::visitBeforeSorts(AQLQuery &) {}
void AQLOptionalVisitor::visitBetweenSorts(AQLQuery &, int) {}
void AQLOptionalVisitor::visitAfterSorts(AQLQuery &) {}


}
