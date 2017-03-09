#include "Baz.h"
#include <android-base/logging.h>

namespace android {
namespace hardware {
namespace tests {
namespace baz {
namespace V1_0 {
namespace implementation {

struct BazCallback : public IBazCallback {
    Return<void> heyItsMe(const sp<IBazCallback> &cb) override;
    Return<void> hey() override;
};

Return<void> BazCallback::heyItsMe(
        const sp<IBazCallback> &cb) {
    LOG(INFO) << "SERVER: heyItsMe cb = " << cb.get();

    return Void();
}

Return<void> BazCallback::hey() {
    LOG(INFO) << "SERVER: hey";

    return Void();
}

// TODO(b/35703683) : replace usage of below methods with toString()

static std::string to_string(const IBaz::Foo::Bar &bar);
static std::string to_string(const IBaz::Foo &foo);
static std::string to_string(const hidl_string &s);
static std::string to_string(bool x);
static std::string to_string(const IBaz::StringMatrix5x3 &M);

template<typename T, size_t SIZE>
static std::string to_string(const hidl_array<T, SIZE> &array);

template<size_t SIZE>
static std::string to_string(const hidl_array<uint8_t, SIZE> &array);

template<typename T>
static std::string to_string(const hidl_vec<T> &vec) {
    std::string out;
    out = "[";
    for (size_t i = 0; i < vec.size(); ++i) {
        if (i > 0) {
            out += ", ";
        }
        out += to_string(vec[i]);
    }
    out += "]";

    return out;
}

template<typename T, size_t SIZE>
static std::string to_string(const hidl_array<T, SIZE> &array) {
    std::string out;
    out = "[";
    for (size_t i = 0; i < SIZE; ++i) {
        if (i > 0) {
            out += ", ";
        }
        out += to_string(array[i]);
    }
    out += "]";

    return out;
}

template<size_t SIZE>
static std::string to_string(const hidl_array<uint8_t, SIZE> &array) {
    std::string out;
    for (size_t i = 0; i < SIZE; ++i) {
        if (i > 0) {
            out += ":";
        }

        char tmp[3];
        sprintf(tmp, "%02x", array[i]);

        out += tmp;
    }

    return out;
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

static std::string to_string(bool x) {
    return x ? "true" : "false";
}

static std::string to_string(const hidl_string &s) {
    return std::string("'") + s.c_str() + "'";
}

static std::string to_string(const IBaz::Foo::Bar &bar) {
    std::string out;
    out = "Bar(";
    out += "z = " + to_string(bar.z) + ", ";
    out += "s = '" + std::string(bar.s.c_str()) + "'";
    out += ")";

    return out;
}

static std::string to_string(const IBaz::Foo &foo) {
    std::string out;
    out = "Foo(";
    out += "x = " + to_string(foo.x) + ", ";
    out += "y = " + to_string(foo.y) + ", ";
    out += "aaa = " + to_string(foo.aaa);
    out += ")";

    return out;
}

static std::string to_string(const IBaz::StringMatrix5x3 &M) {
    return to_string(M.s);
}

static std::string VectorOfArray_to_string(const IBaz::VectorOfArray &in) {
    std::string out;
    out += "VectorOfArray(";

    for (size_t i = 0; i < in.addresses.size(); ++i) {
        if (i > 0) {
            out += ", ";
        }

        for (size_t j = 0; j < 6; ++j) {
            if (j > 0) {
                out += ":";
            }

            char tmp[3];
            sprintf(tmp, "%02x", in.addresses[i][j]);

            out += tmp;
        }
    }

    out += ")";

    return out;
}

// Methods from ::android::hardware::tests::baz::V1_0::IBase follow.
Return<void> Baz::someBaseMethod() {
    LOG(INFO) << "Baz::someBaseMethod";

    return Void();
}

Return<bool> Baz::someBoolMethod(bool x) {
    LOG(INFO) << "Baz::someBoolMethod(" << to_string(x) << ")";

    return !x;
}

Return<void> Baz::someBoolArrayMethod(const hidl_array<bool, 3>& x,
                                      someBoolArrayMethod_cb _hidl_cb) {
    LOG(INFO) << "Baz::someBoolArrayMethod("
        << to_string(x[0])
        << ", "
        << to_string(x[1])
        << ", "
        << to_string(x[2])
        << ")";

    hidl_array<bool, 4> out;
    out[0] = !x[0];
    out[1] = !x[1];
    out[2] = !x[2];
    out[3] = true;

    _hidl_cb(out);

    return Void();
}

Return<void> Baz::someBoolVectorMethod(const hidl_vec<bool>& x, someBoolVectorMethod_cb _hidl_cb) {
    LOG(INFO) << "Baz::someBoolVectorMethod(" << to_string(x) << ")";

    hidl_vec<bool> out;
    out.resize(x.size());
    for (size_t i = 0; i < x.size(); ++i) {
        out[i] = !x[i];
    }

    _hidl_cb(out);

    return Void();
}

Return<void> Baz::someOtherBaseMethod(const IBase::Foo& foo, someOtherBaseMethod_cb _hidl_cb) {
    LOG(INFO) << "Baz::someOtherBaseMethod "
              << to_string(foo);

    _hidl_cb(foo);

    return Void();
}

Return<void> Baz::someMethodWithFooArrays(const hidl_array<IBase::Foo, 2>& fooInput,
                                          someMethodWithFooArrays_cb _hidl_cb) {
    LOG(INFO) << "Baz::someMethodWithFooArrays "
              << to_string(fooInput);

    hidl_array<IBaz::Foo, 2> fooOutput;
    fooOutput[0] = fooInput[1];
    fooOutput[1] = fooInput[0];

    _hidl_cb(fooOutput);

    return Void();
}

Return<void> Baz::someMethodWithFooVectors(const hidl_vec<IBase::Foo>& fooInput,
                                           someMethodWithFooVectors_cb _hidl_cb) {
    LOG(INFO) << "Baz::someMethodWithFooVectors "
              << to_string(fooInput);

    hidl_vec<IBaz::Foo> fooOutput;
    fooOutput.resize(2);
    fooOutput[0] = fooInput[1];
    fooOutput[1] = fooInput[0];

    _hidl_cb(fooOutput);

    return Void();
}

Return<void> Baz::someMethodWithVectorOfArray(const IBase::VectorOfArray& in,
                                              someMethodWithVectorOfArray_cb _hidl_cb) {
    LOG(INFO) << "Baz::someMethodWithVectorOfArray "
              << VectorOfArray_to_string(in);

    IBase::VectorOfArray out;

    const size_t n = in.addresses.size();
    out.addresses.resize(n);

    for (size_t i = 0; i < n; ++i) {
        out.addresses[i] = in.addresses[n - 1 - i];
    }

    _hidl_cb(out);

    return Void();
}

Return<void> Baz::someMethodTakingAVectorOfArray(const hidl_vec<hidl_array<uint8_t, 6>>& in,
                                                 someMethodTakingAVectorOfArray_cb _hidl_cb) {
    LOG(INFO) << "Baz::someMethodTakingAVectorOfArray "
              << to_string(in);

    const size_t n = in.size();

    hidl_vec<hidl_array<uint8_t, 6> > out;
    out.resize(n);

    for (size_t i = 0; i < n; ++i) {
        out[i] = in[n - 1 - i];
    }

    _hidl_cb(out);

    return Void();
}

Return<void> Baz::transpose(const IBase::StringMatrix5x3& in, transpose_cb _hidl_cb) {
    LOG(INFO) << "Baz::transpose " << to_string(in);

    IBase::StringMatrix3x5 out;
    for (size_t i = 0; i < 3; ++i) {
        for (size_t j = 0; j < 5; ++j) {
            out.s[i][j] = in.s[j][i];
        }
    }

    _hidl_cb(out);

    return Void();
}

Return<void> Baz::transpose2(const hidl_array<hidl_string, 5, 3>& in, transpose2_cb _hidl_cb) {
    LOG(INFO) << "Baz::transpose2 " << to_string(in);

    hidl_array<hidl_string, 3, 5> out;
    for (size_t i = 0; i < 3; ++i) {
        for (size_t j = 0; j < 5; ++j) {
            out[i][j] = in[j][i];
        }
    }

    _hidl_cb(out);

    return Void();
}

Return<void> Baz::takeAMask(IBase::BitField bf,
                            uint8_t first,
                            const IBase::MyMask& second,
                            uint8_t third,
                            takeAMask_cb _hidl_cb) {
    _hidl_cb(bf, bf | first, second.value & bf, (bf | bf) & third);
    return Void();
}

// Methods from ::android::hardware::tests::baz::V1_0::IBaz follow.

Return<void> Baz::doThis(float param) {
    LOG(INFO) << "Baz::doThis(" << param << ")";

    return Void();
}

Return<int32_t> Baz::doThatAndReturnSomething(int64_t param) {
    LOG(INFO) << "Baz::doThatAndReturnSomething(" << param << ")";

    return 666;
}

Return<double> Baz::doQuiteABit(int32_t a, int64_t b, float c, double d) {
    LOG(INFO) << "Baz::doQuiteABit("
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

Return<void> Baz::doSomethingElse(const hidl_array<int32_t, 15>& param,
                                  doSomethingElse_cb _hidl_cb) {
    LOG(INFO) << "Baz::doSomethingElse(...)";

    hidl_array<int32_t, 32> result;
    for (size_t i = 0; i < 15; ++i) {
        result[i] = 2 * param[i];
        result[15 + i] = param[i];
    }
    result[30] = 1;
    result[31] = 2;

    _hidl_cb(result);

    return Void();
}

Return<void> Baz::doStuffAndReturnAString(doStuffAndReturnAString_cb _hidl_cb) {
    LOG(INFO) << "doStuffAndReturnAString";

    hidl_string s;
    s = "Hello, world!";

    _hidl_cb(s);

    return Void();
}

Return<void> Baz::mapThisVector(const hidl_vec<int32_t>& param, mapThisVector_cb _hidl_cb) {
    LOG(INFO) << "mapThisVector";

    hidl_vec<int32_t> out;
    out.resize(param.size());
    for (size_t i = 0; i < param.size(); ++i) {
        out[i] = param[i] * 2;
    }

    _hidl_cb(out);

    return Void();
}

Return<void> Baz::callMe(const sp<IBazCallback>& cb) {
    LOG(INFO) << "callMe " << cb.get();

    if (cb != NULL) {
        sp<IBazCallback> my_cb = new BazCallback;
        cb->heyItsMe(my_cb);
    }

    return Void();
}

Return<void> Baz::callMeLater(const sp<IBazCallback>& cb) {
    LOG(INFO) << "callMeLater " << cb.get();

    mStoredCallback = cb;

    return Void();
}

Return<void> Baz::iAmFreeNow() {
    if (mStoredCallback != nullptr) {
        mStoredCallback->hey();
    }
    return Void();
}

Return<void> Baz::dieNow() {
    exit(1);
    return Void();
}

Return<IBaz::SomeEnum> Baz::useAnEnum(IBaz::SomeEnum zzz) {
    LOG(INFO) << "useAnEnum " << (int)zzz;

    return SomeEnum::goober;
}

Return<void> Baz::haveSomeStrings(const hidl_array<hidl_string, 3>& array,
                                  haveSomeStrings_cb _hidl_cb) {
    LOG(INFO) << "haveSomeStrings("
              << to_string(array)
              << ")";

    hidl_array<hidl_string, 2> result;
    result[0] = "Hello";
    result[1] = "World";

    _hidl_cb(result);

    return Void();
}

Return<void> Baz::haveAStringVec(const hidl_vec<hidl_string>& vector,
                                 haveAStringVec_cb _hidl_cb) {
    LOG(INFO) << "haveAStringVec(" << to_string(vector) << ")";

    hidl_vec<hidl_string> result;
    result.resize(2);

    result[0] = "Hello";
    result[1] = "World";

    _hidl_cb(result);

    return Void();
}

Return<void> Baz::returnABunchOfStrings(returnABunchOfStrings_cb _hidl_cb) {
    hidl_string eins; eins = "Eins";
    hidl_string zwei; zwei = "Zwei";
    hidl_string drei; drei = "Drei";
    _hidl_cb(eins, zwei, drei);

    return Void();
}

Return<uint8_t> Baz::returnABitField() {
    return 0;
}

Return<uint32_t> Baz::size(uint32_t size) {
    return size;
}

Return<void> Baz::getNestedStructs(getNestedStructs_cb _hidl_cb) {
    int size = 5;
    hidl_vec<IBaz::NestedStruct> result;
    result.resize(size);
    for (int i = 0; i < size; i++) {
        result[i].a = i;
        if (i == 1) {
            result[i].matrices.resize(6);
        }
    }
    _hidl_cb(result);
    return Void();
}
// Methods from ::android::hidl::base::V1_0::IBase follow.

IBaz* HIDL_FETCH_IBaz(const char* /* name */) {
    return new Baz();
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace baz
}  // namespace tests
}  // namespace hardware
}  // namespace android
