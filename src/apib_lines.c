/*
Copyright 2019 Google LLC

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    https://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "apib_lines.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>

static int isChar(const char c, const char* match)
{
  unsigned int m = 0;

  while (match[m] != 0) {
    if (c == match[m]) {
      return 1;
    }
    m++;
  }
  return 0;
}

void linep_Start(LineState* l, char* line, int size,
		 int len)
{
  l->httpMode = 0;
  l->buf = line;
  l->bufSize = size;
  l->bufLen = len;
  l->lineStart = l->lineEnd = 0;
  l->tokStart = l->tokEnd = 0;
  l->lineComplete = 0;
}

void linep_SetHttpMode(LineState* l, int on)
{
  l->httpMode = on;
}

static void nullLast(LineState* l) 
{
  l->buf[l->lineEnd] = 0;
  l->lineEnd++;
}

int linep_NextLine(LineState* l)
{
  if (l->lineEnd > 0) {
    l->lineStart = l->lineEnd;
  }
  if (l->lineEnd >= l->bufLen) {
    l->lineComplete = 0;
    return 0;
  }
  
  /* Move to the first newline character */
  while ((l->lineEnd < l->bufLen) &&
	 !isChar(l->buf[l->lineEnd], "\r\n")) {
    l->lineEnd++;
  }
  if (l->lineEnd >= l->bufLen) {
    /* Incomplete line in the buffer */
    l->lineComplete = 0;
    return 0;
  }

  if (l->httpMode) {
    if (l->buf[l->lineEnd] == '\r') {
      nullLast(l);
      if (l->buf[l->lineEnd] == '\n') {
	nullLast(l);
      }
    } else {
      nullLast(l);
    }      
  } else {
    /* Overwrite all newlines with nulls */
    while ((l->lineEnd < l->bufLen) &&
	   isChar(l->buf[l->lineEnd], "\r\n")) {
      nullLast(l);
    }
  }

  l->tokStart = l->tokEnd = l->lineStart;

  l->lineComplete = 1;
  return 1;
}

char* linep_GetLine(LineState* l)
{
  if (!l->lineComplete) {
    return NULL;
  }
  return l->buf + l->lineStart;
}

char* linep_NextToken(LineState* l, const char* toks)
{
  if (!l->lineComplete) {
    return NULL;
  }
  if (l->tokEnd >= l->lineEnd) {
    return NULL;
  }

  l->tokStart = l->tokEnd;

  while ((l->tokEnd < l->lineEnd) &&
	 !isChar(l->buf[l->tokEnd], toks)) {
    l->tokEnd++;
  }
  while ((l->tokEnd < l->lineEnd) &&
	 isChar(l->buf[l->tokEnd], toks)) {
    l->buf[l->tokEnd] = 0;
    l->tokEnd++;
  }

  return l->buf + l->tokStart;
}

int linep_Reset(LineState* l)
{
  int remaining;
  if (!l->lineComplete) {
    remaining = l->bufLen - l->lineStart;
    memmove(l->buf, l->buf + l->lineStart, remaining);
  } else {
    remaining = 0;
  }
  l->bufLen = remaining;
  l->lineStart = l->lineEnd = 0;
  l->lineComplete = 0;
  return (remaining >= l->bufSize);
}

int linep_ReadFile(LineState* l, FILE* file)
{
  const int len = l->bufSize - l->bufLen;
  const size_t r = fread(l->buf + l->bufLen, 1, len, file);
  if (r == 0) {
    if (ferror(file)) {
      return -1;
    }
  }
  l->bufLen += r;
  return r;
}

int linep_ReadFd(LineState* l, int fd) {
  const int len = l->bufSize - l->bufLen;
  const size_t r = read(fd, l->buf + l->bufLen, len);
  if (r <= 0) {
    return r;
  }
  l->bufLen += r;
  return r;
}

void linep_GetReadInfo(const LineState* l, char** buf, 
		       int* remaining)
{
  if (buf != NULL) {
    *buf = l->buf + l->bufLen;
  }
  if (remaining != NULL) {
    *remaining = l->bufSize - l->bufLen;
  }
}

int linep_GetDataRemaining(const LineState* l)
{
  return (l->bufLen - l->lineEnd);
}

void linep_WriteRemaining(const LineState* l, FILE* out)
{
  fwrite(l->buf + l->lineEnd, l->bufLen - l->lineEnd, 1, out);
}

void linep_Skip(LineState* l, int toSkip)
{
  l->lineEnd += toSkip;
}

void linep_SetReadLength(LineState* l, int len)
{
  l->bufLen += len;
}

void linep_Debug(const LineState* l, FILE* out)
{
  fprintf(out, 
          "buf len = %i line start = %i end = %i tok start = %i end = %i\n",
	  l->bufLen, l->lineStart, l->lineEnd, 
	  l->tokStart, l->tokEnd);
}

void buf_New(StringBuf* b, int sizeHint) {
  int newSize = (sizeHint > 0 ? sizeHint : DEFAULT_STRINGBUF_SIZE);
  b->buf = (char*)malloc(newSize);
  b->pos = 0;
  b->size = newSize;
  b->buf[0] = 0;
}

void buf_Free(StringBuf* b) {
  free(b->buf);
}

static void ensureSpace(StringBuf* b, int newLen) {
  const int neededLen = (b->size - b->pos) + newLen + 1;
  if (neededLen > (b->size - b->pos)) {
    int newAlloc = b->size;
    while (newAlloc < neededLen) {
      newAlloc *= 2;
    }
    b->buf = (char*)realloc(b->buf, newAlloc);
    b->size = newAlloc;
  }
}

void buf_Append(StringBuf* b, const char* s) {
  const int newLen = strlen(s);
  ensureSpace(b, newLen);
  memcpy(b->buf + b->pos, s, newLen);
  b->pos += newLen;
  b->buf[b->pos] = 0;
}

void buf_Printf(StringBuf* b, const char* format, ...) {
  va_list args;
  int remaining;
  int printLen;

  do {
    remaining = b->size - b->pos;
    va_start(args, format);
    printLen = vsnprintf(b->buf + b->pos, remaining, format, args);
    va_end(args);

    if (printLen >= remaining) {
      // vsnprintf couldn't write the whole string in the space provided.
      // We loop this because sometimes it takes a few tries.
      ensureSpace(b, printLen);
    }
  } while (printLen >= remaining);

  b->pos += printLen;
  b->buf[b->pos] = 0; 
}

const char* buf_Get(const StringBuf* b) {
  return b->buf;
}

int buf_Length(const StringBuf* b) {
  return b->pos;
}
