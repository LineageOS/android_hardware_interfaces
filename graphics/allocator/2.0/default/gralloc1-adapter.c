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

#define LOG_TAG "Gralloc1Adapter"

#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

#include <pthread.h>

#include <cutils/native_handle.h>
#include <hardware/gralloc1.h>
#include <sync/sync.h>
#include <log/log.h>

#include "gralloc1-adapter.h"

struct gralloc1_adapter_module {
    struct gralloc_module_t base;
    struct gralloc1_adapter adapter;
};

struct gralloc1_adapter_device {
    struct gralloc1_device base;

    struct alloc_device_t* alloc_dev;

    /* fixed size for thread safety */
    char saved_dump[4096];
    size_t saved_dump_size;
};

/* additional data associated with registered buffer_handle_t */
struct gralloc1_adapter_buffer_data {
    struct gralloc1_adapter_buffer_info info;

    atomic_int refcount;
    bool owned;
};

struct gralloc1_adapter_buffer_descriptor {
    int width;
    int height;
    int format;
    int producer_usage;
    int consumer_usage;
};

static const struct gralloc1_adapter_module* gralloc1_adapter_module(
        struct gralloc1_device* dev)
{
    return (const struct gralloc1_adapter_module*) dev->common.module;
}

static struct gralloc1_adapter_device* gralloc1_adapter_device(
        struct gralloc1_device* dev)
{
    return (struct gralloc1_adapter_device*) dev;
}

static struct gralloc1_adapter_buffer_data* lookup_buffer_data(
        struct gralloc1_device* dev, buffer_handle_t buffer)
{
    const struct gralloc1_adapter_module* mod = gralloc1_adapter_module(dev);
    if (!mod->adapter.is_registered(&mod->base, buffer))
        return NULL;

    return mod->adapter.get_data(&mod->base, buffer);
}

static struct gralloc1_adapter_buffer_descriptor* lookup_buffer_descriptor(
        struct gralloc1_device* dev, gralloc1_buffer_descriptor_t id)
{
    /* do we want to validate? */
    return (struct gralloc1_adapter_buffer_descriptor*) ((uintptr_t) id);
}

static void device_dump(struct gralloc1_device* device,
        uint32_t* outSize, char* outBuffer)
{
    struct gralloc1_adapter_device* dev = gralloc1_adapter_device(device);

    if (outBuffer) {
        uint32_t copy = (uint32_t) dev->saved_dump_size;
        if (*outSize < copy) {
            copy = *outSize;
        } else {
            *outSize = copy;
        }

        memcpy(outBuffer, dev->saved_dump, copy);
    } else {
        /* dump is optional and may not null-terminate */
        if (dev->alloc_dev->dump) {
            dev->alloc_dev->dump(dev->alloc_dev, dev->saved_dump,
                    sizeof(dev->saved_dump) - 1);
            dev->saved_dump_size = strlen(dev->saved_dump);
        }

        *outSize = (uint32_t) dev->saved_dump_size;
    }
}

static int32_t device_create_descriptor(struct gralloc1_device* device,
        gralloc1_buffer_descriptor_t* outDescriptor)
{
    struct gralloc1_adapter_buffer_descriptor* desc;

    desc = calloc(1, sizeof(*desc));
    if (!desc) {
        return GRALLOC1_ERROR_NO_RESOURCES;
    }

    *outDescriptor = (gralloc1_buffer_descriptor_t) (uintptr_t) desc;

    return GRALLOC1_ERROR_NONE;
}

static int32_t device_destroy_descriptor(struct gralloc1_device* device,
        gralloc1_buffer_descriptor_t descriptor)
{
    struct gralloc1_adapter_buffer_descriptor* desc =
        lookup_buffer_descriptor(device, descriptor);
    if (!desc) {
        return GRALLOC1_ERROR_BAD_DESCRIPTOR;
    }

    free(desc);

    return GRALLOC1_ERROR_NONE;
}

static int32_t device_set_consumer_usage(struct gralloc1_device* device,
        gralloc1_buffer_descriptor_t descriptor, uint64_t usage)
{
    struct gralloc1_adapter_buffer_descriptor* desc =
        lookup_buffer_descriptor(device, descriptor);
    if (!desc) {
        return GRALLOC1_ERROR_BAD_DESCRIPTOR;
    }

    desc->consumer_usage = (int) usage;

    return GRALLOC1_ERROR_NONE;
}

