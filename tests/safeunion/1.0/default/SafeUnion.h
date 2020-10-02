/*
 * Copyright (C) 2018 The Android Open Source Project
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

#ifndef ANDROID_HARDWARE_TESTS_SAFEUNION_V1_0_SAFEUNION_H
#define ANDROID_HARDWARE_TESTS_SAFEUNION_V1_0_SAFEUNION_H

#include <android/hardware/tests/safeunion/1.0/ISafeUnion.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

namespace android {
namespace hardware {
namespace tests {
namespace safeunion {
namespace V1_0 {
namespace implementation {

using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::tests::safeunion::V1_0::ISafeUnion;

struct SafeUnion : public ISafeUnion {
    // Methods from ::android::hardware::tests::safeunion::V1_0::ISafeUnion follow.
    Return<void> newLargeSafeUnion(newLargeSafeUnion_cb _hidl_cb) override;
    Return<void> setA(const LargeSafeUnion& myUnion, int8_t a, setA_cb _hidl_cb) override;
    Return<void> setB(const LargeSafeUnion& myUnion, uint16_t b, setB_cb _hidl_cb) override;
    Return<void> setC(const LargeSafeUnion& myUnion, int32_t c, setC_cb _hidl_cb) override;
    Return<void> setD(const LargeSafeUnion& myUnion, uint64_t d, setD_cb _hidl_cb) override;
    Return<void> setE(const LargeSafeUnion& myUnion, const hidl_array<int8_t, 13>& e, setE_cb _hidl_cb) override;
    Return<void> setF(const LargeSafeUnion& myUnion, const hidl_array<int64_t, 5>& f, setF_cb _hidl_cb) override;
    Return<void> setG(const LargeSafeUnion& myUnion, const hidl_string& g, setG_cb _hidl_cb) override;
    Return<void> setH(const LargeSafeUnion& myUnion, const hidl_vec<bool>& h, setH_cb _hidl_cb) override;
    Return<void> setI(const LargeSafeUnion& myUnion, const hidl_vec<uint64_t>& i, setI_cb _hidl_cb) override;
    Return<void> setJ(const LargeSafeUnion& myUnion, const J& j, setJ_cb _hidl_cb) override;
    Return<void> setK(const LargeSafeUnion& myUnion, const LargeSafeUnion::K& k, setK_cb _hidl_cb) override;
    Return<void> setL(const LargeSafeUnion& myUnion, const SmallSafeUnion& l, setL_cb _hidl_cb) override;
    Return<void> setM(const LargeSafeUnion& myUnion, BitField m, setL_cb _hidl_cb) override;
    Return<void> setN(const LargeSafeUnion& myUnion, hidl_bitfield<BitField> n,
                      setL_cb _hidl_cb) override;

    Return<void> newInterfaceTypeSafeUnion(newInterfaceTypeSafeUnion_cb _hidl_cb) override;
    Return<void> setInterfaceA(const InterfaceTypeSafeUnion& myUnion, uint32_t a, setInterfaceA_cb _hidl_cb) override;
    Return<void> setInterfaceB(const InterfaceTypeSafeUnion& myUnion, const hidl_array<int8_t, 7>& b, setInterfaceB_cb _hidl_cb) override;
    Return<void> setInterfaceC(const InterfaceTypeSafeUnion& myUnion,
                               const sp<::android::hidl::base::V1_0::IBase>& c,
                               setInterfaceC_cb _hidl_cb) override;
    Return<void> setInterfaceD(const InterfaceTypeSafeUnion& myUnion, const hidl_string& d, setInterfaceD_cb _hidl_cb) override;
    Return<void> setInterfaceE(const InterfaceTypeSafeUnion& myUnion, const hidl_vec<hidl_string>& e, setInterfaceE_cb _hidl_cb) override;
    Return<void> setInterfaceF(const InterfaceTypeSafeUnion& myUnion, const hidl_handle& f,
                               setInterfaceF_cb _hidl_cb) override;
    Return<void> setInterfaceG(const InterfaceTypeSafeUnion& myUnion,
                               const hidl_vec<hidl_handle>& g, setInterfaceG_cb _hidl_cb) override;

    Return<void> newHandleTypeSafeUnion(newHandleTypeSafeUnion_cb _hidl_cb) override;
    Return<void> setHandleA(const HandleTypeSafeUnion& myUnion, const hidl_handle& a,
                            setHandleA_cb _hidl_cb) override;
    Return<void> setHandleB(const HandleTypeSafeUnion& myUnion, const hidl_array<hidl_handle, 5>& b,
                            setHandleB_cb _hidl_cb) override;
    Return<void> setHandleC(const HandleTypeSafeUnion& myUnion, const hidl_vec<hidl_handle>& c,
                            setHandleC_cb _hidl_cb) override;
};

extern "C" ISafeUnion* HIDL_FETCH_ISafeUnion(const char* name);

}  // namespace implementation
}  // namespace V1_0
}  // namespace safeunion
}  // namespace tests
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_TESTS_SAFEUNION_V1_0_SAFEUNION_H
