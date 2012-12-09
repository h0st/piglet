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
 *  Curl.cpp
 *
 *  Author: Ora Lassila mailto:ora.lassila@nokia.com
 *  Copyright (c) 2001-2008 Nokia. All Rights Reserved.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include "Curl.h"
#include <string.h>
namespace libcurl {

Curl::Curl(void)
{
  _curl = curl_easy_init();
}

Curl::~Curl(void)
{
  if (_curl)
    curl_easy_cleanup(_curl);
}

void Curl::setURL(const char *url)
{
  curl_easy_setopt(_curl, CURLOPT_URL, url);
}

void Curl::setOption(CURLoption option, void *data)
{
  curl_easy_setopt(_curl, option, data);
}

bool Curl::perform(void)
{
  return (curl_easy_perform(_curl) == CURLE_OK);
}

bool Curl::getInfo(CURLINFO option, void *data) MAYFAIL
{
  CURLcode result = curl_easy_getinfo(_curl, option, data);
  if (result == CURLE_OK)
    return true;
  else
    FAIL("cURL error");
}

bool Curl::findFileTime(const char *url, time_t *time) MAYFAIL
{
  if (strncmp(url, "http", 4) == 0) {
    setURL(url);
    setOption(CURLOPT_NOBODY, (void *)1);
    setOption(CURLOPT_FILETIME, (void *)1);
    setOption(CURLOPT_FOLLOWLOCATION, (void *)1);
    setOption(CURLOPT_CONNECTTIMEOUT, (void *)3); // 3 seconds
    setOption(CURLOPT_NOSIGNAL, (void *)1);
    if (perform()) {
      time_t t;
      if (getInfo(CURLINFO_FILETIME, &t) && (t == -1))
        t = 0;
      *time = t;
      return true;
    }
  }
  else if (strncmp(url, "file", 4) == 0) {
    struct stat s;
    if (stat(url+7, &s) == 0) {
      *time = s.st_mtime; // if needed, expands into s.st_mtimespec.tv_sec
      return true;
    }
  }
  return false;
}

bool Curl::getFileTime(const char *url, time_t *time) MAYFAIL
{
  Curl curl;
  return curl.findFileTime(url, time);
}

}