static int32_t device_set_dimensions(struct gralloc1_device* device,
        gralloc1_buffer_descriptor_t descriptor,
        uint32_t width, uint32_t height)
{
    struct gralloc1_adapter_buffer_descriptor* desc =
        lookup_buffer_descriptor(device, descriptor);
    if (!desc) {
        return GRALLOC1_ERROR_BAD_DESCRIPTOR;
    }

    desc->width = (int) width;
    desc->height = (int) height;

    return GRALLOC1_ERROR_NONE;
}

static int32_t device_set_format(struct gralloc1_device* device,
        gralloc1_buffer_descriptor_t descriptor, int32_t format)
{
    struct gralloc1_adapter_buffer_descriptor* desc =
        lookup_buffer_descriptor(device, descriptor);
    if (!desc) {
        return GRALLOC1_ERROR_BAD_DESCRIPTOR;
    }

    desc->format = format;

    return GRALLOC1_ERROR_NONE;
}

static int32_t device_set_producer_usage(struct gralloc1_device* device,
        gralloc1_buffer_descriptor_t descriptor, uint64_t usage)
{
    struct gralloc1_adapter_buffer_descriptor* desc =
        lookup_buffer_descriptor(device, descriptor);
    if (!desc) {
        return GRALLOC1_ERROR_BAD_DESCRIPTOR;
    }

    desc->producer_usage = (int) usage;

    return GRALLOC1_ERROR_NONE;
}

static int32_t device_get_backing_store(struct gralloc1_device* device,
        buffer_handle_t buffer, gralloc1_backing_store_t* outStore)
{
    /* we never share backing store */
    *outStore = (gralloc1_backing_store_t) (uintptr_t) buffer;

    return GRALLOC1_ERROR_NONE;
}

static int32_t device_get_consumer_usage(struct gralloc1_device* device,
        buffer_handle_t buffer, uint64_t* outUsage)
{
    const struct gralloc1_adapter_buffer_data* data =
        lookup_buffer_data(device, buffer);
    if (!data) {
        return GRALLOC1_ERROR_BAD_HANDLE;
    }

    *outUsage = data->info.usage;

    return GRALLOC1_ERROR_NONE;
}

static int32_t device_get_dimensions(struct gralloc1_device* device,
        buffer_handle_t buffer, uint32_t* outWidth, uint32_t* outHeight)
{
    const struct gralloc1_adapter_buffer_data* data =
        lookup_buffer_data(device, buffer);
    if (!data) {
        return GRALLOC1_ERROR_BAD_HANDLE;
    }

    *outWidth = data->info.width;
    *outHeight = data->info.height;

    return GRALLOC1_ERROR_NONE;
}

static int32_t device_get_format(struct gralloc1_device* device,
        buffer_handle_t buffer, int32_t* outFormat)
{
    const struct gralloc1_adapter_buffer_data* data =
        lookup_buffer_data(device, buffer);
    if (!data) {
        return GRALLOC1_ERROR_BAD_HANDLE;
    }

    *outFormat = data->info.format;

    return GRALLOC1_ERROR_NONE;
}

static int32_t device_get_producer_usage(struct gralloc1_device* device,
        buffer_handle_t buffer, uint64_t* outUsage)
{
    const struct gralloc1_adapter_buffer_data* data =
        lookup_buffer_data(device, buffer);
    if (!data) {
        return GRALLOC1_ERROR_BAD_HANDLE;
    }

    *outUsage = data->info.usage;

    return GRALLOC1_ERROR_NONE;
}

static int32_t device_get_stride(struct gralloc1_device* device,
        buffer_handle_t buffer, uint32_t* outStride)
{
    const struct gralloc1_adapter_buffer_data* data =
        lookup_buffer_data(device, buffer);
    if (!data) {
        return GRALLOC1_ERROR_BAD_HANDLE;
    }

    *outStride = data->info.stride;

    return GRALLOC1_ERROR_NONE;
}

