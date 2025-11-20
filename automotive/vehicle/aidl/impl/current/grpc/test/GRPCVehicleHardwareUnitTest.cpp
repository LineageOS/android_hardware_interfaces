// Copyright (C) 2023 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "GRPCVehicleHardware.h"
#include "VehicleServer.grpc.pb.h"
#include "VehicleServer.pb.h"
#include "VehicleServer_mock.grpc.pb.h"

#include <gmock/gmock.h>
#include <grpc++/grpc++.h>
#include <grpcpp/test/mock_stream.h>
#include <gtest/gtest.h>

#include <utils/SystemClock.h>
#include <chrono>
#include <memory>
#include <string>

namespace android::hardware::automotive::vehicle::virtualization {

namespace aidlvhal = ::aidl::android::hardware::automotive::vehicle;

using ::testing::_;
using ::testing::DoAll;
using ::testing::ElementsAre;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::SaveArg;
using ::testing::SetArgPointee;
using ::testing::SizeIs;

using ::grpc::Status;
using ::grpc::testing::MockClientReader;

using proto::MockVehicleServerStub;

class GRPCVehicleHardwareUnitTest : public ::testing::Test {
  protected:
    NiceMock<MockVehicleServerStub>* mGrpcStub;
    std::unique_ptr<GRPCVehicleHardware> mHardware;

    void SetUp() override {
        auto stub = std::make_unique<NiceMock<MockVehicleServerStub>>();
        mGrpcStub = stub.get();
        // Cannot use make_unique here since the constructor is a private method.
        mHardware = std::unique_ptr<GRPCVehicleHardware>(
                new GRPCVehicleHardware(std::move(stub), /*startValuePollingLoop=*/false));
    }

    void TearDown() override { mHardware.reset(); }

    // Access GRPCVehicleHardware private method.
    void pollValue() { mHardware->pollValue(); }

    Status pollSupportedValuesChange() { return mHardware->pollSupportedValuesChange(); }

    void startValuePollingLoop(std::unique_ptr<proto::VehicleServer::StubInterface> stub) {
        mHardware = std::unique_ptr<GRPCVehicleHardware>(
                new GRPCVehicleHardware(std::move(stub), /*startValuePollingLoop=*/true));
    }

    void generatePropertyUpdateEvent(int32_t propId, int64_t timestamp);
};

MATCHER_P(RepeatedInt32Eq, expected_values, "") {
    return std::vector<int32_t>(arg.begin(), arg.end()) == expected_values;
}

TEST_F(GRPCVehicleHardwareUnitTest, TestSubscribe) {
    proto::VehicleHalCallStatus protoStatus;
    protoStatus.set_status_code(proto::StatusCode::OK);
    proto::SubscribeRequest actualRequest;

    EXPECT_CALL(*mGrpcStub, Subscribe(_, _, _))
            .WillOnce(DoAll(SaveArg<1>(&actualRequest), SetArgPointee<2>(protoStatus),
                            Return(::grpc::Status::OK)));

    aidlvhal::SubscribeOptions options = {.propId = 1,
                                          .areaIds = {1, 2, 3, 4},
                                          .sampleRate = 1.234,
                                          .resolution = 0.01,
                                          .enableVariableUpdateRate = true};
    auto status = mHardware->subscribe(options);

    EXPECT_EQ(status, aidlvhal::StatusCode::OK);
    const auto& protoOptions = actualRequest.options();
    EXPECT_EQ(protoOptions.prop_id(), 1);
    EXPECT_THAT(protoOptions.area_ids(), RepeatedInt32Eq(std::vector<int32_t>({1, 2, 3, 4})));
    EXPECT_FLOAT_EQ(protoOptions.sample_rate(), 1.234);
    EXPECT_FLOAT_EQ(protoOptions.resolution(), 0.01);
    EXPECT_EQ(protoOptions.enable_variable_update_rate(), true);
}

TEST_F(GRPCVehicleHardwareUnitTest, TestSubscribeLegacyServer) {
    EXPECT_CALL(*mGrpcStub, Subscribe(_, _, _))
            .WillOnce(Return(::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "")));

    aidlvhal::SubscribeOptions options;
    auto status = mHardware->subscribe(options);

    EXPECT_EQ(status, aidlvhal::StatusCode::OK);
}

