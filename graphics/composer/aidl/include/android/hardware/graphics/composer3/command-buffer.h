/*
 * Copyright 2021 The Android Open Source Project
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

#pragma once

#include <algorithm>
#include <limits>
#include <memory>
#include <vector>

#include <inttypes.h>
#include <string.h>

#include <aidl/android/hardware/graphics/composer3/BlendMode.h>
#include <aidl/android/hardware/graphics/composer3/ClientTargetProperty.h>
#include <aidl/android/hardware/graphics/composer3/Color.h>
#include <aidl/android/hardware/graphics/composer3/Command.h>
#include <aidl/android/hardware/graphics/composer3/Composition.h>
#include <aidl/android/hardware/graphics/composer3/FloatColor.h>
#include <aidl/android/hardware/graphics/composer3/HandleIndex.h>
#include <aidl/android/hardware/graphics/composer3/IComposer.h>
#include <aidl/android/hardware/graphics/composer3/IComposerClient.h>
#include <aidl/android/hardware/graphics/composer3/PerFrameMetadata.h>
#include <aidl/android/hardware/graphics/composer3/PerFrameMetadataBlob.h>

#include <aidl/android/hardware/graphics/common/ColorTransform.h>
#include <aidl/android/hardware/graphics/common/FRect.h>
#include <aidl/android/hardware/graphics/common/Rect.h>
#include <aidl/android/hardware/graphics/common/Transform.h>

#include <fmq/AidlMessageQueue.h>
#include <log/log.h>
#include <sync/sync.h>

#include <aidlcommonsupport/NativeHandle.h>

using aidl::android::hardware::graphics::common::ColorTransform;
using aidl::android::hardware::graphics::common::Dataspace;
using aidl::android::hardware::graphics::common::FRect;
using aidl::android::hardware::graphics::common::Rect;
using aidl::android::hardware::graphics::common::Transform;

using aidl::android::hardware::graphics::composer3::BlendMode;
using aidl::android::hardware::graphics::composer3::ClientTargetProperty;
using aidl::android::hardware::graphics::composer3::Color;
using aidl::android::hardware::graphics::composer3::Command;
using aidl::android::hardware::graphics::composer3::Composition;
using aidl::android::hardware::graphics::composer3::FloatColor;
using aidl::android::hardware::graphics::composer3::HandleIndex;
using aidl::android::hardware::graphics::composer3::PerFrameMetadata;
using aidl::android::hardware::graphics::composer3::PerFrameMetadataBlob;

using aidl::android::hardware::common::NativeHandle;
using aidl::android::hardware::common::fmq::SynchronizedReadWrite;
using android::AidlMessageQueue;
using CommandQueueType = AidlMessageQueue<int32_t, SynchronizedReadWrite>;
using aidl::android::hardware::common::fmq::MQDescriptor;
using DescriptorType = MQDescriptor<int32_t, SynchronizedReadWrite>;

namespace aidl::android::hardware::graphics::composer3 {

// This class helps build a command queue.  Note that all sizes/lengths are in
// units of uint32_t's.
class CommandWriterBase {
  public:
    CommandWriterBase(uint32_t initialMaxSize) : mDataMaxSize(initialMaxSize) {
        mData = std::make_unique<int32_t[]>(mDataMaxSize);
        reset();
    }

    virtual ~CommandWriterBase() { reset(); }

    void reset() {
        mDataWritten = 0;
        mCommandEnd = 0;

        // handles in mDataHandles are owned by the caller
        mDataHandles.clear();

        // handles in mTemporaryHandles are owned by the writer
        for (auto handle : mTemporaryHandles) {
            native_handle_close(handle);
            native_handle_delete(handle);
        }
        mTemporaryHandles.clear();
    }

    Command getCommand(uint32_t offset) {
        uint32_t val = (offset < mDataWritten) ? mData[offset] : 0;
        return static_cast<Command>(val & static_cast<uint32_t>(Command::OPCODE_MASK));
    }

    bool writeQueue(bool* outQueueChanged, int32_t* outCommandLength,
                    std::vector<NativeHandle>* outCommandHandles) {
        if (mDataWritten == 0) {
            *outQueueChanged = false;
            *outCommandLength = 0;
            outCommandHandles->clear();
            return true;
        }

        // After data are written to the queue, it may not be read by the
        // remote reader when
        //
        //  - the writer does not send them (because of other errors)
        //  - the hwbinder transaction fails
        //  - the reader does not read them (because of other errors)
        //
        // Discard the stale data here.
        size_t staleDataSize = mQueue ? mQueue->availableToRead() : 0;
        if (staleDataSize > 0) {
            ALOGW("discarding stale data from message queue");
            CommandQueueType::MemTransaction tx;
            if (mQueue->beginRead(staleDataSize, &tx)) {
                mQueue->commitRead(staleDataSize);
            }
        }

        // write data to queue, optionally resizing it
        if (mQueue && (mDataMaxSize <= mQueue->getQuantumCount())) {
            if (!mQueue->write(mData.get(), mDataWritten)) {
                ALOGE("failed to write commands to message queue");
                return false;
            }

            *outQueueChanged = false;
        } else {
            auto newQueue = std::make_unique<CommandQueueType>(mDataMaxSize);
            if (!newQueue->isValid() || !newQueue->write(mData.get(), mDataWritten)) {
                ALOGE("failed to prepare a new message queue ");
                return false;
            }

            mQueue = std::move(newQueue);
            *outQueueChanged = true;
        }

        *outCommandLength = mDataWritten;
        *outCommandHandles = std::move(mDataHandles);

        return true;
    }

    DescriptorType getMQDescriptor() const {
        return (mQueue) ? mQueue->dupeDesc() : DescriptorType{};
    }

    static constexpr uint16_t kSelectDisplayLength = 2;
    void selectDisplay(int64_t display) {
        beginCommand(Command::SELECT_DISPLAY, kSelectDisplayLength);
        write64(display);
        endCommand();
    }

    static constexpr uint16_t kSelectLayerLength = 2;
    void selectLayer(int64_t layer) {
        beginCommand(Command::SELECT_LAYER, kSelectLayerLength);
        write64(layer);
        endCommand();
    }

    static constexpr uint16_t kSetErrorLength = 2;
    void setError(uint32_t location, int32_t error) {
        beginCommand(Command::SET_ERROR, kSetErrorLength);
        write(location);
        writeSigned(error);
        endCommand();
    }

    static constexpr uint32_t kPresentOrValidateDisplayResultLength = 1;
    void setPresentOrValidateResult(uint32_t state) {
        beginCommand(Command::SET_PRESENT_OR_VALIDATE_DISPLAY_RESULT,
                     kPresentOrValidateDisplayResultLength);
        write(state);
        endCommand();
    }

    void setChangedCompositionTypes(const std::vector<int64_t>& layers,
                                    const std::vector<Composition>& types) {
        size_t totalLayers = std::min(layers.size(), types.size());
        size_t currentLayer = 0;

        while (currentLayer < totalLayers) {
            size_t count =
                    std::min(totalLayers - currentLayer, static_cast<size_t>(kMaxLength) / 3);

            beginCommand(Command::SET_CHANGED_COMPOSITION_TYPES, count * 3);
            for (size_t i = 0; i < count; i++) {
                write64(layers[currentLayer + i]);
                writeSigned(static_cast<int32_t>(types[currentLayer + i]));
            }
            endCommand();

            currentLayer += count;
        }
    }

    void setDisplayRequests(uint32_t displayRequestMask, const std::vector<int64_t>& layers,
                            const std::vector<uint32_t>& layerRequestMasks) {
        size_t totalLayers = std::min(layers.size(), layerRequestMasks.size());
        size_t currentLayer = 0;

        while (currentLayer < totalLayers) {
            size_t count =
                    std::min(totalLayers - currentLayer, static_cast<size_t>(kMaxLength - 1) / 3);

            beginCommand(Command::SET_DISPLAY_REQUESTS, 1 + count * 3);
            write(displayRequestMask);
            for (size_t i = 0; i < count; i++) {
                write64(layers[currentLayer + i]);
                write(static_cast<int32_t>(layerRequestMasks[currentLayer + i]));
            }
            endCommand();

            currentLayer += count;
        }
    }

    static constexpr uint16_t kSetPresentFenceLength = 1;
    void setPresentFence(int presentFence) {
        beginCommand(Command::SET_PRESENT_FENCE, kSetPresentFenceLength);
        writeFence(presentFence);
        endCommand();
    }

    void setReleaseFences(const std::vector<int64_t>& layers,
                          const std::vector<int>& releaseFences) {
        size_t totalLayers = std::min(layers.size(), releaseFences.size());
        size_t currentLayer = 0;

        while (currentLayer < totalLayers) {
            size_t count =
                    std::min(totalLayers - currentLayer, static_cast<size_t>(kMaxLength) / 3);

            beginCommand(Command::SET_RELEASE_FENCES, count * 3);
            for (size_t i = 0; i < count; i++) {
                write64(layers[currentLayer + i]);
                writeFence(releaseFences[currentLayer + i]);
            }
            endCommand();

            currentLayer += count;
        }
    }

    static constexpr uint16_t kSetColorTransformLength = 17;
    void setColorTransform(const float* matrix, ColorTransform hint) {
        beginCommand(Command::SET_COLOR_TRANSFORM, kSetColorTransformLength);
        for (int i = 0; i < 16; i++) {
            writeFloat(matrix[i]);
        }
        writeSigned(static_cast<int32_t>(hint));
        endCommand();
    }

    void setClientTarget(uint32_t slot, const native_handle_t* target, int acquireFence,
                         Dataspace dataspace, const std::vector<Rect>& damage) {
        setClientTargetInternal(slot, target, acquireFence, static_cast<int32_t>(dataspace),
                                damage);
    }

    static constexpr uint16_t kSetOutputBufferLength = 3;
    void setOutputBuffer(uint32_t slot, const native_handle_t* buffer, int releaseFence) {
        beginCommand(Command::SET_OUTPUT_BUFFER, kSetOutputBufferLength);
        write(slot);
        writeHandle(buffer, true);
        writeFence(releaseFence);
        endCommand();
    }

    static constexpr uint16_t kValidateDisplayLength = 0;
    void validateDisplay() {
        beginCommand(Command::VALIDATE_DISPLAY, kValidateDisplayLength);
        endCommand();
    }

    static constexpr uint16_t kPresentOrValidateDisplayLength = 0;
    void presentOrvalidateDisplay() {
        beginCommand(Command::PRESENT_OR_VALIDATE_DISPLAY, kPresentOrValidateDisplayLength);
        endCommand();
    }

    static constexpr uint16_t kAcceptDisplayChangesLength = 0;
    void acceptDisplayChanges() {
        beginCommand(Command::ACCEPT_DISPLAY_CHANGES, kAcceptDisplayChangesLength);
        endCommand();
    }

    static constexpr uint16_t kPresentDisplayLength = 0;
    void presentDisplay() {
        beginCommand(Command::PRESENT_DISPLAY, kPresentDisplayLength);
        endCommand();
    }

    static constexpr uint16_t kSetLayerCursorPositionLength = 2;
    void setLayerCursorPosition(int32_t x, int32_t y) {
        beginCommand(Command::SET_LAYER_CURSOR_POSITION, kSetLayerCursorPositionLength);
        writeSigned(x);
        writeSigned(y);
        endCommand();
    }

    static constexpr uint16_t kSetLayerBufferLength = 3;
    void setLayerBuffer(uint32_t slot, const native_handle_t* buffer, int acquireFence) {
        beginCommand(Command::SET_LAYER_BUFFER, kSetLayerBufferLength);
        write(slot);
        writeHandle(buffer, true);
        writeFence(acquireFence);
        endCommand();
    }

    void setLayerSurfaceDamage(const std::vector<Rect>& damage) {
        bool doWrite = (damage.size() <= kMaxLength / 4);
        size_t length = (doWrite) ? damage.size() * 4 : 0;

        beginCommand(Command::SET_LAYER_SURFACE_DAMAGE, length);
        // When there are too many rectangles in the damage region and doWrite
        // is false, we write no rectangle at all which means the entire
        // layer is damaged.
        if (doWrite) {
            writeRegion(damage);
        }
        endCommand();
    }

    static constexpr uint16_t kSetLayerBlendModeLength = 1;
    void setLayerBlendMode(BlendMode mode) {
        beginCommand(Command::SET_LAYER_BLEND_MODE, kSetLayerBlendModeLength);
        writeSigned(static_cast<int32_t>(mode));
        endCommand();
    }

    static constexpr uint16_t kSetLayerColorLength = 1;
    void setLayerColor(Color color) {
        beginCommand(Command::SET_LAYER_COLOR, kSetLayerColorLength);
        writeColor(color);
        endCommand();
    }

    static constexpr uint16_t kSetLayerCompositionTypeLength = 1;
    void setLayerCompositionType(Composition type) {
        beginCommand(Command::SET_LAYER_COMPOSITION_TYPE, kSetLayerCompositionTypeLength);
        writeSigned(static_cast<int32_t>(type));
        endCommand();
    }

    static constexpr uint16_t kSetLayerDataspaceLength = 1;
    void setLayerDataspace(Dataspace dataspace) {
        setLayerDataspaceInternal(static_cast<int32_t>(dataspace));
    }

    static constexpr uint16_t kSetLayerDisplayFrameLength = 4;
    void setLayerDisplayFrame(const Rect& frame) {
        beginCommand(Command::SET_LAYER_DISPLAY_FRAME, kSetLayerDisplayFrameLength);
        writeRect(frame);
        endCommand();
    }

    static constexpr uint16_t kSetLayerPlaneAlphaLength = 1;
    void setLayerPlaneAlpha(float alpha) {
        beginCommand(Command::SET_LAYER_PLANE_ALPHA, kSetLayerPlaneAlphaLength);
        writeFloat(alpha);
        endCommand();
    }

    static constexpr uint16_t kSetLayerSidebandStreamLength = 1;
    void setLayerSidebandStream(const native_handle_t* stream) {
        beginCommand(Command::SET_LAYER_SIDEBAND_STREAM, kSetLayerSidebandStreamLength);
        writeHandle(stream);
        endCommand();
    }

    static constexpr uint16_t kSetLayerSourceCropLength = 4;
    void setLayerSourceCrop(const FRect& crop) {
        beginCommand(Command::SET_LAYER_SOURCE_CROP, kSetLayerSourceCropLength);
        writeFRect(crop);
        endCommand();
    }

    static constexpr uint16_t kSetLayerTransformLength = 1;
    void setLayerTransform(Transform transform) {
        beginCommand(Command::SET_LAYER_TRANSFORM, kSetLayerTransformLength);
        writeSigned(static_cast<int32_t>(transform));
        endCommand();
    }

    void setLayerVisibleRegion(const std::vector<Rect>& visible) {
        bool doWrite = (visible.size() <= kMaxLength / 4);
        size_t length = (doWrite) ? visible.size() * 4 : 0;

        beginCommand(Command::SET_LAYER_VISIBLE_REGION, length);
        // When there are too many rectangles in the visible region and
        // doWrite is false, we write no rectangle at all which means the
        // entire layer is visible.
        if (doWrite) {
            writeRegion(visible);
        }
        endCommand();
    }

    static constexpr uint16_t kSetLayerZOrderLength = 1;
    void setLayerZOrder(uint32_t z) {
        beginCommand(Command::SET_LAYER_Z_ORDER, kSetLayerZOrderLength);
        write(z);
        endCommand();
    }

    void setLayerPerFrameMetadata(const std::vector<PerFrameMetadata>& metadataVec) {
        beginCommand(Command::SET_LAYER_PER_FRAME_METADATA, metadataVec.size() * 2);
        for (const auto& metadata : metadataVec) {
            writeSigned(static_cast<int32_t>(metadata.key));
            writeFloat(metadata.value);
        }
        endCommand();
    }

    static constexpr uint16_t kSetLayerColorTransformLength = 16;
    void setLayerColorTransform(const float* matrix) {
        beginCommand(Command::SET_LAYER_COLOR_TRANSFORM, kSetLayerColorTransformLength);
        for (int i = 0; i < 16; i++) {
            writeFloat(matrix[i]);
        }
        endCommand();
    }

    void setLayerPerFrameMetadataBlobs(const std::vector<PerFrameMetadataBlob>& metadata) {
        // in units of uint32_t's
        size_t commandLength = 0;

        if (metadata.size() > std::numeric_limits<uint32_t>::max()) {
            LOG_FATAL("too many metadata blobs - dynamic metadata size is too large");
            return;
        }

        // space for numElements
        commandLength += 1;

        for (auto metadataBlob : metadata) {
            commandLength += 1;  // key of metadata blob
            commandLength += 1;  // size information of metadata blob

            // metadata content size
            size_t metadataSize = metadataBlob.blob.size() / sizeof(uint32_t);
            commandLength += metadataSize;
            commandLength +=
                    (metadataBlob.blob.size() - (metadataSize * sizeof(uint32_t)) > 0) ? 1 : 0;
        }

        if (commandLength > std::numeric_limits<uint16_t>::max()) {
            LOG_FATAL("dynamic metadata size is too large");
            return;
        }

        // Blobs are written as:
        // {numElements, key1, size1, blob1, key2, size2, blob2, key3, size3...}
        uint16_t length = static_cast<uint16_t>(commandLength);
        beginCommand(Command::SET_LAYER_PER_FRAME_METADATA_BLOBS, length);
        write(static_cast<uint32_t>(metadata.size()));
        for (auto metadataBlob : metadata) {
            writeSigned(static_cast<int32_t>(metadataBlob.key));
            write(static_cast<uint32_t>(metadataBlob.blob.size()));
            writeBlob(static_cast<uint32_t>(metadataBlob.blob.size()), metadataBlob.blob.data());
        }
        endCommand();
    }

    static constexpr uint16_t kSetLayerFloatColorLength = 4;
    void setLayerFloatColor(FloatColor color) {
        beginCommand(Command::SET_LAYER_FLOAT_COLOR, kSetLayerFloatColorLength);
        writeFloatColor(color);
        endCommand();
    }

    static constexpr uint16_t kSetClientTargetPropertyLength = 2;
    void setClientTargetProperty(const ClientTargetProperty& clientTargetProperty) {
        beginCommand(Command::SET_CLIENT_TARGET_PROPERTY, kSetClientTargetPropertyLength);
        writeSigned(static_cast<int32_t>(clientTargetProperty.pixelFormat));
        writeSigned(static_cast<int32_t>(clientTargetProperty.dataspace));
        endCommand();
    }

    void setLayerGenericMetadata(const std::string& key, const bool mandatory,
                                 const std::vector<uint8_t>& value) {
        const size_t commandSize = 3 + sizeToElements(key.size()) + sizeToElements(value.size());
        if (commandSize > std::numeric_limits<uint16_t>::max()) {
            LOG_FATAL("Too much generic metadata (%zu elements)", commandSize);
            return;
        }

        beginCommand(Command::SET_LAYER_GENERIC_METADATA, static_cast<uint16_t>(commandSize));
        write(key.size());
        writeBlob(key.size(), reinterpret_cast<const unsigned char*>(key.c_str()));
        write(mandatory);
        write(value.size());
        writeBlob(value.size(), value.data());
        endCommand();
    }

  protected:
    template <typename T>
    void beginCommand(T command, uint16_t length) {
        beginCommandBase(static_cast<Command>(command), length);
    }

    void setClientTargetInternal(uint32_t slot, const native_handle_t* target, int acquireFence,
                                 int32_t dataspace, const std::vector<Rect>& damage) {
        bool doWrite = (damage.size() <= (kMaxLength - 4) / 4);
        size_t length = 4 + ((doWrite) ? damage.size() * 4 : 0);

        beginCommand(Command::SET_CLIENT_TARGET, length);
        write(slot);
        writeHandle(target, true);
        writeFence(acquireFence);
        writeSigned(dataspace);
        // When there are too many rectangles in the damage region and doWrite
        // is false, we write no rectangle at all which means the entire
        // client target is damaged.
        if (doWrite) {
            writeRegion(damage);
        }
        endCommand();
    }

    void setLayerDataspaceInternal(int32_t dataspace) {
        beginCommand(Command::SET_LAYER_DATASPACE, kSetLayerDataspaceLength);
        writeSigned(dataspace);
        endCommand();
    }

    void beginCommandBase(Command command, uint16_t length) {
        if (mCommandEnd) {
            LOG_FATAL("endCommand was not called before command 0x%x", command);
        }

        growData(1 + length);
        write(static_cast<uint32_t>(command) | length);

        mCommandEnd = mDataWritten + length;
    }

    void endCommand() {
        if (!mCommandEnd) {
            LOG_FATAL("beginCommand was not called");
        } else if (mDataWritten > mCommandEnd) {
            LOG_FATAL("too much data written");
            mDataWritten = mCommandEnd;
        } else if (mDataWritten < mCommandEnd) {
            LOG_FATAL("too little data written");
            while (mDataWritten < mCommandEnd) {
                write(0);
            }
        }

        mCommandEnd = 0;
    }

    void write(uint32_t val) { mData[mDataWritten++] = val; }

    void writeSigned(int32_t val) { memcpy(&mData[mDataWritten++], &val, sizeof(val)); }

    void writeFloat(float val) { memcpy(&mData[mDataWritten++], &val, sizeof(val)); }

    void write64(uint64_t val) {
        uint32_t lo = static_cast<uint32_t>(val & 0xffffffff);
        uint32_t hi = static_cast<uint32_t>(val >> 32);
        write(lo);
        write(hi);
    }

    void writeRect(const Rect& rect) {
        writeSigned(rect.left);
        writeSigned(rect.top);
        writeSigned(rect.right);
        writeSigned(rect.bottom);
    }

    void writeRegion(const std::vector<Rect>& region) {
        for (const auto& rect : region) {
            writeRect(rect);
        }
    }

    void writeFRect(const FRect& rect) {
        writeFloat(rect.left);
        writeFloat(rect.top);
        writeFloat(rect.right);
        writeFloat(rect.bottom);
    }

    void writeColor(const Color& color) {
        write((color.r << 0) | (color.g << 8) | (color.b << 16) | (color.a << 24));
    }

    void writeFloatColor(const FloatColor& color) {
        writeFloat(color.r);
        writeFloat(color.g);
        writeFloat(color.b);
        writeFloat(color.a);
    }

    void writeBlob(uint32_t length, const unsigned char* blob) {
        memcpy(&mData[mDataWritten], blob, length);
        uint32_t numElements = length / 4;
        mDataWritten += numElements;
        mDataWritten += (length - (numElements * 4) > 0) ? 1 : 0;
    }

    // ownership of handle is not transferred
    void writeHandle(const native_handle_t* handle, bool useCache) {
        if (!handle) {
            writeSigned(
                    static_cast<int32_t>((useCache) ? HandleIndex::CACHED : HandleIndex::EMPTY));
            return;
        }

        mDataHandles.push_back(::android::dupToAidl(handle));
        writeSigned(mDataHandles.size() - 1);
    }

    void writeHandle(const native_handle_t* handle) { writeHandle(handle, false); }

    // ownership of fence is transferred
    void writeFence(int fence) {
        native_handle_t* handle = nullptr;
        if (fence >= 0) {
            handle = getTemporaryHandle(1, 0);
            if (handle) {
                handle->data[0] = fence;
            } else {
                ALOGW("failed to get temporary handle for fence %d", fence);
                sync_wait(fence, -1);
                close(fence);
            }
        }

        writeHandle(handle);
    }

    native_handle_t* getTemporaryHandle(int numFds, int numInts) {
        native_handle_t* handle = native_handle_create(numFds, numInts);
        if (handle) {
            mTemporaryHandles.push_back(handle);
        }
        return handle;
    }

    static constexpr uint16_t kMaxLength = std::numeric_limits<uint16_t>::max();

    std::unique_ptr<int32_t[]> mData;
    uint32_t mDataWritten;

  private:
    void growData(uint32_t grow) {
        uint32_t newWritten = mDataWritten + grow;
        if (newWritten < mDataWritten) {
            LOG_ALWAYS_FATAL("buffer overflowed; data written %" PRIu32 ", growing by %" PRIu32,
                             mDataWritten, grow);
        }

        if (newWritten <= mDataMaxSize) {
            return;
        }

        uint32_t newMaxSize = mDataMaxSize << 1;
        if (newMaxSize < newWritten) {
            newMaxSize = newWritten;
        }

        auto newData = std::make_unique<int32_t[]>(newMaxSize);
        std::copy_n(mData.get(), mDataWritten, newData.get());
        mDataMaxSize = newMaxSize;
        mData = std::move(newData);
    }

    uint32_t sizeToElements(uint32_t size) { return (size + 3) / 4; }

    uint32_t mDataMaxSize;
    // end offset of the current command
    uint32_t mCommandEnd;

    std::vector<NativeHandle> mDataHandles;
    std::vector<native_handle_t*> mTemporaryHandles;

    std::unique_ptr<CommandQueueType> mQueue;
};

// This class helps parse a command queue.  Note that all sizes/lengths are in
// units of uint32_t's.
class CommandReaderBase {
  public:
    CommandReaderBase() : mDataMaxSize(0) { reset(); }

    bool setMQDescriptor(const DescriptorType& descriptor) {
        mQueue = std::make_unique<CommandQueueType>(descriptor, false);
        if (mQueue->isValid()) {
            return true;
        } else {
            mQueue = nullptr;
            return false;
        }
    }

    bool readQueue(int32_t commandLength, std::vector<NativeHandle> commandHandles) {
        if (!mQueue) {
            return false;
        }

        auto quantumCount = mQueue->getQuantumCount();
        if (mDataMaxSize < quantumCount) {
            mDataMaxSize = quantumCount;
            mData = std::make_unique<int32_t[]>(mDataMaxSize);
        }

        if (commandLength > mDataMaxSize || !mQueue->read(mData.get(), commandLength)) {
            ALOGE("failed to read commands from message queue");
            return false;
        }

        mDataSize = commandLength;
        mDataRead = 0;
        mCommandBegin = 0;
        mCommandEnd = 0;
        mDataHandles = std::move(commandHandles);
        return true;
    }

    void reset() {
        mDataSize = 0;
        mDataRead = 0;
        mCommandBegin = 0;
        mCommandEnd = 0;
        mDataHandles.clear();
    }

  protected:
    template <typename T>
    bool beginCommand(T* outCommand, uint16_t* outLength) {
        return beginCommandBase(reinterpret_cast<Command*>(outCommand), outLength);
    }

    bool isEmpty() const { return (mDataRead >= mDataSize); }

    bool beginCommandBase(Command* outCommand, uint16_t* outLength) {
        if (mCommandEnd) {
            LOG_FATAL("endCommand was not called for last command");
        }

        constexpr uint32_t opcode_mask = static_cast<uint32_t>(Command::OPCODE_MASK);
        constexpr uint32_t length_mask = static_cast<uint32_t>(Command::LENGTH_MASK);

        uint32_t val = read();
        *outCommand = static_cast<Command>(val & opcode_mask);
        *outLength = static_cast<uint16_t>(val & length_mask);

        if (mDataRead + *outLength > mDataSize) {
            ALOGE("command 0x%x has invalid command length %" PRIu16, *outCommand, *outLength);
            // undo the read() above
            mDataRead--;
            return false;
        }

        mCommandEnd = mDataRead + *outLength;

        return true;
    }

    void endCommand() {
        if (!mCommandEnd) {
            LOG_FATAL("beginCommand was not called");
        } else if (mDataRead > mCommandEnd) {
            LOG_FATAL("too much data read");
            mDataRead = mCommandEnd;
        } else if (mDataRead < mCommandEnd) {
            LOG_FATAL("too little data read");
            mDataRead = mCommandEnd;
        }

        mCommandBegin = mCommandEnd;
        mCommandEnd = 0;
    }

    uint32_t getCommandLoc() const { return mCommandBegin; }

    uint32_t read() { return mData[mDataRead++]; }

    int32_t readSigned() {
        int32_t val;
        memcpy(&val, &mData[mDataRead++], sizeof(val));
        return val;
    }

    float readFloat() {
        float val;
        memcpy(&val, &mData[mDataRead++], sizeof(val));
        return val;
    }

    uint64_t read64() {
        uint32_t lo = read();
        uint32_t hi = read();
        return (static_cast<uint64_t>(hi) << 32) | lo;
    }

    Color readColor() {
        uint32_t val = read();
        return Color{
                static_cast<int8_t>((val >> 0) & 0xff),
                static_cast<int8_t>((val >> 8) & 0xff),
                static_cast<int8_t>((val >> 16) & 0xff),
                static_cast<int8_t>((val >> 24) & 0xff),
        };
    }

    // ownership of handle is not transferred
    const native_handle_t* readHandle(bool* outUseCache) {
        const native_handle_t* handle = nullptr;

        int32_t index = readSigned();
        switch (index) {
            case static_cast<int32_t>(HandleIndex::EMPTY):
                *outUseCache = false;
                break;
            case static_cast<int32_t>(HandleIndex::CACHED):
                *outUseCache = true;
                break;
            default:
                if (static_cast<size_t>(index) < mDataHandles.size()) {
                    handle = ::android::makeFromAidl(mDataHandles[index]);
                } else {
                    ALOGE("invalid handle index %zu", static_cast<size_t>(index));
                }
                *outUseCache = false;
                break;
        }

        return handle;
    }

    const native_handle_t* readHandle() {
        bool useCache;
        return readHandle(&useCache);
    }

    // ownership of fence is transferred
    int readFence() {
        auto handle = readHandle();
        if (!handle || handle->numFds == 0) {
            return -1;
        }

        if (handle->numFds != 1) {
            ALOGE("invalid fence handle with %d fds", handle->numFds);
            return -1;
        }

        int fd = dup(handle->data[0]);
        if (fd < 0) {
            ALOGW("failed to dup fence %d", handle->data[0]);
            sync_wait(handle->data[0], -1);
            fd = -1;
        }

        return fd;
    }

    std::unique_ptr<int32_t[]> mData;
    uint32_t mDataRead;

  private:
    std::unique_ptr<CommandQueueType> mQueue;
    uint32_t mDataMaxSize;

    uint32_t mDataSize;

    // begin/end offsets of the current command
    uint32_t mCommandBegin;
    uint32_t mCommandEnd;

    std::vector<NativeHandle> mDataHandles;
};

}  // namespace aidl::android::hardware::graphics::composer3
