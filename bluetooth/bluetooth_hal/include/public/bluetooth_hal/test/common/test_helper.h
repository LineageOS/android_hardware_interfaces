/*
 * Copyright 2024 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

#include "bluetooth_hal/hal_packet.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace bluetooth_hal {
namespace util {

class MockPacketHandler {
 public:
  MockPacketHandler() = default;
  ~MockPacketHandler() = default;

  MockPacketHandler(const MockPacketHandler&) = delete;
  MockPacketHandler& operator=(const MockPacketHandler&) = delete;

  MOCK_METHOD(void, HalPacketCallback,
              (const ::bluetooth_hal::hci::HalPacket& packet));

  MOCK_METHOD(void, PacketReadyCb, ());
};

template <typename ExpectedType, typename ActualType>
class BaseMatcher : public ::testing::MatcherInterface<ActualType> {
 public:
  /**
   * @brief Constructor for BaseMatcher.
   *
   * @param expected_content The expected value to be compared against.
   * @param expected_size Optional size of the expected content (for byte
   * comparison).
   *
   */
  explicit BaseMatcher(ExpectedType expected_content,
                       std::optional<size_t> expected_size = std::nullopt)
      : expected_content_(expected_content), expected_size_(expected_size) {}

  /**
   * @brief Checks if the actual value matches the expected value and explains
   * the result.
   *
   * This pure virtual function must be implemented by derived classes to define
   * the specific matching logic.
   *
   * @param actual_value The actual value being compared.
   * @param listener The listener to receive match result explanation.
   *
   * @return True if the actual value matches the expected value, false
   * otherwise.
   *
   */
  virtual bool MatchAndExplain(
      ActualType actual_value,
      ::testing::MatchResultListener* listener) const = 0;

  /**
   * @brief Describes the matcher (what it expects) to an output stream.
   *
   * This function is used to describe what the matcher expects in case of a
   * mismatch.
   *
   * @param os The output stream to write the description to.
   *
   */
  void DescribeTo([[maybe_unused]] std::ostream* os) const override {}

  /**
   * @brief Describes the negation of the matcher to an output stream.
   *
   * This function is used to describe what the matcher does not expect in case
   * of a match.
   *
   * @param os The output stream to write the negation description to.
   *
   */
  void DescribeNegationTo([[maybe_unused]] std::ostream* os) const override {}

 protected:
  ExpectedType expected_content_;
  std::optional<size_t> expected_size_;
};

/**
 * @brief A matcher class for comparing string values.
 *
 * This matcher compares two values as null-terminated C-style strings.
 *
 * @tparam ExpectedType The type of the expected value (typically const char*).
 * @tparam ActualType The type of the actual value being compared (typically
 * const char*).
 *
 */
template <typename ExpectedType, typename ActualType>
class StringMatcher : public BaseMatcher<ExpectedType, ActualType> {
 public:
  /**
   * @brief Constructor for StringMatcher.
   *
   * @param expected_content The expected string value.
   *
   */
  explicit StringMatcher(ExpectedType expected_content)
      : BaseMatcher<ExpectedType, ActualType>(expected_content) {}

  /**
   * @brief Checks if the actual string matches the expected string and explains
   * the result.
   *
   * @param actual_content The actual string value being compared.
   * @param listener The listener to receive match result explanation (unused in
   * this implementation).
   *
   * @return True if the actual string matches the expected string, false
   * otherwise.
   *
   */
  bool MatchAndExplain(ActualType actual_content,
                       [[maybe_unused]] ::testing::MatchResultListener*
                           listener) const override {
    return std::string(this->expected_content_) == std::string(actual_content);
  }
};

/**
 * @brief A matcher class for comparing byte content.
 *
 * This matcher compares two byte arrays for equality.
 *
 * @tparam ExpectedType The type of the expected value (typically const
 * uint8_t*).
 * @tparam ActualType The type of the actual value being compared (typically
 * const void*).
 *
 */
template <typename ExpectedType, typename ActualType>
class ByteContentMatcher : public BaseMatcher<ExpectedType, ActualType> {
 public:
  /**
   * @brief Constructor for ByteContentMatcher.
   *
   * @param expected_content The expected byte content.
   * @param expected_size The size of the expected byte content.
   *
   */
  ByteContentMatcher(ExpectedType expected_content, size_t expected_size)
      : BaseMatcher<ExpectedType, ActualType>(expected_content, expected_size) {
  }

  /**
   * @brief Checks if the actual byte content matches the expected byte content
   * and explains the result.
   *
   * @param actual_content The actual byte content being compared.
   * @param listener The listener to receive match result explanation (unused in
   * this implementation).
   *
   * @return True if the actual byte content matches the expected byte content,
   * false otherwise.
   *
   */
  bool MatchAndExplain(ActualType actual_content,
                       [[maybe_unused]] ::testing::MatchResultListener*
                           listener) const override {
    const auto* actual_byte = reinterpret_cast<const uint8_t*>(actual_content);
    return std::equal(actual_byte, actual_byte + this->expected_size_.value(),
                      this->expected_content_);
  }
};