TEST_F(GRPCVehicleHardwareUnitTest, TestSubscribeGrpcFailure) {
    EXPECT_CALL(*mGrpcStub, Subscribe(_, _, _))
            .WillOnce(Return(::grpc::Status(::grpc::StatusCode::INTERNAL, "GRPC Error")));

    aidlvhal::SubscribeOptions options;
    auto status = mHardware->subscribe(options);

    EXPECT_EQ(status, aidlvhal::StatusCode::INTERNAL_ERROR);
}

TEST_F(GRPCVehicleHardwareUnitTest, TestSubscribeProtoFailure) {
    proto::VehicleHalCallStatus protoStatus;
    protoStatus.set_status_code(proto::StatusCode::NOT_AVAILABLE_SPEED_LOW);

    EXPECT_CALL(*mGrpcStub, Subscribe(_, _, _))
            .WillOnce(DoAll(SetArgPointee<2>(protoStatus),  // Set the output status
                            Return(::grpc::Status::OK)));

    aidlvhal::SubscribeOptions options;
    auto status = mHardware->subscribe(options);

    EXPECT_EQ(status, aidlvhal::StatusCode::NOT_AVAILABLE_SPEED_LOW);
}

TEST_F(GRPCVehicleHardwareUnitTest, TestUnsubscribe) {
    proto::VehicleHalCallStatus protoStatus;
    protoStatus.set_status_code(proto::StatusCode::OK);
    proto::UnsubscribeRequest actualRequest;

    EXPECT_CALL(*mGrpcStub, Unsubscribe(_, _, _))
            .WillOnce(DoAll(SaveArg<1>(&actualRequest), SetArgPointee<2>(protoStatus),
                            Return(::grpc::Status::OK)));

    int32_t propId = 1;
    int32_t areaId = 2;
    auto status = mHardware->unsubscribe(propId, areaId);

    EXPECT_EQ(status, aidlvhal::StatusCode::OK);
    EXPECT_EQ(actualRequest.prop_id(), propId);
    EXPECT_EQ(actualRequest.area_id(), areaId);
}

TEST_F(GRPCVehicleHardwareUnitTest, TestUnsubscribeLegacyServer) {
    EXPECT_CALL(*mGrpcStub, Unsubscribe(_, _, _))
            .WillOnce(Return(::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "")));

    auto status = mHardware->unsubscribe(1, 2);

    EXPECT_EQ(status, aidlvhal::StatusCode::OK);
}

TEST_F(GRPCVehicleHardwareUnitTest, TestUnsubscribeGrpcFailure) {
    EXPECT_CALL(*mGrpcStub, Unsubscribe(_, _, _))
            .WillOnce(Return(::grpc::Status(::grpc::StatusCode::INTERNAL, "GRPC Error")));

    auto status = mHardware->unsubscribe(1, 2);

    EXPECT_EQ(status, aidlvhal::StatusCode::INTERNAL_ERROR);
}

TEST_F(GRPCVehicleHardwareUnitTest, TestUnsubscribeProtoFailure) {
    proto::VehicleHalCallStatus protoStatus;
    protoStatus.set_status_code(proto::StatusCode::NOT_AVAILABLE_SPEED_LOW);

    EXPECT_CALL(*mGrpcStub, Unsubscribe(_, _, _))
            .WillOnce(DoAll(SetArgPointee<2>(protoStatus),  // Set the output status
                            Return(::grpc::Status::OK)));

    auto status = mHardware->unsubscribe(1, 2);

    EXPECT_EQ(status, aidlvhal::StatusCode::NOT_AVAILABLE_SPEED_LOW);
}

