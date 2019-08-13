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

#include "src/apib_iothread.h"
#include "src/apib_url.h"
#include "src/apib_message.h"
#include "test/test_server.h"

#include "gtest/gtest.h"

static int testServerPort;

TEST(IO, OneThread) {
  char url[128];
  sprintf(url, "http://localhost:%i", testServerPort);
  url_InitOne(url);

  IOThread t;
  memset(&t, 0, sizeof(IOThread));
  t.numConnections = 1;
  t.verbose = 1;
  t.httpVerb = strdup("GET");

  message_Init();

  iothread_Start(&t);
  sleep(2);
  iothread_Stop(&t);

  free(t.httpVerb);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);

  TestServer svr;
  int err = testserver_Start(&svr, 0);
  if (err != 0) {
    fprintf(stderr, "Can't start test server: %i\n", err);
    return 2;
  }
  testServerPort = testserver_GetPort(&svr);

  int r = RUN_ALL_TESTS();

  testserver_Stop(&svr);
  // testserver_Join(&svr);

  return r;
}