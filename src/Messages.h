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
 *  Messages.h
 *
 *  Author: Ora Lassila mailto:ora.lassila@nokia.com
 *  Copyright (c) 2001-2008 Nokia. All Rights Reserved.
 */

#pragma once

namespace Piglet {

#define Message(v, s) static const char *v = s

Message(ERR_DB_OPEN,      "Unable to open database");
Message(ERR_NODE_ID,      "Unable to create a new node ID");
Message(ERR_NODE_DETAILS, "Unable to query for node details");
Message(ERR_NODE_NEW,     "Unable to insert a new node");
Message(ERR_NODE_FIND,    "Unable to find node");
Message(ERR_TRIPLE_ADD,   "Unable to insert new triple");
Message(ERR_TRIPLE_DEL,   "Unable to delete triple");
Message(ERR_TRIPLE_FIND,  "Unable to query for triples");
Message(ERR_NS_ADD,       "Unable to insert new namespace");
Message(ERR_NS_DEL,       "Unable to delete namespace");
Message(ERR_NS_FIND,      "Unable to find namespace");
Message(ERR_SRC_FIND,     "Unable to determine if source has been loaded before");
Message(ERR_SRC_TIME,     "Unable to update load time");
Message(ERR_SRC_DEL,      "Unable to update load time");
Message(ERR_SRC_QUERY,    "Unable to find sources");
Message(ERR_TRANSACTION,  "Transaction-related error");

#define PIGLET_DEBUG 0

}