TEST_F(GRPCVehicleHardwareUnitTest, TestPollValue) {
    int64_t testTimestamp = 12345;
    int32_t testPropId = 54321;
    int64_t startTimestamp = elapsedRealtimeNano();

    // This will be converted to a unique_ptr in StartPropertyValuesStream. The ownership is passed
    // there.
    auto clientReader = new MockClientReader<proto::VehiclePropValues>();
    EXPECT_CALL(*mGrpcStub, StartPropertyValuesStreamRaw(_, _)).WillOnce(Return(clientReader));
    EXPECT_CALL(*clientReader, Read(_))
            .WillOnce([testTimestamp, testPropId](proto::VehiclePropValues* values) {
                values->Clear();
                auto value = values->add_values();
                value->set_timestamp(testTimestamp);
                value->set_prop(testPropId);
                return true;
            })
            .WillOnce(Return(false));
    EXPECT_CALL(*clientReader, Finish()).WillOnce(Return(::grpc::Status::OK));

    std::vector<aidlvhal::VehiclePropValue> propertyEvents;

    mHardware->registerOnPropertyChangeEvent(
            std::make_unique<GRPCVehicleHardware::PropertyChangeCallback>(
                    [&propertyEvents](const std::vector<aidlvhal::VehiclePropValue>& events) {
                        for (const auto& event : events) {
                            propertyEvents.push_back(event);
                        }
                    }));

    pollValue();

    ASSERT_THAT(propertyEvents, SizeIs(1));
    EXPECT_EQ(propertyEvents[0].prop, testPropId);
    EXPECT_GT(propertyEvents[0].timestamp, startTimestamp)
            << "Timestamp must be updated to Android timestamp";
    EXPECT_LT(propertyEvents[0].timestamp, elapsedRealtimeNano())
            << "Timestamp must be updated to Android timestamp";
}

TEST_F(GRPCVehicleHardwareUnitTest, TestPollValueIgnoreOutdatedValue) {
    int64_t testTimestamp1 = 12345;
    int32_t value1 = 1324;
    int64_t testTimestamp2 = 12340;
    int32_t value2 = 1423;
    int32_t testPropId = 54321;
    int64_t startTimestamp = elapsedRealtimeNano();

    // This will be converted to a unique_ptr in StartPropertyValuesStream. The ownership is passed
    // there.
    auto clientReader = new MockClientReader<proto::VehiclePropValues>();
    EXPECT_CALL(*mGrpcStub, StartPropertyValuesStreamRaw(_, _)).WillOnce(Return(clientReader));
    EXPECT_CALL(*clientReader, Read(_))
            .WillOnce([testTimestamp1, value1, testPropId](proto::VehiclePropValues* values) {
                values->Clear();
                auto value = values->add_values();
                value->set_timestamp(testTimestamp1);
                value->set_prop(testPropId);
                value->add_int32_values(value1);
                return true;
            })
            .WillOnce([testTimestamp2, value2, testPropId](proto::VehiclePropValues* values) {
                values->Clear();
                // This event is outdated, must be ignored.
                auto value = values->add_values();
                value->set_timestamp(testTimestamp2);
                value->set_prop(testPropId);
                value->add_int32_values(value2);
                return true;
            })
            .WillOnce(Return(false));
    EXPECT_CALL(*clientReader, Finish()).WillOnce(Return(::grpc::Status::OK));

    std::vector<aidlvhal::VehiclePropValue> propertyEvents;

    mHardware->registerOnPropertyChangeEvent(
            std::make_unique<GRPCVehicleHardware::PropertyChangeCallback>(
                    [&propertyEvents](const std::vector<aidlvhal::VehiclePropValue>& events) {
                        for (const auto& event : events) {
                            propertyEvents.push_back(event);
                        }
                    }));

    pollValue();

    ASSERT_THAT(propertyEvents, SizeIs(1)) << "Outdated event must be ignored";
    EXPECT_EQ(propertyEvents[0].prop, testPropId);
    EXPECT_GT(propertyEvents[0].timestamp, startTimestamp);
    EXPECT_LT(propertyEvents[0].timestamp, elapsedRealtimeNano());
    EXPECT_THAT(propertyEvents[0].value.int32Values, ElementsAre(value1));
}

