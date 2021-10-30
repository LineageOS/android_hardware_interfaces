#pragma once

#include <chrono>

#include <gtest/gtest.h>

namespace android::hardware::health::test_utils {

using testing::AssertionFailure;
using testing::AssertionResult;
using testing::AssertionSuccess;
using std::chrono_literals::operator""s;

// Needs to be called repeatedly within a period of time to ensure values are initialized.
template <typename BatteryStatusType, typename BatteryStatusToStringFn>
inline AssertionResult IsBatteryCurrentSignCorrect(const BatteryStatusType& status, int32_t current,
                                                   bool acceptZeroCurrentAsUnknown,
                                                   const BatteryStatusToStringFn& toString) {
    // For IHealth.getCurrentNow/Average, if current is not available, it is expected that
    // the error code is NOT_SUPPORTED, which is checked above. Hence, zero current is
    // not treated as unknown values.
    // For IHealth.getHealthInfo, if current is not available, health_info.current_* == 0.
    // Caller of this function provides current.result == Result::SUCCESS. Hence, just skip the
    // check.
    if (current == 0 && acceptZeroCurrentAsUnknown) {
        return AssertionSuccess()
               << "current is 0, which indicates the value may not be available. Skipping.";
    }

    switch (status) {
        case BatteryStatusType::UNKNOWN:
            if (current != 0) {
                // BatteryStatus may be UNKNOWN initially with a non-zero current value, but
                // after it is initialized, it should be known.
                return AssertionFailure()
                       << "BatteryStatus is UNKNOWN but current is not 0. Actual: " << current;
            }
            break;
        case BatteryStatusType::CHARGING:
            if (current <= 0) {
                return AssertionFailure()
                       << "BatteryStatus is CHARGING but current is not positive. Actual: "
                       << current;
            }
            break;
        case BatteryStatusType::NOT_CHARGING:
            if (current > 0) {
                return AssertionFailure() << "BatteryStatus is " << toString(status)
                                          << " but current is positive. Actual: " << current;
            }
            break;
        case BatteryStatusType::DISCHARGING:
            if (current >= 0) {
                return AssertionFailure() << "BatteryStatus is " << toString(status)
                                          << " but current is not negative. Actual: " << current;
            }
            break;
        case BatteryStatusType::FULL:
            // Battery current may be positive or negative depending on the load.
            break;
        default:
            return AssertionFailure() << "Unknown BatteryStatus " << toString(status);
    }

    return AssertionSuccess() << "BatteryStatus is " << toString(status)
                              << " and current has the correct sign: " << current;
}

inline AssertionResult IsValueSimilar(int32_t dividend, int32_t divisor, double factor) {
    auto difference = abs(dividend - divisor);
    if (difference > factor * abs(divisor)) {
        return AssertionFailure() << dividend << " and " << divisor
                                  << " are not similar (factor = " << factor << ")";
    }
    return AssertionSuccess() << dividend << " and " << divisor
                              << " are similar (factor = " << factor << ")";
}

inline AssertionResult IsBatteryCurrentSimilar(int32_t currentNow, int32_t currentAverage,
                                               double currentCompareFactor) {
    // Check that the two values are similar. Note that the two tests uses a different
    // divisor to ensure that they are actually pretty similar. For example,
    // IsValueSimilar(5,10,0.4) returns true, but IsValueSimlar(10,5,0.4) returns false.
    auto res = IsValueSimilar(currentNow, currentAverage, currentCompareFactor)
               << " for now vs. average. Check units.";
    if (!res) return res;
    res = IsValueSimilar(currentAverage, currentNow, currentCompareFactor)
          << " for average vs. now. Check units.";
    if (!res) return res;
    return AssertionSuccess() << "currentNow = " << currentNow
                              << " and currentAverage = " << currentAverage
                              << " are considered similar.";
}

// Test that f() returns AssertionSuccess() once in a given period of time.
template <typename Duration, typename Function>
inline AssertionResult SucceedOnce(Duration d, Function f) {
    AssertionResult result = AssertionFailure() << "Function is never evaluated.";
    auto end = std::chrono::system_clock::now() + d;
    while (std::chrono::system_clock::now() <= end) {
        result = f();
        if (result) {
            return result;
        }
        std::this_thread::sleep_for(2s);
    }
    return result;
}

template <typename BatteryStatusType, typename BatteryInfoType, typename BatteryStatusToStringFn>
AssertionResult IsBatteryStatusCorrect(const BatteryStatusType& status,
                                       const BatteryInfoType& batteryInfo,
                                       const BatteryStatusToStringFn& toString) {
    bool isConnected = batteryInfo.chargerAcOnline || batteryInfo.chargerUsbOnline ||
                       batteryInfo.chargerWirelessOnline;

    std::stringstream message;
    message << "BatteryStatus is " << toString(status) << " and " << (isConnected ? "" : "no ")
            << "power source is connected: ac=" << batteryInfo.chargerAcOnline
            << ", usb=" << batteryInfo.chargerUsbOnline
            << ", wireless=" << batteryInfo.chargerWirelessOnline;

    switch (status) {
        case BatteryStatusType::UNKNOWN: {
            // Don't enforce anything on isConnected on unknown battery status.
            // Battery-less devices must report UNKNOWN battery status, but may report true
            // or false on isConnected.
        } break;
        case BatteryStatusType::CHARGING:
        case BatteryStatusType::NOT_CHARGING:
        case BatteryStatusType::FULL: {
            if (!isConnected) {
                return AssertionFailure() << message.str();
            }
        } break;
        case BatteryStatusType::DISCHARGING: {
            if (isConnected) {
                return AssertionFailure() << message.str();
            }
        } break;
        default: {
            return AssertionFailure() << "Unknown battery status value " << toString(status);
        } break;
    }

    return AssertionSuccess() << message.str();
}

}  // namespace android::hardware::health::test_utils
