
#define LOG_TAG "hidl_test"

#include "Foo.h"
#include "FooCallback.h"
#include <android-base/logging.h>
#include <hidl-test/FooHelper.h>
#include <inttypes.h>
#include <utils/Timers.h>

namespace android {
namespace hardware {
namespace tests {
namespace foo {
namespace V1_0 {
namespace implementation {

// Methods from ::android::hardware::tests::foo::V1_0::IFoo follow.
Return<void> Foo::doThis(float param) {
    ALOGI("SERVER(Foo) doThis(%.2f)", param);

    return Void();
}

Return<int32_t> Foo::doThatAndReturnSomething(
        int64_t param) {
    LOG(INFO) << "SERVER(Foo) doThatAndReturnSomething(" << param << ")";

    return 666;
}

Return<double> Foo::doQuiteABit(
        int32_t a,
        int64_t b,
        float c,
        double d) {
    LOG(INFO) << "SERVER(Foo) doQuiteABit("
              << a
              << ", "
              << b
              << ", "
              << c
              << ", "
              << d
              << ")";

    return 666.5;
}

Return<void> Foo::doSomethingElse(
        const hidl_array<int32_t, 15> &param, doSomethingElse_cb _cb) {
    ALOGI("SERVER(Foo) doSomethingElse(...)");

    hidl_array<int32_t, 32> result;
    for (size_t i = 0; i < 15; ++i) {
        result[i] = 2 * param[i];
        result[15 + i] = param[i];
    }
    result[30] = 1;
    result[31] = 2;

    _cb(result);

    return Void();
}

Return<void> Foo::doStuffAndReturnAString(
        doStuffAndReturnAString_cb _cb) {
    ALOGI("SERVER(Foo) doStuffAndReturnAString");

    _cb("Hello, world");

    return Void();
}

Return<void> Foo::mapThisVector(
        const hidl_vec<int32_t> &param, mapThisVector_cb _cb) {
    ALOGI("SERVER(Foo) mapThisVector");

    hidl_vec<int32_t> out;
    out.resize(param.size());

    for (size_t i = 0; i < out.size(); ++i) {
        out[i] = param[i] * 2;
    }

    _cb(out);

    return Void();
}

Return<void> Foo::callMe(
        const sp<IFooCallback> &cb) {
    ALOGI("SERVER(Foo) callMe %p", cb.get());

    if (cb != NULL) {

        hidl_array<nsecs_t, 3> c;
        ALOGI("SERVER(Foo) callMe %p calling IFooCallback::heyItsYou, " \
              "should return immediately", cb.get());
        c[0] = systemTime();
        cb->heyItsYou(cb);
        c[0] = systemTime() - c[0];
        ALOGI("SERVER(Foo) callMe %p calling IFooCallback::heyItsYou " \
              "returned after %" PRId64 "ns", cb.get(), c[0]);

        ALOGI("SERVER(Foo) callMe %p calling IFooCallback::heyItsYouIsntIt, " \
              "should block for %" PRId64 " seconds", cb.get(),
              DELAY_S);
        c[1] = systemTime();
        bool answer = cb->heyItsYouIsntIt(cb);
        c[1] = systemTime() - c[1];
        ALOGI("SERVER(Foo) callMe %p IFooCallback::heyItsYouIsntIt " \
              "responded with %d after %" PRId64 "ns", cb.get(), answer, c[1]);

        ALOGI("SERVER(Foo) callMe %p calling " \
              "IFooCallback::heyItsTheMeaningOfLife, " \
              "should return immediately ", cb.get());
        c[2] = systemTime();
        cb->heyItsTheMeaningOfLife(42);
        c[2] = systemTime() - c[2];
        ALOGI("SERVER(Foo) callMe %p After call to " \
              "IFooCallback::heyItsTheMeaningOfLife " \
              "responded after %" PRId64 "ns", cb.get(), c[2]);

        ALOGI("SERVER(Foo) callMe %p calling IFooCallback::youBlockedMeFor " \
              "to report times", cb.get());
        cb->youBlockedMeFor(c);
        ALOGI("SERVER(Foo) callMe %p After call to " \
              "IFooCallback::heyYouBlockedMeFor", cb.get());
    }

    return Void();
}

Return<Foo::SomeEnum> Foo::useAnEnum(SomeEnum param) {
    ALOGI("SERVER(Foo) useAnEnum %d", (int)param);

    return SomeEnum::goober;
}

Return<void> Foo::haveAGooberVec(const hidl_vec<Goober>& param) {
    ALOGI("SERVER(Foo) haveAGooberVec &param = %p", &param);

    return Void();
}

Return<void> Foo::haveAGoober(const Goober &g) {
    ALOGI("SERVER(Foo) haveaGoober g=%p", &g);

    return Void();
}

Return<void> Foo::haveAGooberArray(const hidl_array<Goober, 20> & /* lots */) {
    ALOGI("SERVER(Foo) haveAGooberArray");

    return Void();
}

Return<void> Foo::haveATypeFromAnotherFile(const Abc &def) {
    ALOGI("SERVER(Foo) haveATypeFromAnotherFile def=%p", &def);

    return Void();
}

Return<void> Foo::haveSomeStrings(
        const hidl_array<hidl_string, 3> &array,
        haveSomeStrings_cb _cb) {
    ALOGI("SERVER(Foo) haveSomeStrings([\"%s\", \"%s\", \"%s\"])",
          array[0].c_str(),
          array[1].c_str(),
          array[2].c_str());

    hidl_array<hidl_string, 2> result;
    result[0] = "Hello";
    result[1] = "World";

    _cb(result);

    return Void();
}

Return<void> Foo::haveAStringVec(
        const hidl_vec<hidl_string> &vector,
        haveAStringVec_cb _cb) {
    ALOGI("SERVER(Foo) haveAStringVec([\"%s\", \"%s\", \"%s\"])",
          vector[0].c_str(),
          vector[1].c_str(),
          vector[2].c_str());

    hidl_vec<hidl_string> result;
    result.resize(2);

    result[0] = "Hello";
    result[1] = "World";

    _cb(result);

    return Void();
}

Return<void> Foo::transposeMe(
        const hidl_array<float, 3, 5> &in, transposeMe_cb _cb) {
    ALOGI("SERVER(Foo) transposeMe(%s)", to_string(in).c_str());

    hidl_array<float, 5, 3> out;
    for (size_t i = 0; i < 5; ++i) {
        for (size_t j = 0; j < 3; ++j) {
            out[i][j] = in[j][i];
        }
    }

    ALOGI("SERVER(Foo) transposeMe returning %s", to_string(out).c_str());

    _cb(out);

    return Void();
}

Return<void> Foo::callingDrWho(
        const MultiDimensional &in, callingDrWho_cb _hidl_cb) {
    ALOGI("SERVER(Foo) callingDrWho(%s)", MultiDimensionalToString(in).c_str());

    MultiDimensional out;
    for (size_t i = 0; i < 5; ++i) {
        for (size_t j = 0; j < 3; ++j) {
            out.quuxMatrix[i][j].first = in.quuxMatrix[4 - i][2 - j].last;
            out.quuxMatrix[i][j].last = in.quuxMatrix[4 - i][2 - j].first;
        }
    }

    _hidl_cb(out);

    return Void();
}

Return<void> Foo::transpose(const StringMatrix5x3 &in, transpose_cb _hidl_cb) {
    LOG(INFO) << "SERVER(Foo) transpose " << to_string(in);

    StringMatrix3x5 out;
    for (size_t i = 0; i < 3; ++i) {
        for (size_t j = 0; j < 5; ++j) {
            out.s[i][j] = in.s[j][i];
        }
    }

    _hidl_cb(out);

    return Void();
}

Return<void> Foo::transpose2(
        const hidl_array<hidl_string, 5, 3> &in, transpose2_cb _hidl_cb) {
    LOG(INFO) << "SERVER(Foo) transpose2 " << to_string(in);

    hidl_array<hidl_string, 3, 5> out;
    for (size_t i = 0; i < 3; ++i) {
        for (size_t j = 0; j < 5; ++j) {
            out[i][j] = in[j][i];
        }
    }

    _hidl_cb(out);

    return Void();
}

Return<void> Foo::sendVec(
        const hidl_vec<uint8_t> &data, sendVec_cb _hidl_cb) {
    _hidl_cb(data);

    return Void();
}

Return<void> Foo::sendVecVec(sendVecVec_cb _hidl_cb) {
    hidl_vec<hidl_vec<uint8_t>> data;
    _hidl_cb(data);

    return Void();
}

Return<void> Foo::haveAVectorOfInterfaces(
        const hidl_vec<sp<ISimple> > &in,
        haveAVectorOfInterfaces_cb _hidl_cb) {
    _hidl_cb(in);

    return Void();
}

Return<void> Foo::haveAVectorOfGenericInterfaces(
        const hidl_vec<sp<android::hardware::IBinder> > &in,
        haveAVectorOfGenericInterfaces_cb _hidl_cb) {
    _hidl_cb(in);
    return Void();
}

Return<void> Foo::createMyHandle(createMyHandle_cb _hidl_cb) {
    native_handle_t* nh = native_handle_create(0, 10);
    int data[] = {2,3,5,7,11,13,17,19,21,23};
    CHECK(sizeof(data) == 10 * sizeof(int));
    memcpy(nh->data, data, sizeof(data));
    mHandles.push_back(nh);

    MyHandle h;
    h.guard = 666;
    h.h = nh;
    _hidl_cb(h);
    return Void();
}

Return<void> Foo::createHandles(uint32_t size, createHandles_cb _hidl_cb) {
    hidl_vec<const native_handle_t*> handles;
    handles.resize(size);
    for(uint32_t i = 0; i < size; ++i) {
        createMyHandle([&](const MyHandle& h) {
            handles[i] = h.h;
        });
    }
    _hidl_cb(handles);
    return Void();
}

Return<void> Foo::closeHandles() {
    for(native_handle_t* h : mHandles) {
        native_handle_delete(h);
    }
    mHandles.clear();
    return Void();
}

IFoo* HIDL_FETCH_IFoo(const char* /* name */) {
    return new Foo();
}

} // namespace implementation
}  // namespace V1_0
}  // namespace foo
}  // namespace tests
}  // namespace hardware
}  // namespace android