TEST_F(GRPCVehicleHardwareUnitTest, TestValuePollingLoop) {
    int64_t testTimestamp = 12345;
    int32_t testPropId = 54321;
    int32_t testAreaId = 123;
    auto stub = std::make_unique<NiceMock<MockVehicleServerStub>>();

    // This will be converted to a unique_ptr in StartPropertyValuesStream. The ownership is passed
    // there.
    auto valueReader = new MockClientReader<proto::VehiclePropValues>();
    EXPECT_CALL(*stub, StartPropertyValuesStreamRaw(_, _)).WillOnce(Return(valueReader));
    EXPECT_CALL(*valueReader, Read(_))
            .WillRepeatedly([testTimestamp, testPropId](proto::VehiclePropValues* values) {
                // Sleep for 10ms and always return the same property event.
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                values->Clear();
                auto value = values->add_values();
                value->set_timestamp(testTimestamp);
                value->set_prop(testPropId);
                return true;
            });
    EXPECT_CALL(*valueReader, Finish()).WillOnce(Return(::grpc::Status::OK));

    auto supportedValuesChangeReader = new MockClientReader<proto::SupportedValuesChange>();
    EXPECT_CALL(*stub, StartSupportedValuesChangeStreamRaw(_, _))
            .WillOnce(Return(supportedValuesChangeReader));
    EXPECT_CALL(*supportedValuesChangeReader, Read(_))
            .WillRepeatedly([testPropId, testAreaId](proto::SupportedValuesChange* values) {
                // Sleep for 10ms and always return the same SupportedValuesChange
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                values->Clear();
                auto value = values->add_prop_id_area_ids();
                value->set_prop_id(testPropId);
                value->set_area_id(testAreaId);
                return true;
            });
    EXPECT_CALL(*supportedValuesChangeReader, Finish()).WillOnce(Return(::grpc::Status::OK));

    startValuePollingLoop(std::move(stub));

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // This must stop the loop and wait for the thread to finish.
    mHardware.reset();
}

TEST_F(GRPCVehicleHardwareUnitTest, TestPollSupportedValuesChange) {
    int32_t testPropId = 1234;
    int32_t testAreaId = 4321;

    // This will be converted to a unique_ptr in StartPropertyValuesStream. The ownership is passed
    // there.
    auto clientReader = new MockClientReader<proto::SupportedValuesChange>();
    EXPECT_CALL(*mGrpcStub, StartSupportedValuesChangeStreamRaw(_, _))
            .WillOnce(Return(clientReader));
    EXPECT_CALL(*clientReader, Read(_))
            .WillOnce(
                    [testPropId, testAreaId](proto::SupportedValuesChange* supportedValuesChange) {
                        supportedValuesChange->Clear();
                        auto propIdAreaId = supportedValuesChange->add_prop_id_area_ids();
                        propIdAreaId->set_prop_id(testPropId);
                        propIdAreaId->set_area_id(testAreaId);
                        return true;
                    })
            .WillOnce(Return(false));
    EXPECT_CALL(*clientReader, Finish()).WillOnce(Return(grpc::Status::OK));

    std::vector<PropIdAreaId> updatedPropIdAreaIds;

    mHardware->registerSupportedValueChangeCallback(
            std::make_unique<GRPCVehicleHardware::SupportedValueChangeCallback>(
                    [&updatedPropIdAreaIds](const std::vector<PropIdAreaId>& propIdAreaIds) {
                        for (const auto& propIdAreaId : propIdAreaIds) {
                            updatedPropIdAreaIds.push_back(propIdAreaId);
                        }
                    }));

    auto status = pollSupportedValuesChange();

    ASSERT_TRUE(status.ok()) << "Status error: " << status.error_message();
    ASSERT_THAT(updatedPropIdAreaIds, SizeIs(1));
    EXPECT_EQ(updatedPropIdAreaIds[0].propId, testPropId);
    EXPECT_EQ(updatedPropIdAreaIds[0].areaId, testAreaId);
}

TEST_F(GRPCVehicleHardwareUnitTest, TestPollSupportedValuesChange_NotImplemented) {
    int32_t testPropId = 1234;
    int32_t testAreaId = 4321;
    auto status = grpc::Status(grpc::StatusCode::UNIMPLEMENTED, "");

    // This will be converted to a unique_ptr in StartPropertyValuesStream. The ownership is passed
    // there.
    auto clientReader = new MockClientReader<proto::SupportedValuesChange>();
    EXPECT_CALL(*mGrpcStub, StartSupportedValuesChangeStreamRaw(_, _))
            .WillOnce(Return(clientReader));
    EXPECT_CALL(*clientReader, Read(_)).WillOnce(Return(false));
    EXPECT_CALL(*clientReader, Finish()).WillOnce(Return(status));

    std::vector<PropIdAreaId> updatedPropIdAreaIds;

    mHardware->registerSupportedValueChangeCallback(
            std::make_unique<GRPCVehicleHardware::SupportedValueChangeCallback>(
                    [&updatedPropIdAreaIds](const std::vector<PropIdAreaId>& propIdAreaIds) {
                        for (const auto& propIdAreaId : propIdAreaIds) {
                            updatedPropIdAreaIds.push_back(propIdAreaId);
                        }
                    }));

    auto gotStatus = pollSupportedValuesChange();

    ASSERT_FALSE(status.ok()) << "Status must not be okay";
    ASSERT_EQ(status.error_code(), grpc::StatusCode::UNIMPLEMENTED);
}

