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
 *  AQLSupport.h
 *
 *  Author: Sami Kiminki, skiminki@users.sourceforge.net
 */

#pragma once

#include <ostream>
#include <string>

#include "AQLModel.h"

namespace Piglet {


   /**
    * This is a base for visitors which only want to implement some of the
    * visit methods, but not all
    */
   class AQLOptionalVisitor : public AQLVisitor {

      public:
         virtual void visitBeforeChildren(AQLJunctionCriterion &);
         virtual void visitBetweenChildren(AQLJunctionCriterion &, int);
         virtual void visitAfterChildren(AQLJunctionCriterion &);

         virtual void visit(AQLPropertyExpr &);
         virtual void visit(AQLPropertyReferenceExpr &);
         virtual void visit(AQLLiteralExpr &);

         virtual void visitBeforeChildren(AQLComparisonCriterion &);
         virtual void visitBetweenChildren(AQLComparisonCriterion &);
         virtual void visitAfterChildren(AQLComparisonCriterion &);

         virtual void visitBeforeChildren(AQLNotExpression &);
         virtual void visitAfterChildren(AQLNotExpression &);

         virtual void visitBeforeChildren(AQLFunctionExpr &);
         virtual void visitBetweenChildren(AQLFunctionExpr &, int pos);
         virtual void visitAfterChildren(AQLFunctionExpr &);

         virtual void visitBeforeChildren(AQLJoin &);
         virtual void visitAfterChildren(AQLJoin &);

         virtual void visitBeforeChildren(AQLSelect &);
         virtual void visitBetweenChildren(AQLSelect &, int);
         virtual void visitAfterChildren(AQLSelect &);

         virtual void visitBeforeChildren(AQLSort &);
         virtual void visitAfterChildren(AQLSort &);

         virtual void visitBeforeChildren(AQLQuery &);
         virtual void visitAfterChildren(AQLQuery &);
         virtual void visitBeforeSelects(AQLQuery &);
         virtual void visitAfterSelects(AQLQuery &);
         virtual void visitBeforeJoins(AQLQuery &);
         virtual void visitAfterJoins(AQLQuery &);
         virtual void visitBeforeCriterion(AQLQuery &);
         virtual void visitAfterCriterion(AQLQuery &);
         virtual void visitBeforeSorts(AQLQuery &);
         virtual void visitBetweenSorts(AQLQuery &, int pos);
         virtual void visitAfterSorts(AQLQuery &);

   };

   /**
    * Visitor that prints AQL into stream, mostly for debugging purposes
    */
   class AQLPrinterVisitor : public AQLVisitor {

      private:
         std::ostream &os;

      protected:
        void printString(const std::string &) const;

      public:
         AQLPrinterVisitor(std::ostream &);

         virtual void visitBeforeChildren(AQLJunctionCriterion &);
         virtual void visitBetweenChildren(AQLJunctionCriterion &, int);
         virtual void visitAfterChildren(AQLJunctionCriterion &);

         virtual void visit(AQLPropertyExpr &);
         virtual void visit(AQLPropertyReferenceExpr &);
         virtual void visit(AQLLiteralExpr &);
         virtual void visitBeforeChildren(AQLFunctionExpr &);
         virtual void visitBetweenChildren(AQLFunctionExpr &, int pos);
         virtual void visitAfterChildren(AQLFunctionExpr &);

         virtual void visitBeforeChildren(AQLComparisonCriterion &);
         virtual void visitBetweenChildren(AQLComparisonCriterion &);
         virtual void visitAfterChildren(AQLComparisonCriterion &);

         virtual void visitBeforeChildren(AQLNotExpression &);
         virtual void visitAfterChildren(AQLNotExpression &);

         virtual void visitBeforeChildren(AQLJoin &);
         virtual void visitAfterChildren(AQLJoin &);

         virtual void visitBeforeChildren(AQLSelect &);
         virtual void visitBetweenChildren(AQLSelect &, int);
         virtual void visitAfterChildren(AQLSelect &);

         virtual void visitBeforeChildren(AQLSort &);
         virtual void visitAfterChildren(AQLSort &);

         virtual void visitBeforeChildren(AQLQuery &);
         virtual void visitAfterChildren(AQLQuery &);
         virtual void visitBeforeSelects(AQLQuery &);
         virtual void visitAfterSelects(AQLQuery &);
         virtual void visitBeforeJoins(AQLQuery &);
         virtual void visitAfterJoins(AQLQuery &);
         virtual void visitBeforeCriterion(AQLQuery &);
         virtual void visitAfterCriterion(AQLQuery &);
         virtual void visitBeforeSorts(AQLQuery &);
         virtual void visitBetweenSorts(AQLQuery &, int pos);
         virtual void visitAfterSorts(AQLQuery &);

   };

}
