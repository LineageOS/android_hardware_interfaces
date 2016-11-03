
#define LOG_TAG "hidl_test"

#include "Fetcher.h"
#include <android-base/logging.h>
#include <inttypes.h>

namespace android {
namespace hardware {
namespace tests {
namespace inheritance {
namespace V1_0 {
namespace implementation {

Fetcher::Fetcher() {
    mPrecious = IChild::getService("local child", true);
    CHECK(!mPrecious->isRemote());
}

template <typename CB>
Return<void> selectService(bool sendRemote, CB &_hidl_cb, sp<IChild> &local) {
    sp<IChild> toSend;
    if (sendRemote) {
        toSend = IChild::getService("child");
        if (!toSend->isRemote()) {
            toSend = nullptr;
        }
    } else {
        toSend = local;
    }
    ALOGI("SERVER(Fetcher) selectService returning %p", toSend.get());
    _hidl_cb(toSend);
    return Void();
}

// Methods from ::android::hardware::tests::inheritance::V1_0::IFetcher follow.
Return<void> Fetcher::getGrandparent(bool sendRemote, getGrandparent_cb _hidl_cb)  {
    return selectService(sendRemote, _hidl_cb, mPrecious);
}

Return<void> Fetcher::getParent(bool sendRemote, getParent_cb _hidl_cb)  {
    return selectService(sendRemote, _hidl_cb, mPrecious);
}

Return<void> Fetcher::getChild(bool sendRemote, getChild_cb _hidl_cb)  {
    return selectService(sendRemote, _hidl_cb, mPrecious);
}

IFetcher* HIDL_FETCH_IFetcher(const char* /* name */) {
    return new Fetcher();
}

} // namespace implementation
}  // namespace V1_0
}  // namespace inheritance
}  // namespace tests
}  // namespace hardware
}  // namespace android
