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
 *  Condition.cpp
 *
 *  Author: Ora Lassila mailto:ora.lassila@nokia.com
 *  Copyright (c) 2001-2008 Nokia. All Rights Reserved.
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Condition.h"
#include <string.h>

namespace Piglet {

Condition::Condition()
{
}
Condition::Condition(const std::string &message) : _message(message)
{
}
Condition::Condition(const char *format, ...)
{
  va_list ap;
  va_start(ap, format);
  setFormattedMessage(format, ap);
  va_end(ap);
}
Condition::~Condition()
{
  // NO-OP
}

void Condition::setFormattedMessage(const char *format, va_list ap)
{
  char *message=0;
  int ret=vasprintf(&message, format, ap);
  if (ret>=0) {
    _message=message;
    free(message);
  } else {
    _message="Format failure!";
  }
}


std::ostream& operator<<(std::ostream& os, const Condition &c)
{
  os << c.message() << "\n";
  return os;
}

bool check(bool success, const char *message) MAYFAIL
{
  if (success)
    return success;
  else
    FAIL(message);
}

}
