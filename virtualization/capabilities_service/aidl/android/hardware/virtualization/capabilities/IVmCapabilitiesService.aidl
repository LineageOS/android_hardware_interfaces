/*
 * Copyright (C) 2024 The Android Open Source Project
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

package android.hardware.virtualization.capabilities;

/**
 * Encapsulates vendor-specific capabilities that can be granted to VMs.
 */
@VintfStability
interface IVmCapabilitiesService {
    /**
     * Grant access for the VM represented by the given vm_fd to the given vendor-owned tee
     * services. The names in |vendorTeeServices| must match the ones defined in the
     * tee_service_contexts files.
     * TODO(ioffe): link to the integration doc for custom smc filtering feature once
     * it's ready.
     */
    void grantAccessToVendorTeeServices(
            in ParcelFileDescriptor vmFd, in String[] vendorTeeServices);
}
