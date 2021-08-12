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

package android.hardware.memtrack;

import android.hardware.memtrack.DeviceInfo;
import android.hardware.memtrack.MemtrackRecord;
import android.hardware.memtrack.MemtrackType;

/**
 * The Memory Tracker HAL is designed to return information about
 * device-specific memory usage.
 * The primary goal is to be able to track memory that is not
 * trackable in any other way, for example texture memory that is allocated by
 * a process, but not mapped in to that process's address space.
 * A secondary goal is to be able to categorize memory used by a process into
 * GL, graphics, etc. All memory sizes must be in real memory usage,
 * accounting for stride, bit depth, rounding up to page size, etc.
 *
 * The following getMemory() categories are important for memory accounting in
 * Android frameworks (e.g. `dumpsys meminfo`) and should be reported as described
 * below:
 *
 * - MemtrackType::GRAPHICS and MemtrackRecord::FLAG_SMAPS_UNACCOUNTED
 *     This should report the PSS of all CPU-Mapped DMA-BUFs (buffers mapped into
 *     the process address space) and all GPU-Mapped DMA-BUFs (buffers mapped into
 *     the GPU device address space on behalf of the process), removing any overlap
 *     between the CPU-mapped and GPU-mapped sets.
 *
 * - MemtrackType::GL and MemtrackRecord::FLAG_SMAPS_UNACCOUNTED
 *     This category should report all GPU private allocations for the specified
 *     PID that are not accounted in /proc/<pid>/smaps.
 *
 *     getMemory() called with PID 0 should report the global total GPU-private
 *     memory, for MemtrackType::GL and MemtrackRecord::FLAG_SMAPS_UNACCOUNTED.
 *
 *     getMemory() called with PID 0 for a MemtrackType other than GL should
 *     report 0.
 *
 * - MemtrackType::OTHER and MemtrackRecord::FLAG_SMAPS_UNACCOUNTED
 *     Any other memory not accounted for in /proc/<pid>/smaps if any, otherwise
 *     this should return 0.
 *
 * SMAPS_UNACCOUNTED memory should also include memory that is mapped with
 * VM_PFNMAP flag set. For these mappings PSS and RSS are reported as 0 in smaps.
 * Such mappings have no backing page structs from which PSS/RSS can be calculated.
 *
 * Any memtrack operation that is not supported should return a binder status with
 * exception code EX_UNSUPPORTED_OPERATION.
 *
 * Constructor for the interface should be used to perform memtrack management
 * setup actions and must be called once before any calls to getMemory().
 */
@VintfStability
interface IMemtrack {
    /**
     * getMemory() populates MemtrackRecord vector with the sizes of memory
     * plus associated flags for that memory.
     *
     * A process collecting memory statistics will call getMemory for each
     * combination of pid and memory type. For each memory type that it
     * recognizes, the HAL must fill out an array of memtrack_record
     * structures breaking down the statistics of that memory type as much as
     * possible. For example,
     * getMemory(<pid>, GL) might return:
     * { { 4096,  ACCOUNTED | PRIVATE | SYSTEM },
     *   { 40960, UNACCOUNTED | PRIVATE | SYSTEM },
     *   { 8192,  ACCOUNTED | PRIVATE | DEDICATED },
     *   { 8192,  UNACCOUNTED | PRIVATE | DEDICATED } }
     * If the HAL cannot differentiate between SYSTEM and DEDICATED memory, it
     * could return:
     * { { 12288,  ACCOUNTED | PRIVATE },
     *   { 49152,  UNACCOUNTED | PRIVATE } }
     *
     * Memory must not overlap between types. For example, a graphics buffer
     * that has been mapped into the GPU as a surface must show up when
     * GRAPHICS is requested and not when GL
     * is requested.
     *
     * @param pid process for which memory information is requested
     * @param type memory type that information is being requested about
     * @return vector of MemtrackRecord containing memory information
     */
    MemtrackRecord[] getMemory(in int pid, in MemtrackType type);

    /**
     * getGpuDeviceInfo() populates DeviceInfo with the ID and name
     * of each GPU device.
     *
     * For example, getGpuDeviceInfor, might return:
     * { { 0, <gpu-device-name> },
     *   { 1, <gpu-device-name> } }
     *
     * This information is used to identify GPU devices for GPU specific
     * memory accounting (e.g. DMA buffer usage).
     *
     * The device name(s) provided in getGpuDeviceInfo() must match the
     * device name in the corresponding device(s) sysfs entry.
     *
     * @return vector of DeviceInfo populated for all GPU devices.
     */
    DeviceInfo[] getGpuDeviceInfo();
}