TEST_F(GRPCVehicleHardwareUnitTest, TestGetValues) {
    int64_t testRequestId = 1234;
    int32_t testPropId = 4321;
    int32_t testValue = 123456;
    proto::VehiclePropValueRequests gotRequests;
    EXPECT_CALL(*mGrpcStub, GetValues(_, _, _))
            .WillOnce([&gotRequests, testRequestId, testPropId, testValue](
                              ::grpc::ClientContext* context,
                              const proto::VehiclePropValueRequests& request,
                              proto::GetValueResults* response) {
                gotRequests = request;
                response->Clear();
                auto* resultPtr = response->add_results();
                resultPtr->set_request_id(testRequestId);
                resultPtr->set_status(proto::StatusCode::OK);
                auto* valuePtr = resultPtr->mutable_value();
                valuePtr->set_prop(testPropId);
                valuePtr->add_int32_values(testValue);
                return ::grpc::Status::OK;
            });

    std::vector<aidlvhal::GetValueRequest> requests;
    requests.push_back(aidlvhal::GetValueRequest{.requestId = testRequestId,
                                                 .prop = {
                                                         .prop = testPropId,
                                                 }});

    std::vector<aidlvhal::GetValueResult> gotResults;

    auto status = mHardware->getValues(
            std::make_shared<GRPCVehicleHardware::GetValuesCallback>(
                    [&gotResults](std::vector<aidlvhal::GetValueResult> results) {
                        for (const auto& result : results) {
                            gotResults.push_back(result);
                        }
                    }),
            requests);

    ASSERT_EQ(status, aidlvhal::StatusCode::OK);
    ASSERT_THAT(gotRequests.requests(), SizeIs(1));
    EXPECT_THAT(gotRequests.requests(0).request_id(), testRequestId);
    EXPECT_THAT(gotRequests.requests(0).value().prop(), testPropId);

    ASSERT_THAT(gotResults, SizeIs(1));
    EXPECT_EQ(gotResults[0].requestId, testRequestId);
    EXPECT_EQ(gotResults[0].status, aidlvhal::StatusCode::OK);
    EXPECT_EQ(gotResults[0].prop->prop, testPropId);
    EXPECT_THAT(gotResults[0].prop->value.int32Values, ElementsAre(testValue));
}

void GRPCVehicleHardwareUnitTest::generatePropertyUpdateEvent(int32_t propId, int64_t timestamp) {
    // This will be converted to a unique_ptr in StartPropertyValuesStream. The ownership is passed
    // there.
    auto clientReader = new MockClientReader<proto::VehiclePropValues>();
    EXPECT_CALL(*mGrpcStub, StartPropertyValuesStreamRaw(_, _)).WillOnce(Return(clientReader));
    EXPECT_CALL(*clientReader, Read(_))
            .WillOnce([timestamp, propId](proto::VehiclePropValues* values) {
                values->Clear();
                auto value = values->add_values();
                value->set_timestamp(timestamp);
                value->set_prop(propId);
                return true;
            })
            .WillOnce(Return(false));
    EXPECT_CALL(*clientReader, Finish()).WillOnce(Return(::grpc::Status::OK));

    pollValue();
}

