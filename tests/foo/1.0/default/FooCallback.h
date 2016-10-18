#ifndef HIDL_GENERATED_android_hardware_tests_foo_V1_0_FooCallback_H_
#define HIDL_GENERATED_android_hardware_tests_foo_V1_0_FooCallback_H_

#include <android/hardware/tests/foo/1.0/IFooCallback.h>
#include <hidl/Status.h>
#include <hidl/MQDescriptor.h>

#include <utils/Condition.h>
#include <utils/Timers.h>
namespace android {
namespace hardware {
namespace tests {
namespace foo {
namespace V1_0 {
namespace implementation {

using ::android::hardware::tests::foo::V1_0::IFooCallback;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_vec;
using ::android::hardware::hidl_string;
using ::android::sp;

struct FooCallback : public IFooCallback {
    FooCallback() : mLock{}, mCond{} {}
    // Methods from ::android::hardware::tests::foo::V1_0::IFooCallback follow.
    Return<void> heyItsYou(const sp<IFooCallback>& cb)  override;
    Return<bool> heyItsYouIsntIt(const sp<IFooCallback>& cb)  override;
    Return<void> heyItsTheMeaningOfLife(uint8_t tmol)  override;
    Return<void> reportResults(int64_t ns, reportResults_cb _hidl_cb)  override;
    Return<void> youBlockedMeFor(const hidl_array<int64_t, 3 /* 3 */>& callerBlockedInfo)  override;

    static constexpr nsecs_t DELAY_S = 1;
    static constexpr nsecs_t DELAY_NS = seconds_to_nanoseconds(DELAY_S);
    static constexpr nsecs_t TOLERANCE_NS = milliseconds_to_nanoseconds(10);
    static constexpr nsecs_t ONEWAY_TOLERANCE_NS = milliseconds_to_nanoseconds(1);

    hidl_array<InvokeInfo, 3> invokeInfo;
    Mutex mLock;
    Condition mCond;
};

extern "C" IFooCallback* HIDL_FETCH_IFooCallback(const char* name);

}  // namespace implementation
}  // namespace V1_0
}  // namespace foo
}  // namespace tests
}  // namespace hardware
}  // namespace android

#endif  // HIDL_GENERATED_android_hardware_tests_foo_V1_0_FooCallback_H_
