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

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "gtest/gtest.h"
#include "src/apib_iothread.h"
#include "src/apib_message.h"
#include "src/apib_reporting.h"
#include "src/apib_url.h"
#include "test/test_server.h"

static int testServerPort;
static TestServer* testServer;

class IOTest : public ::testing::Test {
 protected:
  IOTest() {
    RecordInit(NULL, NULL);
    RecordStart(1);
    testserver_ResetStats(testServer);
  }
  ~IOTest() {
    // The "url_" family of functions use static data, so reset every time.
    url_Reset();
    EndReporting();
  }
};

static void compareReporting() {
  TestServerStats stats;
  testserver_GetStats(testServer, &stats);
  BenchmarkResults results;
  ReportResults(&results);

  EXPECT_LT(0, results.successfulRequests);
  EXPECT_EQ(0, results.unsuccessfulRequests);
  EXPECT_EQ(0, results.socketErrors);

  EXPECT_EQ(results.successfulRequests, stats.successCount);
  EXPECT_EQ(results.unsuccessfulRequests, stats.errorCount);
  EXPECT_EQ(results.socketErrors, stats.socketErrorCount);
  EXPECT_EQ(results.connectionsOpened, stats.connectionCount);
}

TEST_F(IOTest, OneThread) {
  char url[128];
  sprintf(url, "http://localhost:%i/hello", testServerPort);
  url_InitOne(url);

  //t = (IOThread*)calloc(1, sizeof(IOThread));
  IOThread t;
  memset(&t, 0, sizeof(IOThread));
  t.numConnections = 1;
  // t.verbose = 1;
  t.httpVerb = strdup("GET");

  iothread_Start(&t);
  sleep(1);
  iothread_Stop(&t);
  RecordStop();

  compareReporting();
  free(t.httpVerb);
}

TEST_F(IOTest, OneThreadNoKeepAlive) {
  char url[128];
  sprintf(url, "http://localhost:%i/hello", testServerPort);
  url_InitOne(url);

  IOThread t;
  memset(&t, 0, sizeof(IOThread));
  t.numConnections = 1;
  // t.verbose = 1;
  t.httpVerb = strdup("GET");
  t.noKeepAlive = 1;

  iothread_Start(&t);
  sleep(1);
  iothread_Stop(&t);
  RecordStop();

  compareReporting();
  BenchmarkResults results;
  ReportResults(&results);
  EXPECT_LT(1, results.connectionsOpened);
  EXPECT_EQ(results.completedRequests, results.connectionsOpened);
  free(t.httpVerb);
}

TEST_F(IOTest, OneThreadThinkTime) {
  char url[128];
  sprintf(url, "http://localhost:%i/hello", testServerPort);
  url_InitOne(url);

  IOThread t;
  memset(&t, 0, sizeof(IOThread));
  t.numConnections = 1;
  // t.verbose = 1;
  t.httpVerb = strdup("GET");
  t.thinkTime = 100;

  iothread_Start(&t);
  sleep(1);
  iothread_Stop(&t);
  RecordStop();

  compareReporting();
  free(t.httpVerb);
}

TEST_F(IOTest, OneThreadThinkTimeNoKeepAlive) {
  char url[128];
  sprintf(url, "http://localhost:%i/hello", testServerPort);
  url_InitOne(url);

  IOThread t;
  memset(&t, 0, sizeof(IOThread));
  t.numConnections = 1;
  // t.verbose = 1;
  t.httpVerb = strdup("GET");
  t.thinkTime = 100;
  t.noKeepAlive = 1;

  iothread_Start(&t);
  sleep(1);
  iothread_Stop(&t);
  RecordStop();

  compareReporting();
  free(t.httpVerb);
}

TEST_F(IOTest, OneThreadLarge) {
  char url[128];
  sprintf(url, "http://localhost:%i/data?size=4000", testServerPort);
  url_InitOne(url);

  IOThread t;
  memset(&t, 0, sizeof(IOThread));
  t.numConnections = 1;
  // t.verbose = 1;
  t.httpVerb = strdup("GET");

  iothread_Start(&t);
  sleep(1);
  iothread_Stop(&t);
  RecordStop();

  compareReporting();
  free(t.httpVerb);
}