static int32_t device_allocate(struct gralloc1_device* device,
        uint32_t numDescriptors,
        const gralloc1_buffer_descriptor_t* descriptors,
        buffer_handle_t* outBuffers)
{
    const struct gralloc1_adapter_module* mod =
        gralloc1_adapter_module(device);
    struct gralloc1_adapter_device* dev = gralloc1_adapter_device(device);
    gralloc1_error_t err = GRALLOC1_ERROR_NONE;
    uint32_t i;

    for (i = 0; i < numDescriptors; i++) {
        const struct gralloc1_adapter_buffer_descriptor* desc =
            lookup_buffer_descriptor(device, descriptors[i]);
        struct gralloc1_adapter_buffer_data* data;
        buffer_handle_t buffer;
        int dummy_stride;
        int ret;

        if (!desc) {
            err = GRALLOC1_ERROR_BAD_DESCRIPTOR;
            break;
        }

        data = calloc(1, sizeof(*data));
        if (!data) {
            err = GRALLOC1_ERROR_NO_RESOURCES;
            break;
        }

        ret = dev->alloc_dev->alloc(dev->alloc_dev, desc->width, desc->height,
                desc->format, desc->producer_usage | desc->consumer_usage,
                &buffer, &dummy_stride);
        if (ret) {
            free(data);
            err = GRALLOC1_ERROR_NO_RESOURCES;
            break;
        }

        mod->adapter.get_info(&mod->base, buffer, &data->info);
        data->refcount = 1;
        data->owned = true;

        mod->adapter.set_data(&mod->base, buffer, data);

        outBuffers[i] = buffer;
    }

    if (err != GRALLOC1_ERROR_NONE) {
        uint32_t j;
        for (j = 0; j < i; j++) {
            free(mod->adapter.get_data(&mod->base, outBuffers[i]));
            dev->alloc_dev->free(dev->alloc_dev, outBuffers[i]);
        }

        return err;
    }

    return (numDescriptors > 1) ?
        GRALLOC1_ERROR_NOT_SHARED : GRALLOC1_ERROR_NONE;
}

static int32_t device_retain(struct gralloc1_device* device,
        buffer_handle_t buffer)
{
    static pthread_mutex_t register_mutex = PTHREAD_MUTEX_INITIALIZER;
    const struct gralloc1_adapter_module* mod =
        gralloc1_adapter_module(device);
    struct gralloc1_adapter_buffer_data* data;

    pthread_mutex_lock(&register_mutex);

    if (mod->adapter.is_registered(&mod->base, buffer)) {
        data = mod->adapter.get_data(&mod->base, buffer);
        data->refcount++;
    } else {
        int ret;

        data = calloc(1, sizeof(*data));
        if (!data) {
            pthread_mutex_unlock(&register_mutex);
            return GRALLOC1_ERROR_NO_RESOURCES;
        }

        ret = mod->base.registerBuffer(&mod->base, buffer);
        if (ret) {
            pthread_mutex_unlock(&register_mutex);
            free(data);

            return GRALLOC1_ERROR_NO_RESOURCES;
        }

        mod->adapter.get_info(&mod->base, buffer, &data->info);
        data->refcount = 1;
        data->owned = false;

        mod->adapter.set_data(&mod->base, buffer, data);
    }

    pthread_mutex_unlock(&register_mutex);

    return GRALLOC1_ERROR_NONE;
}

static int32_t device_release(struct gralloc1_device* device,
        buffer_handle_t buffer)
{
    struct gralloc1_adapter_buffer_data* data =
        lookup_buffer_data(device, buffer);
    if (!data) {
        ALOGE("unable to release unregistered buffer %p", buffer);
        return GRALLOC1_ERROR_BAD_HANDLE;
    }

    data->refcount--;
    if (!data->refcount) {
        if (data->owned) {
            struct gralloc1_adapter_device* dev =
                gralloc1_adapter_device(device);
            dev->alloc_dev->free(dev->alloc_dev, buffer);
        } else {
            const struct gralloc1_adapter_module* mod =
                gralloc1_adapter_module(device);
            mod->base.unregisterBuffer(&mod->base, buffer);

            native_handle_close(buffer);
            native_handle_delete((native_handle_t*) buffer);
        }

        free(data);
    }

    return GRALLOC1_ERROR_NONE;
}

