// Copyright 2018 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "google/cloud/bigtable/table.h"
#include "google/cloud/bigtable/testing/mock_sample_row_keys_reader.h"
#include "google/cloud/bigtable/testing/table_test_fixture.h"
#include "google/cloud/testing_util/assert_ok.h"
#include "google/cloud/testing_util/chrono_literals.h"
#include <typeinfo>

namespace bigtable = google::cloud::bigtable;
using ::google::cloud::testing_util::chrono_literals::operator"" _us;
using ::testing::_;
using ::testing::Return;

/// Define types and functions used for this tests.
namespace {
class TableSampleRowKeysTest : public bigtable::testing::TableTestFixture {};
using bigtable::testing::MockSampleRowKeysReader;
}  // anonymous namespace

/// @test Verify that Table::SampleRows<T>() works for default parameter.
TEST_F(TableSampleRowKeysTest, DefaultParameterTest) {
  namespace btproto = ::google::bigtable::v2;

  auto reader =
      new MockSampleRowKeysReader("google.bigtable.v2.Bigtable.SampleRowKeys");
  EXPECT_CALL(*client_, SampleRowKeys(_, _))
      .WillOnce(reader->MakeMockReturner());
  EXPECT_CALL(*reader, Read(_))
      .WillOnce([](btproto::SampleRowKeysResponse* r) {
        {
          r->set_row_key("test1");
          r->set_offset_bytes(11);
        }
        return true;
      })
      .WillOnce(Return(false));
  EXPECT_CALL(*reader, Finish()).WillOnce(Return(grpc::Status::OK));
  auto result = table_.SampleRows();
  ASSERT_STATUS_OK(result);
  auto it = result->begin();
  EXPECT_NE(it, result->end());
  EXPECT_EQ(it->row_key, "test1");
  EXPECT_EQ(it->offset_bytes, 11);
  EXPECT_EQ(++it, result->end());
}

/// @test Verify that Table::SampleRows<T>() works for std::vector.
TEST_F(TableSampleRowKeysTest, SimpleVectorTest) {
  namespace btproto = ::google::bigtable::v2;

  auto reader =
      new MockSampleRowKeysReader("google.bigtable.v2.Bigtable.SampleRowKeys");
  EXPECT_CALL(*client_, SampleRowKeys(_, _))
      .WillOnce(reader->MakeMockReturner());
  EXPECT_CALL(*reader, Read(_))
      .WillOnce([](btproto::SampleRowKeysResponse* r) {
        {
          r->set_row_key("test1");
          r->set_offset_bytes(11);
        }
        return true;
      })
      .WillOnce(Return(false));
  EXPECT_CALL(*reader, Finish()).WillOnce(Return(grpc::Status::OK));
  auto result = table_.SampleRows();
  ASSERT_STATUS_OK(result);
  auto it = result->begin();
  EXPECT_NE(it, result->end());
  EXPECT_EQ(it->row_key, "test1");
  EXPECT_EQ(it->offset_bytes, 11);
  EXPECT_EQ(++it, result->end());
}

TEST_F(TableSampleRowKeysTest, SampleRowKeysRetryTest) {
  namespace btproto = ::google::bigtable::v2;

  auto reader =
      new MockSampleRowKeysReader("google.bigtable.v2.Bigtable.SampleRowKeys");
  auto reader_retry =
      new MockSampleRowKeysReader("google.bigtable.v2.Bigtable.SampleRowKeys");
  EXPECT_CALL(*client_, SampleRowKeys(_, _))
      .WillOnce(reader->MakeMockReturner())
      .WillOnce(reader_retry->MakeMockReturner());

  EXPECT_CALL(*reader, Read(_))
      .WillOnce([](btproto::SampleRowKeysResponse* r) {
        {
          r->set_row_key("test1");
          r->set_offset_bytes(11);
        }
        return true;
      })
      .WillOnce(Return(false));

  EXPECT_CALL(*reader, Finish())
      .WillOnce(
          Return(grpc::Status(grpc::StatusCode::UNAVAILABLE, "try-again")));

  EXPECT_CALL(*reader_retry, Read(_))
      .WillOnce([](btproto::SampleRowKeysResponse* r) {
        {
          r->set_row_key("test2");
          r->set_offset_bytes(123);
        }
        return true;
      })
      .WillOnce([](btproto::SampleRowKeysResponse* r) {
        {
          r->set_row_key("test3");
          r->set_offset_bytes(1234);
        }
        return true;
      })
      .WillOnce(Return(false));

  EXPECT_CALL(*reader_retry, Finish()).WillOnce(Return(grpc::Status::OK));

  auto results = table_.SampleRows();
  ASSERT_STATUS_OK(results);

  auto it = results->begin();
  EXPECT_NE(it, results->end());
  EXPECT_EQ("test2", it->row_key);
  ++it;
  EXPECT_NE(it, results->end());
  EXPECT_EQ("test3", it->row_key);
  EXPECT_EQ(++it, results->end());
}

#if GOOGLE_CLOUD_CPP_HAVE_EXCEPTIONS
/// @test Verify that Table::sample_rows() reports correctly on too many errors.
TEST_F(TableSampleRowKeysTest, TooManyFailures) {
  namespace btproto = ::google::bigtable::v2;

  // Create a table with specific policies so we can test the behavior
  // without having to depend on timers expiring.  In this case tolerate only
  // 3 failures.
  ::bigtable::Table custom_table(
      client_, "foo_table",
      // Configure the Table to stop at 3 failures.
      ::bigtable::LimitedErrorCountRetryPolicy(2),
      // Use much shorter backoff than the default to test faster.
      ::bigtable::ExponentialBackoffPolicy(10_us, 40_us),
      ::bigtable::SafeIdempotentMutationPolicy());

  // Setup the mocks to fail more than 3 times.
  auto r1 =
      new MockSampleRowKeysReader("google.bigtable.v2.Bigtable.SampleRowKeys");
  EXPECT_CALL(*r1, Read(_))
      .WillOnce([](btproto::SampleRowKeysResponse* r) {
        {
          r->set_row_key("test1");
          r->set_offset_bytes(11);
        }
        return true;
      })
      .WillOnce(Return(false));
  EXPECT_CALL(*r1, Finish())
      .WillOnce(Return(grpc::Status(grpc::StatusCode::ABORTED, "")));

  auto create_cancelled_stream = [&](grpc::ClientContext*,
                                     btproto::SampleRowKeysRequest const&) {
    auto stream = new MockSampleRowKeysReader(
        "google.bigtable.v2.Bigtable.SampleRowKeys");
    EXPECT_CALL(*stream, Read(_)).WillOnce(Return(false));
    EXPECT_CALL(*stream, Finish())
        .WillOnce(Return(grpc::Status(grpc::StatusCode::ABORTED, "")));
    return stream->AsUniqueMocked();
  };

  EXPECT_CALL(*client_, SampleRowKeys(_, _))
      .WillOnce(r1->MakeMockReturner())
      .WillOnce(create_cancelled_stream)
      .WillOnce(create_cancelled_stream);

  EXPECT_FALSE(custom_table.SampleRows());
}
#endif  // GOOGLE_CLOUD_CPP_HAVE_EXCEPTIONS