TEST_F(IOTest, MoreConnections) {
  char url[128];
  sprintf(url, "http://localhost:%i/hello", testServerPort);
  url_InitOne(url);

  IOThread t;
  memset(&t, 0, sizeof(IOThread));
  t.numConnections = 10;
  // t.verbose = 1;
  t.httpVerb = strdup("GET");

  iothread_Start(&t);
  sleep(1);
  iothread_Stop(&t);
  RecordStop();

  compareReporting();
  free(t.httpVerb);
}

TEST_F(IOTest, ResizeCommand) {
  char url[128];
  sprintf(url, "http://localhost:%i/hello", testServerPort);
  url_InitOne(url);

  IOThread t;
  memset(&t, 0, sizeof(IOThread));
  t.numConnections = 1;
  // t.verbose = 1;
  t.httpVerb = strdup("GET");

  iothread_Start(&t);
  usleep(250000);
  iothread_SetNumConnections(&t, 5);
  usleep(250000);
  iothread_SetNumConnections(&t, 2);
  iothread_SetNumConnections(&t, 3);
  iothread_SetNumConnections(&t, 1);
  usleep(250000);
  iothread_Stop(&t);
  RecordStop();

  compareReporting();
  free(t.httpVerb);
}

TEST_F(IOTest, ResizeFromZero) {
  char url[128];
  sprintf(url, "http://localhost:%i/hello", testServerPort);
  url_InitOne(url);

  IOThread t;
  memset(&t, 0, sizeof(IOThread));
  t.numConnections = 0;
  // t.verbose = 1;
  t.httpVerb = strdup("GET");

  iothread_Start(&t);
  usleep(250000);
  iothread_SetNumConnections(&t, 5);
  usleep(250000);
  iothread_Stop(&t);
  RecordStop();

  compareReporting();
  free(t.httpVerb);
}

#define POST_LEN 3000

TEST_F(IOTest, OneThreadBigPost) {
  char url[128];
  sprintf(url, "http://localhost:%i/echo", testServerPort);
  url_InitOne(url);

  IOThread t;
  memset(&t, 0, sizeof(IOThread));
  t.numConnections = 1;
  // t.verbose = 1;
  t.httpVerb = strdup("POST");
  t.sendData = (char*)malloc(POST_LEN);
  t.sendDataLen = POST_LEN;

  for (int p = 0; p < POST_LEN; p += 10) {
    memcpy(t.sendData + p, "abcdefghij", 10);
  }

  iothread_Start(&t);
  sleep(1);
  iothread_Stop(&t);
  RecordStop();

  compareReporting();
  free(t.httpVerb);
}

TEST_F(IOTest, OneThreadHeaders) {
  char url[128];
  sprintf(url, "http://localhost:%i/hello", testServerPort);
  url_InitOne(url);

  IOThread t;
  memset(&t, 0, sizeof(IOThread));
  t.numConnections = 1;
  // t.verbose = 1;
  t.httpVerb = strdup("GET");
  t.numHeaders = 1;
  t.headers = (char**)malloc(sizeof(char*));
  t.headers[0] = strdup("Authorization: Basic dGVzdDp2ZXJ5dmVyeXNlY3JldA==");

  iothread_Start(&t);
  sleep(1);
  iothread_Stop(&t);
  RecordStop();

  compareReporting();
  free(t.httpVerb);
  free(t.headers[0]);
  free(t.headers);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);

  testServer = (TestServer*)malloc(sizeof(TestServer));
  int err = testserver_Start(testServer, 0, NULL, NULL);
  if (err != 0) {
    fprintf(stderr, "Can't start test server: %i\n", err);
    return 2;
  }
  testServerPort = testserver_GetPort(testServer);

  int r = RUN_ALL_TESTS();

  testserver_Stop(testServer);
  // testserver_Join(&svr);
  free(testServer);

  return r;
}