/**
 * @brief A matcher class for comparing vector values.
 *
 * This matcher compares two vectors for equality.
 *
 * @tparam ExpectedType The type of the expected value (typically
 * std::vector<T>).
 * @tparam ActualType The type of the actual value being compared (typically
 * std::vector<T>).
 *
 */
template <typename ExpectedType, typename ActualType>
class VectorMatcher : public BaseMatcher<ExpectedType, ActualType> {
 public:
  /**
   * @brief Constructor for VectorMatcher.
   *
   * @param expected_content The expected vector value.
   *
   */
  explicit VectorMatcher(ExpectedType expected_content)
      : BaseMatcher<ExpectedType, ActualType>(expected_content) {}

  /**
   * @brief Checks if the actual vector matches the expected vector and explains
   * the result.
   *
   * @param actual_content The actual vector value being compared.
   * @param listener The listener to receive match result explanation (unused in
   * this implementation).
   *
   * @return True if the actual vector matches the expected vector, false
   * otherwise.
   *
   */
  bool MatchAndExplain(ActualType actual_content,
                       [[maybe_unused]] ::testing::MatchResultListener*
                           listener) const override {
    return this->expected_content_ == actual_content;
  }
};

/**
 * @brief A factory class for creating various matcher objects.
 *
 * This class provides static factory methods for creating different types of
 * matchers.
 */
class MatcherFactory {
 public:
  /**
   * @brief Creates a StringMatcher for comparing string values.
   *
   * @tparam ExpectedType The type of the expected value (typically const
   * char*).
   * @tparam ActualType The type of the actual value being compared (defaults to
   * const char*).
   *
   * @param expected_content The expected string value.
   *
   * @return A ::testing::Matcher object that can be used to compare string
   * values.
   *
   */
  template <typename ExpectedType, typename ActualType = const char*>
  static ::testing::Matcher<ActualType> CreateStringMatcher(
      ExpectedType expected_content) {
    return ::testing::MakeMatcher(
        new StringMatcher<ExpectedType, ActualType>(expected_content));
  }

  /**
   * @brief Creates a ByteContentMatcher for comparing byte content.
   *
   * @tparam ExpectedType The type of the expected value (typically const
   * uint8_t*).
   * @tparam ActualType The type of the actual value being compared (defaults to
   * const void*).
   *
   * @param expected_content The expected byte content.
   * @param expected_size The size of the expected byte content.
   *
   * @return A ::testing::Matcher object that can be used to compare byte
   * content.
   *
   */
  template <typename ExpectedType, typename ActualType = const void*>
  static ::testing::Matcher<ActualType> CreateByteContentMatcher(
      ExpectedType expected_content, size_t expected_size) {
    return ::testing::MakeMatcher(
        new ByteContentMatcher<ExpectedType, ActualType>(expected_content,
                                                         expected_size));
  }

  /**
   * @brief Creates a VectorMatcher for comparing vector values.
   *
   * @tparam ExpectedType The type of the expected value (typically
   * std::vector<T>).
   * @tparam ActualType The type of the actual value being compared (defaults to
   * const std::vector<uint8_t>&).
   *
   * @param expected_content The expected vector value.
   *
   * @return A ::testing::Matcher object that can be used to compare vector
   * values.
   *
   */
  template <typename ExpectedType,
            typename ActualType = const std::vector<uint8_t>&>
  static ::testing::Matcher<ActualType> CreateVectorMatcher(
      ExpectedType expected_content) {
    return ::testing::MakeMatcher(
        new VectorMatcher<ExpectedType, ActualType>(expected_content));
  }

  /**
   * @brief Creates a VectorMatcher for comparing HalPacket values.
   *
   * @tparam ExpectedType The type of the expected value (typically
   * std::vector<T>).
   * @tparam ActualType The type of the actual value being compared (defaults to
   * const ::bluetooth_hal::hci::HalPacket&).
   *
   * @param expected_content The expected HalPacket value.
   *
   * @return A ::testing::Matcher object that can be used to compare HalPacket
   * values.
   *
   */
  template <typename ExpectedType,
            typename ActualType = const ::bluetooth_hal::hci::HalPacket&>
  static ::testing::Matcher<ActualType> CreateHalPacketMatcher(
      ExpectedType expected_content) {
    return ::testing::MakeMatcher(
        new VectorMatcher<ExpectedType, ActualType>(expected_content));
  }
};

}  // namespace util
}  // namespace bluetooth_hal
