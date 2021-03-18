/*
 * Copyright (C) 2020 The Android Open Source Project
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

package android.hardware.neuralnetworks;

/**
 * Describes the location of a data object.
 *
 * If the data object is an omitted operand, all of the fields must be 0. If the poolIndex refers to
 * a driver-managed buffer allocated from IDevice::allocate, or an AHardwareBuffer of a format other
 * than AHARDWAREBUFFER_FORMAT_BLOB, the offset, length, and padding must be set to 0 indicating
 * the entire pool is used.
 *
 * Otherwise, the offset, length, and padding specify a sub-region of a memory pool. The sum of
 * offset, length, and padding must not exceed the total size of the specified memory pool. If the
 * data object is a scalar operand or a tensor operand with fully specified dimensions, the value of
 * length must be equal to the raw size of the operand (i.e. the size of an element multiplied
 * by the number of elements). When used in Operand, the value of padding must be 0. When used in
 * RequestArgument, the value of padding specifies the extra bytes at the end of the memory region
 * that may be used by the device to access memory in chunks, for efficiency. If the data object is
 * a Request output whose dimensions are not fully specified, the value of length specifies the
 * total size of the writable region of the output data, and padding specifies the extra bytes at
 * the end of the memory region that may be used by the device to access memory in chunks, for
 * efficiency, but must not be used to hold any output data.
 *
 * When used in RequestArgument, clients should prefer to align and pad the sub-region to
 * 64 bytes when possible; this may allow the device to access the sub-region more efficiently.
 * The sub-region is aligned to 64 bytes if the value of offset is a multiple of 64.
 * The sub-region is padded to 64 bytes if the sum of length and padding is a multiple of 64.
 */
@VintfStability
parcelable DataLocation {
    /**
     * The index of the memory pool where this location is found.
     */
    int poolIndex;
    /**
     * Offset in bytes from the start of the pool.
     */
    long offset;
    /**
     * The length of the data in bytes.
     */
    long length;
    /**
     * The end padding of the specified memory region in bytes.
     */
    long padding;
}
