#include "FooCallback.h"
#include <android-base/logging.h>
#include <inttypes.h>

namespace android {
namespace hardware {
namespace tests {
namespace foo {
namespace V1_0 {
namespace implementation {

Return<void> FooCallback::heyItsYou(
        const sp<IFooCallback> &_cb) {
    nsecs_t start = systemTime();
    ALOGI("SERVER(FooCallback) heyItsYou cb = %p", _cb.get());
    mLock.lock();
    invokeInfo[0].invoked = true;
    invokeInfo[0].timeNs = systemTime() - start;
    mCond.signal();
    mLock.unlock();
    return Void();
}

Return<bool> FooCallback::heyItsYouIsntIt(const sp<IFooCallback> &_cb) {
    nsecs_t start = systemTime();
    ALOGI("SERVER(FooCallback) heyItsYouIsntIt cb = %p sleeping for %" PRId64 " seconds", _cb.get(), DELAY_S);
    sleep(DELAY_S);
    ALOGI("SERVER(FooCallback) heyItsYouIsntIt cb = %p responding", _cb.get());
    mLock.lock();
    invokeInfo[1].invoked = true;
    invokeInfo[1].timeNs = systemTime() - start;
    mCond.signal();
    mLock.unlock();
    return true;
}

Return<void> FooCallback::heyItsTheMeaningOfLife(uint8_t tmol) {
    nsecs_t start = systemTime();
    ALOGI("SERVER(FooCallback) heyItsTheMeaningOfLife = %d sleeping for %" PRId64 " seconds", tmol, DELAY_S);
    sleep(DELAY_S);
    ALOGI("SERVER(FooCallback) heyItsTheMeaningOfLife = %d done sleeping", tmol);
    mLock.lock();
    invokeInfo[2].invoked = true;
    invokeInfo[2].timeNs = systemTime() - start;
    mCond.signal();
    mLock.unlock();
    return Void();
}

Return<void> FooCallback::reportResults(int64_t ns, reportResults_cb cb) {
    ALOGI("SERVER(FooCallback) reportResults(%" PRId64 " seconds)", nanoseconds_to_seconds(ns));
    nsecs_t leftToWaitNs = ns;
    mLock.lock();
    while (!(invokeInfo[0].invoked && invokeInfo[1].invoked && invokeInfo[2].invoked) &&
           leftToWaitNs > 0) {
      nsecs_t start = systemTime();
      ::android::status_t rc = mCond.waitRelative(mLock, leftToWaitNs);
      if (rc != ::android::OK) {
          ALOGI("SERVER(FooCallback)::reportResults(%" PRId64 " ns) Condition::waitRelative(%" PRId64 ") returned error (%d)", ns, leftToWaitNs, rc);
          break;
      }
      ALOGI("SERVER(FooCallback)::reportResults(%" PRId64 " ns) Condition::waitRelative was signalled", ns);
      leftToWaitNs -= systemTime() - start;
    }
    mLock.unlock();
    cb(leftToWaitNs, invokeInfo);
    return Void();
}

Return<void> FooCallback::youBlockedMeFor(const hidl_array<int64_t, 3> &ns) {
    for (size_t i = 0; i < 3; i++) {
        invokeInfo[i].callerBlockedNs = ns[i];
    }
    return Void();
}

IFooCallback* HIDL_FETCH_IFooCallback(const char* /* name */) {
    return new FooCallback();
}

} // namespace implementation
}  // namespace V1_0
}  // namespace foo
}  // namespace tests
}  // namespace hardware
}  // namespace android
