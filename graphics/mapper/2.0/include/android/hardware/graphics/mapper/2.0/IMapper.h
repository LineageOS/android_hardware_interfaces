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

#ifndef ANDROID_HARDWARE_GRAPHICS_MAPPER_V2_0_IMAPPER_H
#define ANDROID_HARDWARE_GRAPHICS_MAPPER_V2_0_IMAPPER_H

#include <type_traits>

#include <android/hardware/graphics/mapper/2.0/types.h>

extern "C" {

namespace android {
namespace hardware {
namespace graphics {
namespace mapper {
namespace V2_0 {

struct Device {
    struct Rect {
        int32_t left;
        int32_t top;
        int32_t width;
        int32_t height;
    };
    static_assert(std::is_pod<Rect>::value, "Device::Rect is not POD");

    /*
     * Create a mapper device.
     *
     * @return error is NONE upon success. Otherwise,
     *                  NOT_SUPPORTED when creation will never succeed.
     *                  BAD_RESOURCES when creation failed at this time.
     * @return device is the newly created mapper device.
     */
    typedef Error (*createDevice)(Device** outDevice);

    /*
     * Destroy a mapper device.
     *
     * @return error is always NONE.
     * @param device is the mapper device to destroy.
     */
    typedef Error (*destroyDevice)(Device* device);

    /*
     * Adds a reference to the given buffer handle.
     *
     * A buffer handle received from a remote process or exported by
     * IAllocator::exportHandle is unknown to this client-side library. There
     * is also no guarantee that the buffer's backing store will stay alive.
     * This function must be called at least once in both cases to intrdouce
     * the buffer handle to this client-side library and to secure the backing
     * store. It may also be called more than once to increase the reference
     * count if two components in the same process want to interact with the
     * buffer independently.
     *
     * @param device is the mapper device.
     * @param bufferHandle is the buffer to which a reference must be added.
     * @return error is NONE upon success. Otherwise,
     *                  BAD_BUFFER when the buffer handle is invalid
     *                  NO_RESOURCES when it is not possible to add a
     *                               reference to this buffer at this time
     */
    typedef Error (*retain)(Device* device,
                            const native_handle_t* bufferHandle);

    /*
     * Removes a reference from the given buffer buffer.
     *
     * If no references remain, the buffer handle should be freed with
     * native_handle_close/native_handle_delete. When the last buffer handle
     * referring to a particular backing store is freed, that backing store
     * should also be freed.
     *
     * @param device is the mapper device.
     * @param bufferHandle is the buffer from which a reference must be
     *        removed.
     * @return error is NONE upon success. Otherwise,
     *                  BAD_BUFFER when the buffer handle is invalid.
     */
    typedef Error (*release)(Device* device,
                             const native_handle_t* bufferHandle);

    /*
     * Gets the width and height of the buffer in pixels.
     *
     * See IAllocator::BufferDescriptorInfo for more information.
     *
     * @param device is the mapper device.
     * @param bufferHandle is the buffer from which to get the dimensions.
     * @return error is NONE upon success. Otherwise,
     *                  BAD_BUFFER when the buffer handle is invalid.
     * @return width is the width of the buffer in pixels.
     * @return height is the height of the buffer in pixels.
     */
    typedef Error (*getDimensions)(Device* device,
                                   const native_handle_t* bufferHandle,
                                   uint32_t* outWidth,
                                   uint32_t* outHeight);

    /*
     * Gets the format of the buffer.
     *
     * See IAllocator::BufferDescriptorInfo for more information.
     *
     * @param device is the mapper device.
     * @param bufferHandle is the buffer from which to get format.
     * @return error is NONE upon success. Otherwise,
     *                  BAD_BUFFER when the buffer handle is invalid.
     * @return format is the format of the buffer.
     */
    typedef Error (*getFormat)(Device* device,
                               const native_handle_t* bufferHandle,
                               PixelFormat* outFormat);

    /*
     * Gets the number of layers of the buffer.
     *
     * See IAllocator::BufferDescriptorInfo for more information.
     *
     * @param device is the mapper device.
     * @param bufferHandle is the buffer from which to get format.
     * @return error is NONE upon success. Otherwise,
     *                  BAD_BUFFER when the buffer handle is invalid.
     * @return layerCount is the number of layers of the buffer.
     */
    typedef Error (*getLayerCount)(Device* device,
                               const native_handle_t* bufferHandle,
                               uint32_t* outLayerCount);

    /*
     * Gets the producer usage flags which were used to allocate this buffer.
     *
     * See IAllocator::BufferDescriptorInfo for more information.
     *
     * @param device is the mapper device.
     * @param bufferHandle is the buffer from which to get the producer usage
     *        flags.
     * @return error is NONE upon success. Otherwise,
     *                  BAD_BUFFER when the buffer handle is invalid.
     * @return usageMask contains the producer usage flags of the buffer.
     */
    typedef Error (*getProducerUsageMask)(Device* device,
                                          const native_handle_t* bufferHandle,
                                          uint64_t* outUsageMask);

    /*
     * Gets the consumer usage flags which were used to allocate this buffer.
     *
     * See IAllocator::BufferDescriptorInfo for more information.
     *
     * @param device is the mapper device.
     * @param bufferHandle is the buffer from which to get the consumer usage
     *        flags.
     * @return error is NONE upon success. Otherwise,
     *                  BAD_BUFFER when the buffer handle is invalid.
     * @return usageMask contains the consumer usage flags of the buffer.
     */
    typedef Error (*getConsumerUsageMask)(Device* device,
                                          const native_handle_t* bufferHandle,
                                          uint64_t* outUsageMask);

    /*
     * Gets a value that uniquely identifies the backing store of the given
     * buffer.
     *
     * Buffers which share a backing store should return the same value from
     * this function. If the buffer is present in more than one process, the
     * backing store value for that buffer is not required to be the same in
     * every process.
     *
     * @param device is the mapper device.
     * @param bufferHandle is the buffer from which to get the backing store
     *        identifier.
     * @return error is NONE upon success. Otherwise,
     *                  BAD_BUFFER when the buffer handle is invalid.
     * @return store is the backing store identifier for this buffer.
     */
    typedef Error (*getBackingStore)(Device* device,
                                     const native_handle_t* bufferHandle,
                                     BackingStore* outStore);

    /*
     * Gets the stride of the buffer in pixels.
     *
     * The stride is the offset in pixel-sized elements between the same
     * column in two adjacent rows of pixels. This may not be equal to the
     * width of the buffer.
     *
     * @param device is the mapper device.
     * @param bufferHandle is the buffer from which to get the stride.
     * @return error is NONE upon success. Otherwise,
     *                  BAD_BUFFER when the buffer handle is invalid.
     *                  UNDEFINED when the notion of a stride is not
     *                            meaningful for the buffer format.
     * @return store is the stride in pixels.
     */
    typedef Error (*getStride)(Device* device,
                               const native_handle_t* bufferHandle,
                               uint32_t* outStride);

    /*
     * Returns the number of flex layout planes which are needed to represent
     * the given buffer. This may be used to efficiently allocate only as many
     * plane structures as necessary before calling into lockFlex.
     *
     * If the given buffer cannot be locked as a flex format, this function
     * may return UNSUPPORTED (as lockFlex would).
     *
     * @param device is the mapper device.
     * @param bufferHandle is the buffer for which the number of planes should
     *        be queried.
     * @return error is NONE upon success. Otherwise,
     *                  BAD_BUFFER when the buffer handle is invalid.
     *                  UNSUPPORTED when the buffer's format cannot be
     *                              represented in a flex layout.
     * @return numPlanes is the number of flex planes required to describe the
     *         given buffer.
     */
    typedef Error (*getNumFlexPlanes)(Device* device,
                                      const native_handle_t* bufferHandle,
                                      uint32_t* outNumPlanes);

    /*
     * Locks the given buffer for the specified CPU usage.
     *
     * Exactly one of producerUsageMask and consumerUsageMask must be 0. The
     * usage which is not 0 must be one of the *Usage::CPU* values, as
     * applicable. Locking a buffer for a non-CPU usage is not supported.
     *
     * Locking the same buffer simultaneously from multiple threads is
     * permitted, but if any of the threads attempt to lock the buffer for
     * writing, the behavior is undefined, except that it must not cause
     * process termination or block the client indefinitely. Leaving the
     * buffer content in an indeterminate state or returning an error are both
     * acceptable.
     *
     * The client must not modify the content of the buffer outside of
     * accessRegion, and the device need not guarantee that content outside of
     * accessRegion is valid for reading. The result of reading or writing
     * outside of accessRegion is undefined, except that it must not cause
     * process termination.
     *
     * data will be filled with a pointer to the locked buffer memory. This
     * address will represent the top-left corner of the entire buffer, even
     * if accessRegion does not begin at the top-left corner.
     *
     * acquireFence is a file descriptor referring to a acquire sync fence
     * object, which will be signaled when it is safe for the device to access
     * the contents of the buffer (prior to locking). If it is already safe to
     * access the buffer contents, -1 may be passed instead.
     *
     * @param device is the mapper device.
     * @param bufferHandle is the buffer to lock.
     * @param producerUsageMask contains the producer usage flags to request;
     *        either this or consumerUsagemask must be 0, and the other must
     *        be a CPU usage.
     * @param consumerUsageMask contains the consumer usage flags to request;
     *        either this or producerUsageMask must be 0, and the other must
     *        be a CPU usage.
     * @param accessRegion is the portion of the buffer that the client
     *        intends to access.
     * @param acquireFence is a sync fence file descriptor as described above.
     * @return error is NONE upon success. Otherwise,
     *                  BAD_BUFFER when the buffer handle is invalid.
     *                  BAD_VALUE when neither or both of producerUsageMask
     *                            and consumerUsageMask were 0, or the usage
     *                            which was not 0 was not a CPU usage.
     *                  NO_RESOURCES when the buffer cannot be locked at this
     *                               time, but locking may succeed at a future
     *                               time.
     *                  UNSUPPORTED when the buffer cannot be locked with the
     *                              given usage, and any future attempts at
     *                              locking will also fail.
     * @return data will be filled with a CPU-accessible pointer to the buffer
     *         data.
     */
    typedef Error (*lock)(Device* device,
                          const native_handle_t* bufferHandle,
                          uint64_t producerUsageMask,
                          uint64_t consumerUsageMask,
                          const Rect* accessRegion,
                          int32_t acquireFence,
                          void** outData);

    /*
     * This is largely the same as lock(), except that instead of returning a
     * pointer directly to the buffer data, it returns an FlexLayout struct
     * describing how to access the data planes.
     *
     * This function must work on buffers with PixelFormat::YCbCr_*_888 if
     * supported by the device, as well as with any other formats requested by
     * multimedia codecs when they are configured with a
     * flexible-YUV-compatible color format.
     *
     * This function may also be called on buffers of other formats, including
     * non-YUV formats, but if the buffer format is not compatible with a
     * flexible representation, it may return UNSUPPORTED.
     *
     * @param device is the mapper device.
     * @param bufferHandle is the buffer to lock.
     * @param producerUsageMask contains the producer usage flags to request;
     *        either this or consumerUsagemask must be 0, and the other must
     *        be a CPU usage.
     * @param consumerUsageMask contains the consumer usage flags to request;
     *        either this or producerUsageMask must be 0, and the other must
     *        be a CPU usage.
     * @param accessRegion is the portion of the buffer that the client
     *        intends to access.
     * @param acquireFence is a sync fence file descriptor as described in
     *        lock().
     * @return error is NONE upon success. Otherwise,
     *                  BAD_BUFFER when the buffer handle is invalid.
     *                  BAD_VALUE when neither or both of producerUsageMask
     *                            and consumerUsageMask were 0, or the usage
     *                            which was not 0 was not a CPU usage.
     *                  NO_RESOURCES when the buffer cannot be locked at this
     *                               time, but locking may succeed at a future
     *                               time.
     *                  UNSUPPORTED when the buffer cannot be locked with the
     *                              given usage, and any future attempts at
     *                              locking will also fail.
     * @return flexLayout will be filled with the description of the planes in
     *         the buffer.
     */
    typedef Error (*lockFlex)(Device* device,
                              const native_handle_t* bufferHandle,
                              uint64_t producerUsageMask,
                              uint64_t consumerUsageMask,
                              const Rect* accessRegion,
                              int32_t acquireFence,
                              FlexLayout* outFlexLayout);

    /*
     * This function indicates to the device that the client will be done with
     * the buffer when releaseFence signals.
     *
     * releaseFence will be filled with a file descriptor referring to a
     * release sync fence object, which will be signaled when it is safe to
     * access the contents of the buffer (after the buffer has been unlocked).
     * If it is already safe to access the buffer contents, then -1 may be
     * returned instead.
     *
     * This function is used to unlock both buffers locked by lock() and those
     * locked by lockFlex().
     *
     * @param device is the mapper device.
     * @param bufferHandle is the buffer to unlock.
     * @return error is NONE upon success. Otherwise,
     *                  BAD_BUFFER when the buffer handle is invalid.
     * @return releaseFence is a sync fence file descriptor as described
     *         above.
     */
    typedef Error (*unlock)(Device* device,
                            const native_handle_t* bufferHandle,
                            int32_t* outReleaseFence);
};
static_assert(std::is_pod<Device>::value, "Device is not POD");

struct IMapper {
    Device::createDevice createDevice;
    Device::destroyDevice destroyDevice;

    Device::retain retain;
    Device::release release;
    Device::getDimensions getDimensions;
    Device::getFormat getFormat;
    Device::getLayerCount getLayerCount;
    Device::getProducerUsageMask getProducerUsageMask;
    Device::getConsumerUsageMask getConsumerUsageMask;
    Device::getBackingStore getBackingStore;
    Device::getStride getStride;
    Device::getNumFlexPlanes getNumFlexPlanes;
    Device::lock lock;
    Device::lockFlex lockFlex;
    Device::unlock unlock;
};
static_assert(std::is_pod<IMapper>::value, "IMapper is not POD");

} // namespace V2_0
} // namespace mapper
} // namespace graphics
} // namespace hardware
} // namespace android

const void* HALLIB_FETCH_Interface(const char* name);

} // extern "C"

#endif /* ANDROID_HARDWARE_GRAPHICS_MAPPER_V2_0_IMAPPER_H */
