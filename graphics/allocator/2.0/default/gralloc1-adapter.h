/*
 * Copyright 2016 The Android Open Source Project
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
#ifndef ANDROID_HARDWARE_GRALLOC1_ADAPTER_H
#define ANDROID_HARDWARE_GRALLOC1_ADAPTER_H

#include <stdbool.h>
#include <hardware/gralloc.h>

__BEGIN_DECLS

struct gralloc1_adapter_buffer_info {
    int width;
    int height;
    int format;
    int usage;

    int stride;
    uint32_t num_flex_planes;
};

/* This struct must be embedded in the HAL's HAL_MODULE_INFO_SYM and must
 * follow gralloc_module_t immediately. */
struct gralloc1_adapter {
    uint16_t real_module_api_version;

    /* Return true if the buffer is registered.  A locally allocated buffer is
     * always registered.
     *
     * This function is called frequently.  It must be thread safe just like
     * other functions are.
     */
    bool (*is_registered)(const struct gralloc_module_t* mod,
            buffer_handle_t buffer);

    /* Set the adapter data for a registered buffer. */
    void (*set_data)(const struct gralloc_module_t* mod,
            buffer_handle_t buffer, void* data);

    /* Get the adapter data for a registered buffer. */
    void* (*get_data)(const struct gralloc_module_t* mod,
            buffer_handle_t buffer);

    /* Get the buffer info, such as width, height, etc. */
    void (*get_info)(const struct gralloc_module_t* mod,
            buffer_handle_t buffer,
            struct gralloc1_adapter_buffer_info* info);

    /* Get the flexilble layout matching ycbcr. */
    void (*get_flexible_layout)(const struct gralloc_module_t* mod,
            buffer_handle_t buffer, const struct android_ycbcr* ycbcr,
            struct android_flex_layout* layout);
};

int gralloc1_adapter_device_open(const struct hw_module_t* module,
        const char* id, struct hw_device_t** device);

__END_DECLS

#endif /* ANDROID_HARDWARE_GRALLOC1_ADAPTER_H */