static int32_t device_get_num_flex_planes(struct gralloc1_device* device,
        buffer_handle_t buffer, uint32_t* outNumPlanes)
{
    const struct gralloc1_adapter_buffer_data* data =
        lookup_buffer_data(device, buffer);
    if (!data) {
        return GRALLOC1_ERROR_BAD_HANDLE;
    }

    *outNumPlanes = data->info.num_flex_planes;

    return GRALLOC1_ERROR_NONE;
}

static int32_t device_lock(struct gralloc1_device* device,
        buffer_handle_t buffer,
        uint64_t producerUsage, uint64_t consumerUsage,
        const gralloc1_rect_t* accessRegion, void** outData,
        int32_t acquireFence)
{
    const struct gralloc1_adapter_module* mod =
        gralloc1_adapter_module(device);
    const int usage = (int) (producerUsage | consumerUsage);
    const struct gralloc1_adapter_buffer_data* data =
        lookup_buffer_data(device, buffer);
    int ret;

    if (!data) {
        ALOGE("unable to lock unregistered buffer %p", buffer);
        return GRALLOC1_ERROR_BAD_HANDLE;
    }

    if (mod->adapter.real_module_api_version >=
            GRALLOC_MODULE_API_VERSION_0_3) {
        ret = mod->base.lockAsync(&mod->base,
                buffer, usage,
                accessRegion->left,
                accessRegion->top,
                accessRegion->width,
                accessRegion->height,
                outData, acquireFence);
    } else {
        if (acquireFence >= 0) {
            sync_wait(acquireFence, -1);
        }

        ret = mod->base.lock(&mod->base,
                buffer, usage,
                accessRegion->left,
                accessRegion->top,
                accessRegion->width,
                accessRegion->height,
                outData);

        if (acquireFence >= 0 && !ret) {
            close(acquireFence);
        }
    }

    return (ret) ? GRALLOC1_ERROR_NO_RESOURCES : GRALLOC1_ERROR_NONE;
}

static int32_t device_lock_flex(struct gralloc1_device* device,
        buffer_handle_t buffer,
        uint64_t producerUsage, uint64_t consumerUsage,
        const gralloc1_rect_t* accessRegion,
        struct android_flex_layout* outFlexLayout,
        int32_t acquireFence)
{
    const struct gralloc1_adapter_module* mod =
        gralloc1_adapter_module(device);
    const int usage = (int) (producerUsage | consumerUsage);
    const struct gralloc1_adapter_buffer_data* data =
        lookup_buffer_data(device, buffer);
    struct android_ycbcr ycbcr;
    int ret;

    if (!data) {
        ALOGE("unable to lockFlex unregistered buffer %p", buffer);
        return GRALLOC1_ERROR_BAD_HANDLE;
    }

    if (outFlexLayout->num_planes < data->info.num_flex_planes) {
        return GRALLOC1_ERROR_BAD_VALUE;
    }

    if (mod->adapter.real_module_api_version >=
            GRALLOC_MODULE_API_VERSION_0_3 && mod->base.lockAsync_ycbcr) {
        ret = mod->base.lockAsync_ycbcr(&mod->base,
                buffer, usage,
                accessRegion->left,
                accessRegion->top,
                accessRegion->width,
                accessRegion->height,
                &ycbcr, acquireFence);
    } else if (mod->base.lock_ycbcr) {
        if (acquireFence >= 0) {
            sync_wait(acquireFence, -1);
        }

        ret = mod->base.lock_ycbcr(&mod->base,
                buffer, usage,
                accessRegion->left,
                accessRegion->top,
                accessRegion->width,
                accessRegion->height,
                &ycbcr);

        if (acquireFence >= 0 && !ret) {
            close(acquireFence);
        }
    } else {
        return GRALLOC1_ERROR_UNSUPPORTED;
    }

    if (ret) {
        return GRALLOC1_ERROR_NO_RESOURCES;
    }

    mod->adapter.get_flexible_layout(&mod->base, buffer,
            &ycbcr, outFlexLayout);

    return GRALLOC1_ERROR_NONE;
}

