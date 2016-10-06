
#define LOG_TAG "hidl_test"

#include "Foo.h"
#include "FooCallback.h"
#include <android-base/logging.h>
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

Return<void> Foo::doThis(uint32_t param) {
    ALOGI("SERVER(Foo) doThis (int) (%d)", param);
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

    hidl_string s;
    s = "Hello, world";

    _cb(s);

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
              FooCallback::DELAY_S);
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

// NOTE: duplicated code in hidl_test
using std::to_string;

static std::string to_string(const IFoo::StringMatrix5x3 &M);
static std::string to_string(const IFoo::StringMatrix3x5 &M);
static std::string to_string(const hidl_string &s);

template<typename T>
static std::string to_string(const T *elems, size_t n) {
    std::string out;
    out = "[";
    for (size_t i = 0; i < n; ++i) {
        if (i > 0) {
            out += ", ";
        }
        out += to_string(elems[i]);
    }
    out += "]";

    return out;
}

template<typename T, size_t SIZE>
static std::string to_string(const hidl_array<T, SIZE> &array) {
    return to_string(&array[0], SIZE);
}

template<typename T, size_t SIZE1, size_t SIZE2>
static std::string to_string(const hidl_array<T, SIZE1, SIZE2> &array) {
    std::string out;
    out = "[";
    for (size_t i = 0; i < SIZE1; ++i) {
        if (i > 0) {
            out += ", ";
        }

        out += "[";
        for (size_t j = 0; j < SIZE2; ++j) {
            if (j > 0) {
                out += ", ";
            }

            out += to_string(array[i][j]);
        }
        out += "]";
    }
    out += "]";

    return out;
}

template<typename T>
static std::string to_string(const hidl_vec<T> &vec) {
    return to_string(&vec[0], vec.size());
}

static std::string to_string(const IFoo::StringMatrix5x3 &M) {
    return to_string(M.s);
}

static std::string to_string(const IFoo::StringMatrix3x5 &M) {
    return to_string(M.s);
}

static std::string to_string(const hidl_string &s) {
    return std::string("'") + s.c_str() + "'";
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
// end duplicated code

static std::string QuuxToString(const IFoo::Quux &val) {
    std::string s;

    s = "Quux(first='";
    s += val.first.c_str();
    s += "', last='";
    s += val.last.c_str();
    s += "')";

    return s;
}

static std::string MultiDimensionalToString(const IFoo::MultiDimensional &val) {
    std::string s;

    s += "MultiDimensional(";

    s += "quuxMatrix=[";

    size_t k = 0;
    for (size_t i = 0; i < 5; ++i) {
        if (i > 0) {
            s += ", ";
        }

        s += "[";
        for (size_t j = 0; j < 3; ++j, ++k) {
            if (j > 0) {
                s += ", ";
            }

            s += QuuxToString(val.quuxMatrix[i][j]);
        }
    }
    s += "]";

    s += ")";

    return s;
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


IFoo* HIDL_FETCH_IFoo(const char* /* name */) {
    return new Foo();
}

} // namespace implementation
}  // namespace V1_0
}  // namespace foo
}  // namespace tests
}  // namespace hardware
}  // namespace android