TEST_F(GRPCVehicleHardwareUnitTest, TestGetValuesOutdatedRetry) {
    int64_t startTimestamp = elapsedRealtimeNano();
    int64_t testRequestId = 1234;
    int32_t testPropId = 4321;
    int32_t testValue1 = 123456;
    int32_t testValue2 = 654321;
    int32_t testTimestamp1 = 1000;
    int32_t testTimestamp2 = 2000;

    // A property update event for testTimestamp2 happens before getValues returns.
    generatePropertyUpdateEvent(testPropId, testTimestamp2);

    // GetValues first returns an outdated result, then an up-to-date result.
    EXPECT_CALL(*mGrpcStub, GetValues(_, _, _))
            .WillOnce([testRequestId, testPropId, testValue1, testTimestamp1](
                              ::grpc::ClientContext* context,
                              const proto::VehiclePropValueRequests& request,
                              proto::GetValueResults* response) {
                response->Clear();
                auto* resultPtr = response->add_results();
                resultPtr->set_request_id(testRequestId);
                resultPtr->set_status(proto::StatusCode::OK);
                auto* valuePtr = resultPtr->mutable_value();
                valuePtr->set_prop(testPropId);
                valuePtr->set_timestamp(testTimestamp1);
                valuePtr->add_int32_values(testValue1);
                return ::grpc::Status::OK;
            })
            .WillOnce([testRequestId, testPropId, testValue2, testTimestamp2](
                              ::grpc::ClientContext* context,
                              const proto::VehiclePropValueRequests& request,
                              proto::GetValueResults* response) {
                response->Clear();
                auto* resultPtr = response->add_results();
                resultPtr->set_request_id(testRequestId);
                resultPtr->set_status(proto::StatusCode::OK);
                auto* valuePtr = resultPtr->mutable_value();
                valuePtr->set_prop(testPropId);
                valuePtr->set_timestamp(testTimestamp2);
                valuePtr->add_int32_values(testValue2);
                return ::grpc::Status::OK;
            });

    std::vector<aidlvhal::GetValueRequest> requests;
    requests.push_back(aidlvhal::GetValueRequest{.requestId = testRequestId,
                                                 .prop = {
                                                         .prop = testPropId,
                                                 }});

    std::vector<aidlvhal::GetValueResult> gotResults;

    auto status = mHardware->getValues(
            std::make_shared<GRPCVehicleHardware::GetValuesCallback>(
                    [&gotResults](std::vector<aidlvhal::GetValueResult> results) {
                        for (const auto& result : results) {
                            gotResults.push_back(result);
                        }
                    }),
            requests);

    ASSERT_EQ(status, aidlvhal::StatusCode::OK);
    ASSERT_THAT(gotResults, SizeIs(1));
    EXPECT_EQ(gotResults[0].requestId, testRequestId);
    EXPECT_EQ(gotResults[0].status, aidlvhal::StatusCode::OK);
    EXPECT_EQ(gotResults[0].prop->prop, testPropId);
    EXPECT_THAT(gotResults[0].prop->value.int32Values, ElementsAre(testValue2));
    EXPECT_GT(gotResults[0].prop->timestamp, startTimestamp);
    EXPECT_LT(gotResults[0].prop->timestamp, elapsedRealtimeNano());
}

TEST_F(GRPCVehicleHardwareUnitTest, testGetMinMaxSupportedValues) {
    int32_t testPropId = 1234;
    int32_t testAreaId = 4321;
    int32_t testValue1 = 12345;
    int32_t testValue2 = 123456;
    std::vector<PropIdAreaId> propIdAreaIds = {{.propId = testPropId, .areaId = testAreaId}};

    EXPECT_CALL(*mGrpcStub, GetMinMaxSupportedValues(_, _, _))
            .WillOnce([=](::grpc::ClientContext* context,
                          const proto::GetMinMaxSupportedValuesRequest& request,
                          proto::GetMinMaxSupportedValuesResult* response) {
                for (const auto& propIdAreaId : request.prop_id_area_id()) {
                    proto::MinMaxSupportedValueResult* individualResult = response->add_result();
                    individualResult->set_status(proto::StatusCode::OK);
                    individualResult->mutable_min_supported_value()->add_int32_values(testValue1);
                    individualResult->mutable_max_supported_value()->add_int32_values(testValue2);
                }
                return ::grpc::Status::OK;
            });

    auto results = mHardware->getMinMaxSupportedValues(propIdAreaIds);

    ASSERT_THAT(results, ::testing::SizeIs(1));
    EXPECT_EQ(results[0].status, aidlvhal::StatusCode::OK);
    ASSERT_TRUE(results[0].minSupportedValue.has_value());
    EXPECT_EQ(results[0].minSupportedValue.value(),
              aidlvhal::RawPropValues{.int32Values = {testValue1}});
    ASSERT_TRUE(results[0].maxSupportedValue.has_value());
    EXPECT_EQ(results[0].maxSupportedValue.value(),
              aidlvhal::RawPropValues{.int32Values = {testValue2}});
}