static int32_t device_unlock(struct gralloc1_device* device,
        buffer_handle_t buffer, int32_t* outReleaseFence)
{
    const struct gralloc1_adapter_module* mod =
        gralloc1_adapter_module(device);
    int ret;

    if (mod->adapter.real_module_api_version >=
            GRALLOC_MODULE_API_VERSION_0_3) {
        ret = mod->base.unlockAsync(&mod->base, buffer, outReleaseFence);
    } else {
        ret = mod->base.unlock(&mod->base, buffer);
        if (!ret) {
            *outReleaseFence = -1;
        }
    }

    return (ret) ? GRALLOC1_ERROR_BAD_HANDLE : GRALLOC1_ERROR_NONE;
}

static gralloc1_function_pointer_t device_get_function(
        struct gralloc1_device* device, int32_t descriptor)
{
    switch ((gralloc1_function_descriptor_t) descriptor) {
#define CASE(id, ptr)              \
    case GRALLOC1_FUNCTION_ ## id: \
        return (gralloc1_function_pointer_t) device_ ## ptr
    CASE(DUMP, dump);
    CASE(CREATE_DESCRIPTOR, create_descriptor);
    CASE(DESTROY_DESCRIPTOR, destroy_descriptor);
    CASE(SET_CONSUMER_USAGE, set_consumer_usage);
    CASE(SET_DIMENSIONS, set_dimensions);
    CASE(SET_FORMAT, set_format);
    CASE(SET_PRODUCER_USAGE, set_producer_usage);
    CASE(GET_BACKING_STORE, get_backing_store);
    CASE(GET_CONSUMER_USAGE, get_consumer_usage);
    CASE(GET_DIMENSIONS, get_dimensions);
    CASE(GET_FORMAT, get_format);
    CASE(GET_PRODUCER_USAGE, get_producer_usage);
    CASE(GET_STRIDE, get_stride);
    CASE(ALLOCATE, allocate);
    CASE(RETAIN, retain);
    CASE(RELEASE, release);
    CASE(GET_NUM_FLEX_PLANES, get_num_flex_planes);
    CASE(LOCK, lock);
    CASE(LOCK_FLEX, lock_flex);
    CASE(UNLOCK, unlock);
#undef CASE
    default: return NULL;
    }
}

static void device_get_capabilities(struct gralloc1_device* device,
        uint32_t* outCount, int32_t* outCapabilities)
{
    *outCount = 0;
}

static int device_close(struct hw_device_t* device)
{
    struct gralloc1_adapter_device* dev =
        (struct gralloc1_adapter_device*) device;
    int ret;

    ret = dev->alloc_dev->common.close(&dev->alloc_dev->common);
    if (!ret) {
        free(dev);
    }

    return ret;
}

int gralloc1_adapter_device_open(const struct hw_module_t* module,
        const char* id, struct hw_device_t** device)
{
    const struct gralloc1_adapter_module* mod =
        (const struct gralloc1_adapter_module*) module;
    struct alloc_device_t* alloc_dev;
    struct gralloc1_adapter_device* dev;
    int ret;

    if (strcmp(id, GRALLOC_HARDWARE_MODULE_ID) != 0) {
        ALOGE("unknown gralloc1 device id: %s", id);
        return -EINVAL;
    }

    ret = module->methods->open(module, GRALLOC_HARDWARE_GPU0,
            (struct hw_device_t**) &alloc_dev);
    if (ret) {
        return ret;
    }

    dev = malloc(sizeof(*dev));
    if (!dev) {
        alloc_dev->common.close(&alloc_dev->common);
        return -ENOMEM;
    }

    *dev = (struct gralloc1_adapter_device) {
        .base = {
            .common = {
                .tag = HARDWARE_DEVICE_TAG,
                .version = HARDWARE_DEVICE_API_VERSION(0, 0),
                .module = (struct hw_module_t*) mod,
                .close = device_close,
            },
            .getCapabilities = device_get_capabilities,
            .getFunction = device_get_function,
        },
        .alloc_dev = alloc_dev,
    };

    *device = (struct hw_device_t*) dev;

    return 0;
}
