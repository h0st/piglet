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
 *  aqltester-main.cpp
 *
 *  Author: Sami Kiminki, skiminki@users.sourceforge.net
 */

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <fstream>
#include <typeinfo>

#include "AQL.h"
#include "AQLLispParser.h"
#include "AQLSupport.h"
#include "AQLToSQLTranslator.h"
#include "Condition.h"
#include "DB.h"

namespace {
using namespace Piglet;

struct ArgParseException : Piglet::Condition {
  ArgParseException(const char *format ...) {
     va_list ap;
     va_start(ap, format);
     setFormattedMessage(format, ap);
     va_end(ap);
  }
};

enum OutputLevel {
  OL_QUIET=0,
  OL_NORMAL,
  OL_VERBOSE,
  OL_DEBUG,
} outputLevel = OL_NORMAL;

enum OPERATING_MODE_ENUM {
   OM_HELP=0,
   OM_PARSE_QUERY,
   OM_OPTIMIZED_AQL,
   OM_SQL,
   OM_RAW_RESULT,
   OM_RESULT,
};

enum AQL_PARSER_ENUM {
   AP_LIST,
};


void print(OutputLevel ol, const char *format ...)
{
  if (ol<=outputLevel)
  {
    va_list ap;
    va_start(ap, format);
    vprintf(format, ap);
    va_end(ap);
  }
}



void printHelp()
{
  std::cout <<
  "Usage: aqltester-main [options] <db_file> <input_file>\n"
  "  <db_file>      File containing SQLite3 database\n"
  "  <input_file>   File containing AQL queries or - for stdin\n"
  "\n"
  "Options:\n"
  "  --help         Display this help\n"
  "  --version      Display version information\n"
  "  --quiet        Output only the result (for scripts)\n"
  "  --verbose      Verbose output\n"
  "  --debug        Lots of debug stuff\n"
  "  --parser=...   Select AQL parser front-end. One of: list. Default: list\n"
  "  --stop-at=...  Stops query processing after specific stage and display\n"
  "                 working data. Stages: parse_query, optimized_aql, sql,\n"
  "                 raw_result, result. Default: result.\n"
  "";
}

AQLParser *createAqlParser(AQL_PARSER_ENUM a)
{
  switch (a)
  {
  case AP_LIST:
    return new AQLLispParser();

  default:
    throw Condition("Internal error: Could not instantiate parser %d", int(a));
  }
}

int processMain(int argc, char **argv)
{
   using namespace Piglet;
   std::istream *is = 0;
   bool closeIsAfterUse=false;

   AQL_PARSER_ENUM aqlParser=AP_LIST;
   OPERATING_MODE_ENUM operatingMode = OM_RESULT;

   // parse switches
   char **argp=argv+1;
   for (; argp!=argv+argc; ++argp) {
      char *arg=*argp;
      if (strncmp(arg, "--", 2)==0) {
         // it's a switch

         if (strlen(arg)==2) {
            // end of switches
            ++argp;
            break;
         }

         if (strcmp(arg+2, "help")==0) {
           // help
           operatingMode=OM_HELP;
         }
         else if (strcmp(arg+2, "quiet")==0) {
           outputLevel=OL_QUIET;
         }
         else if (strcmp(arg+2, "verbose")==0) {
           outputLevel=OL_VERBOSE;
         }
         else if (strcmp(arg+2, "debug")==0) {
           outputLevel=OL_DEBUG;
         }
         else if (strcmp(arg+2, "stop-at=parse_query")==0) {
           operatingMode=OM_PARSE_QUERY;
         }
         else if (strcmp(arg+2, "stop-at=optimized_aql")==0) {
           operatingMode=OM_OPTIMIZED_AQL;
         }
         else if (strcmp(arg+2, "stop-at=sql")==0) {
           operatingMode=OM_SQL;
         }
         else if (strcmp(arg+2, "stop-at=raw_result")==0) {
           operatingMode=OM_RAW_RESULT;
         }
         else if (strcmp(arg+2, "stop-at=result")==0) {
           operatingMode=OM_RESULT;
         }
         else {
           // no match found, this is error
           throw ArgParseException("Unknown switch %s", arg);
         }
      } else {
         // not a switch
         break;
      }
   }

   // There should be 2 arguments left, namely the database and input files
   if (argc - (argp-argv) < 2) {
      operatingMode=OM_HELP;
   }

   if (argc - (argp-argv) > 2) {
      throw ArgParseException("Too many program arguments");
   }

   if (operatingMode==OM_HELP)
   {
     printHelp();
     return 1;
   }


   // exactly 2 arguments left
   char *db_file=*argp++;
   const char *input=*argp++;



   if (strcmp(input, "-")==0) {
      is=&std::cin;
   } else {
      is=new std::ifstream(input);
      if (is->fail()) {
         throw Condition("Could not open input file '%s'", input);
      }
      closeIsAfterUse=true;
   }



   AQLQuery *aqlQuery=0;
   SQLQuery *sqlQuery=0;
   SQLResult *sqlResult=0;
   AQLResult *aqlResult=0;
   Piglet::DB pigletDb(db_file, true);

   try {
     // ok, and go

     for (int om=OM_PARSE_QUERY; om<=operatingMode; ++om)
     {
       switch (om)
       {
       case OM_PARSE_QUERY: {
         print(OL_VERBOSE, "Parsing query...\n");
         AQLParser *parser=createAqlParser(aqlParser);
         aqlQuery=parser->parseQuery(*is);
         delete parser;
         break;
       }
       case OM_OPTIMIZED_AQL: {
         print(OL_VERBOSE, "Optimizing AQL...\n");
         AQLToSQLTranslator translator;
         translator.optimize(*aqlQuery);
         break;
       }

       case OM_SQL: {
         print(OL_VERBOSE, "Generating SQL...\n");
         AQLToSQLTranslator translator;
         sqlQuery=translator.translateToSql(*aqlQuery, false);
         break;
       }
       case OM_RAW_RESULT: {
         print(OL_VERBOSE, "Executing SQL...\n");
         SQLQueryExecutor queryExecutor;
         sqlResult=queryExecutor.execute(*sqlQuery);
         break;
       }
       case OM_RESULT: {
         print(OL_VERBOSE, "Formatting result...\n");
         AQLQueryExecutor queryExecutor;
         aqlResult=queryExecutor.executeQueryWithPreformedResult(*aqlQuery, sqlResult);
         sqlResult=0; // sqlResult is now owned by aqlResult

         int rowNum=0;

         while (aqlResult->hasNextRow())
         {
           const std::vector<AQLResultItem *> row=
             aqlResult->nextRow();

           ++rowNum;
           print(OL_QUIET, "Row %d\n", rowNum);
           print(OL_QUIET, "%s\n", "******************");

           for (size_t i=0; i<row.size(); ++i)
           {
             AQLResultItem *value=row[i];
             const char *textValue;
             if (value) {
               textValue=value->value.c_str();
             } else {
               textValue="(null)";
             }
             print(OL_QUIET, "  %s: %s\n", aqlResult->getHeader().at(i).c_str(), textValue);
           }

           print(OL_QUIET, "\n");

         }

         break;
       }

       default:
         throw Condition("Unknown operating mode %d", static_cast<int>(om));
       }

       if (om==operatingMode || outputLevel>=OL_DEBUG)
       {
         switch (om)
         {
         case OM_PARSE_QUERY: {
           print(OL_NORMAL, "AQL Query:\n");
           AQLPrinterVisitor aqlPrinterVisitor(std::cout);
           aqlQuery->accept(aqlPrinterVisitor);
           break;
         }
         case OM_OPTIMIZED_AQL: {
           print(OL_NORMAL, "AQL Query after optimization:\n");
           AQLPrinterVisitor aqlPrinterVisitor(std::cout);
           aqlQuery->accept(aqlPrinterVisitor);
           break;
         }

         case OM_SQL:
           print(OL_NORMAL, "SQL query:\n%s\n", sqlQuery->sqlQuery.c_str());
           // TODO: print parameters
           break;

         case OM_RAW_RESULT: {
           int row=0;
           while (sqlResult->nextRow()) {
             int col=0;
             ++row;
             print(OL_NORMAL, "Row %d\n", row);
             print(OL_NORMAL, "%s\n", "******************");
             for (std::vector<std::string>::const_iterator i=sqlResult->getColumnNames().begin();
                  i!=sqlResult->getColumnNames().end(); ++i, ++col)
             {
               const char *cellValue;
               if (!sqlResult->isNull(col)) {
                 cellValue=sqlResult->getString(col);
               } else {
                 cellValue="(null)";
               }

               print(OL_NORMAL, "  %s: %s\n", (*i).c_str(), cellValue);
             }
             print(OL_NORMAL, "\n");
           }
           sqlResult->reexecute();
           break;
         }

         case OM_RESULT:
           // no debug here
           break;

         default:
           throw Condition("Unimplemented debug for operating mode %d", static_cast<int>(om));
         }
       }
     }
   } catch (...) {
      if (closeIsAfterUse) delete is;

      delete aqlQuery;
      delete sqlQuery;
      delete sqlResult;
      delete aqlResult;

      throw;
   }

   delete aqlQuery;
   delete sqlQuery;
   delete sqlResult;
   delete aqlResult;

   if (closeIsAfterUse) delete is;

   return 0;
}

} // end of anonymous namespace

int main(int argc, char **argv) {
   using namespace Piglet;

   try {
      return processMain(argc, argv);
   }
   catch (ArgParseException &ex) {
     std::cerr << "Argument exception: " << ex.message() << std::endl;
     return 1;
   }
   catch (Condition &exception) {
      const std::type_info &info = typeid(exception);
      std::cerr << "General exception " << info.name() << ": " << exception.message() << std::endl;
      return 1;
   }

}