TEST_F(GRPCVehicleHardwareUnitTest, testGetMinMaxSupportedValues_noMaxValue) {
    int32_t testPropId = 1234;
    int32_t testAreaId = 4321;
    int32_t testValue1 = 12345;
    std::vector<PropIdAreaId> propIdAreaIds = {{.propId = testPropId, .areaId = testAreaId}};

    EXPECT_CALL(*mGrpcStub, GetMinMaxSupportedValues(_, _, _))
            .WillOnce([=](::grpc::ClientContext* context,
                          const proto::GetMinMaxSupportedValuesRequest& request,
                          proto::GetMinMaxSupportedValuesResult* response) {
                for (const auto& propIdAreaId : request.prop_id_area_id()) {
                    proto::MinMaxSupportedValueResult* individualResult = response->add_result();
                    individualResult->set_status(proto::StatusCode::OK);
                    individualResult->mutable_min_supported_value()->add_int32_values(testValue1);
                }
                return ::grpc::Status::OK;
            });

    auto results = mHardware->getMinMaxSupportedValues(propIdAreaIds);

    ASSERT_THAT(results, ::testing::SizeIs(1));
    EXPECT_EQ(results[0].status, aidlvhal::StatusCode::OK);
    ASSERT_TRUE(results[0].minSupportedValue.has_value());
    EXPECT_EQ(results[0].minSupportedValue.value(),
              aidlvhal::RawPropValues{.int32Values = {testValue1}});
    ASSERT_FALSE(results[0].maxSupportedValue.has_value());
}

TEST_F(GRPCVehicleHardwareUnitTest, testGetSupportedValuesLists) {
    int32_t testPropId = 1234;
    int32_t testAreaId = 4321;
    int32_t testValue1 = 12345;
    int32_t testValue2 = 123456;
    std::vector<PropIdAreaId> propIdAreaIds = {{.propId = testPropId, .areaId = testAreaId}};

    EXPECT_CALL(*mGrpcStub, GetSupportedValuesLists(_, _, _))
            .WillOnce([=](::grpc::ClientContext* context,
                          const proto::GetSupportedValuesListsRequest& request,
                          proto::GetSupportedValuesListsResult* response) {
                for (const auto& propIdAreaId : request.prop_id_area_id()) {
                    proto::SupportedValuesListResult* individualResult = response->add_result();
                    individualResult->set_status(proto::StatusCode::OK);
                    individualResult->add_supported_values_list()->add_int32_values(testValue1);
                    individualResult->add_supported_values_list()->add_int32_values(testValue2);
                }
                return ::grpc::Status::OK;
            });

    auto results = mHardware->getSupportedValuesLists(propIdAreaIds);

    ASSERT_THAT(results, ::testing::SizeIs(1));
    EXPECT_EQ(results[0].status, aidlvhal::StatusCode::OK);
    ASSERT_TRUE(results[0].supportedValuesList.has_value());
    ASSERT_THAT(results[0].supportedValuesList.value(), ::testing::SizeIs(2));
    EXPECT_EQ(results[0].supportedValuesList.value()[0],
              aidlvhal::RawPropValues{.int32Values = {testValue1}});
    EXPECT_EQ(results[0].supportedValuesList.value()[1],
              aidlvhal::RawPropValues{.int32Values = {testValue2}});
}

TEST_F(GRPCVehicleHardwareUnitTest, testGetSupportedValuesLists_noSupportedValue) {
    int32_t testPropId = 1234;
    int32_t testAreaId = 4321;
    std::vector<PropIdAreaId> propIdAreaIds = {{.propId = testPropId, .areaId = testAreaId}};

    EXPECT_CALL(*mGrpcStub, GetSupportedValuesLists(_, _, _))
            .WillOnce([=](::grpc::ClientContext* context,
                          const proto::GetSupportedValuesListsRequest& request,
                          proto::GetSupportedValuesListsResult* response) {
                for (const auto& propIdAreaId : request.prop_id_area_id()) {
                    proto::SupportedValuesListResult* individualResult = response->add_result();
                    individualResult->set_status(proto::StatusCode::INTERNAL_ERROR);
                }
                return ::grpc::Status::OK;
            });

    auto results = mHardware->getSupportedValuesLists(propIdAreaIds);

    ASSERT_THAT(results, ::testing::SizeIs(1));
    EXPECT_EQ(results[0].status, aidlvhal::StatusCode::INTERNAL_ERROR);
    ASSERT_FALSE(results[0].supportedValuesList.has_value());
}

}  // namespace android::hardware::automotive::vehicle::virtualization
