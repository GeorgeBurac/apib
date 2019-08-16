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

#include "src/apib_iothread.h"
#include "src/apib_reporting.h"
#include "gtest/gtest.h"

class Reporting : public ::testing::Test {
  protected:
   Reporting() {
     RecordInit(NULL, NULL);
   }
   ~Reporting() {
     EndReporting();
   }
};

TEST_F(Reporting, ReportingZero) {
  RecordStart(1);
  RecordStop();
  BenchmarkResults r;
  ReportResults(&r);
  EXPECT_EQ(0, r.completedRequests);
  EXPECT_EQ(0, r.successfulRequests);
  EXPECT_EQ(0, r.unsuccessfulRequests);
  EXPECT_EQ(0, r.socketErrors);
  EXPECT_EQ(0, r.connectionsOpened);
  EXPECT_EQ(0, r.totalBytesSent);
  EXPECT_EQ(0, r.totalBytesReceived);
}

TEST_F(Reporting, ReportingCount) {
  RecordStart(1);
  RecordConnectionOpen();
  RecordResult(200);
  RecordResult(201);
  RecordResult(204);
  RecordResult(403);
  RecordResult(401);
  RecordResult(500);
  RecordSocketError();
  RecordConnectionOpen();
  RecordStop();

  BenchmarkResults r;
  ReportResults(&r);

  EXPECT_EQ(6, r.completedRequests);
  EXPECT_EQ(3, r.successfulRequests);
  EXPECT_EQ(3, r.unsuccessfulRequests);
  EXPECT_EQ(1, r.socketErrors);
  EXPECT_EQ(2, r.connectionsOpened);
  EXPECT_EQ(0, r.totalBytesSent);
  EXPECT_EQ(0, r.totalBytesReceived);
}

TEST_F(Reporting, ReportingInterval) {
  RecordStart(1);
  RecordConnectionOpen();
  RecordResult(200);
  RecordResult(201);
  RecordResult(400);

  BenchmarkIntervalResults ri;
  ReportIntervalResults(&ri);
  EXPECT_EQ(2, ri.successfulRequests);
  EXPECT_LT(0.0, ri.averageThroughput);

  RecordResult(204);
  RecordResult(403);
  RecordResult(401);
  RecordResult(500);
  RecordResult(200);

  ReportIntervalResults(&ri);
  EXPECT_EQ(2, ri.successfulRequests);
  EXPECT_LT(0.0, ri.averageThroughput);

  RecordStop();
  BenchmarkResults r;
  ReportResults(&r);

  EXPECT_EQ(8, r.completedRequests);
  EXPECT_EQ(4, r.successfulRequests);
  EXPECT_EQ(4, r.unsuccessfulRequests);
  EXPECT_EQ(0, r.socketErrors);
  EXPECT_EQ(1, r.connectionsOpened);
  EXPECT_EQ(0, r.totalBytesSent);
  EXPECT_EQ(0, r.totalBytesReceived);
}

TEST_F(Reporting, Latencies) {
  IOThread threads[2];
  threads[0].latencies = (long long*)malloc(sizeof(long long) * 4);
  threads[0].latencies[0] = 100000000;
  threads[0].latencies[1] = 110000000;
  threads[0].latencies[2] = 140000000;
  threads[0].latencies[3] = 100000000;
  threads[0].latenciesSize = threads[0].latenciesCount = 4;
  threads[0].readBytes = 123;
  threads[0].writeBytes = 456;

  threads[1].latencies = (long long*)malloc(sizeof(long long) * 3);
  threads[1].latencies[0] = 50000000;
  threads[1].latencies[1] = 60000000;
  threads[1].latencies[2] = 70000000;
  threads[1].latenciesSize = threads[1].latenciesCount = 3;
  threads[1].readBytes = 999;
  threads[1].writeBytes = 1000;

  ConsolidateLatencies(threads, 2);

  free(threads[0].latencies);
  free(threads[1].latencies);

  BenchmarkResults r;
  ReportResults(&r);
  EXPECT_EQ(50.0, r.latencies[0]);
  EXPECT_EQ(140.0, r.latencies[100]);
  EXPECT_EQ(123 + 999, r.totalBytesReceived);
  EXPECT_EQ(456 + 1000, r.totalBytesSent);
}