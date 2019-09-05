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

#ifndef APIB_IOTHREAD_H
#define APIB_IOTHREAD_H

#include <openssl/ssl.h>
#include <pthread.h>

#include "ev.h"
#include "http_parser.h"
#include "src/apib_lines.h"
#include "src/apib_oauth.h"
#include "src/apib_rand.h"
#include "src/apib_url.h"

#ifdef __cplusplus
extern "C" {
#endif

struct connState;
struct threadCommand;

typedef enum { STOP, SET_CONNECTIONS } ThreadCmd;

// This is used to send instructions to the thread from outside.
typedef struct threadCommand {
  ThreadCmd command;
  int newNumConnections;
  int stopTimeoutSecs;
  struct threadCommand* next;
} Command;

// This is a generic thread-safe queue for commands.
typedef struct {
  Command* head;
  pthread_mutex_t lock;
} CommandQueue;

// This structure represents a single thread that runs a benchmark
// across multiple connections.
typedef struct {
  // The caller must initialize this first section before using.
  int index;
  int numConnections;
  int verbose;
  char* httpVerb;
  char* sslCipher;
  char* sendData;
  size_t sendDataLen;
  SSL_CTX* sslCtx;
  OAuthInfo* oauth;
  char** headers;
  unsigned int numHeaders;
  unsigned int thinkTime;
  int hostHeaderOverride;
  int noKeepAlive;

  // Everything ABOVE must be initialized.
  long readCount;
  long writeCount;
  long long readBytes;
  long long writeBytes;
  struct connState** connections;

  // Internal stuff -- no need for anyone to set
  pthread_t thread;
  int keepRunning;
  RandState randState;
  struct ev_loop* loop;
  ev_async async;
  CommandQueue commands;
  ev_timer shutdownTimer;
} IOThread;

#define READ_BUF_SIZE 1024
#define WRITE_BUF_SIZE 128

extern http_parser_settings HttpParserSettings;

// This is an internal structure used per connection.
typedef struct connState {
  int index;
  int keepRunning;
  IOThread* t;
  int fd;
  SSL* ssl;
  int backwardsIo;
  ev_io io;
  ev_timer thinkTimer;
  URLInfo* url;
  StringBuf writeBuf;
  size_t writeBufPos;
  char* readBuf;
  size_t readBufPos;
  http_parser parser;
  int readDone;
  int needsOpen;
  long long startTime;
} ConnectionState;

// Start the thread. It's up to the caller to initialize everything
// in the structure above. This call will spawn a thread, and keep
// running until "iothread_Stop" is called.
extern void iothread_Start(IOThread* t);

// Stop the thread. It will signal for a stop, and then stop
// more forcefully after "timeoutSecs" seconds
extern void iothread_RequestStop(IOThread* t, int timeoutSecs);

// Wait for the thread to exit cleanly.
extern void iothread_Join(IOThread* t);

// Convenience that stops and joins all at once with a one-second timeout
extern void iothread_Stop(IOThread* t);

// Change the number of connections. This will happen as part of normal
// processing, with unneeded connections shutting down when done
// with their current requests.
extern void iothread_SetNumConnections(IOThread* t, int newConnections);

// These are internal methods used by apib_iothread.c for various
// implementations.
// extern void io_Verbose(ConnectionState* c, const char* format, ...);
extern void io_WriteDone(ConnectionState* c, int err);
extern void io_ReadDone(ConnectionState* c, int err);
extern void io_CloseDone(ConnectionState* c);

// High-level operations:

// Connect in a non-blocking way, and return non-zero on error.
extern int io_Connect(ConnectionState* c);

// Write what's in "sendBuf" to the socket, and call io_WriteDone when done.
extern void io_SendWrite(ConnectionState* c);

// Read the whole HTTP response and call "io_ReadDone" when done.
extern void io_SendRead(ConnectionState* c);

// Do what it says on the tin, and call "io_CloseDone" when done.
extern void io_Close(ConnectionState* c);

// Lower-level operations
typedef enum {
  OK,
  NEED_READ,
  NEED_WRITE,
  FEOF,
  TLS_ERROR,
  SOCKET_ERROR
} IOStatus;

extern IOStatus io_Write(ConnectionState* c, const void* buf, size_t count,
                         size_t* written);
extern IOStatus io_Read(ConnectionState* c, void* buf, size_t count,
                        size_t* readed);
extern IOStatus io_CloseConnection(ConnectionState* c);
extern void io_FreeConnection(ConnectionState* c);

// Functions specific to the command queue.
extern void command_Init(CommandQueue* q);
extern void command_Free(CommandQueue* q);
// The caller should not free "cmd" after calling this one.
extern void command_Add(CommandQueue* q, Command* cmd);
// The caller should indeed free the result if it's not NULL
extern Command* command_Pop(CommandQueue* q);

// Debugging macro
#define io_Verbose(c, ...) \
  if ((c)->t->verbose) {   \
    printf(__VA_ARGS__);   \
  }

#define iothread_Verbose(t, ...) \
  if ((t)->verbose) {            \
    printf(__VA_ARGS__);         \
  }

#ifdef __cplusplus
}
#endif

#endif  // APIB_IOTHREAD_H