/*
 * Copyright (C) 2019 The Android Open Source Project
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

#define LOG_TAG "neuralnetworks_hidl_hal_test"

#include "VtsHalNeuralnetworks.h"

#include "Callbacks.h"
#include "GeneratedTestHarness.h"
#include "TestHarness.h"
#include "Utils.h"

#include <android-base/logging.h>
#include <android/hidl/memory/1.0/IMemory.h>
#include <hidlmemory/mapping.h>
#include <cstdio>
#include <cstdlib>
#include <random>

#include <gtest/gtest.h>

namespace android {
namespace hardware {
namespace neuralnetworks {
namespace V1_2 {
namespace vts {
namespace functional {

using ::android::hardware::neuralnetworks::V1_2::implementation::ExecutionCallback;
using ::android::hardware::neuralnetworks::V1_2::implementation::PreparedModelCallback;
using ::android::nn::allocateSharedMemory;
using ::test_helper::MixedTypedExample;

namespace {

// In frameworks/ml/nn/runtime/tests/generated/, creates a hidl model of mobilenet.
#include "examples/mobilenet_224_gender_basic_fixed.example.cpp"
#include "vts_models/mobilenet_224_gender_basic_fixed.model.cpp"

// Prevent the compiler from complaining about an otherwise unused function.
[[maybe_unused]] auto dummy_createTestModel = createTestModel_dynamic_output_shape;
[[maybe_unused]] auto dummy_get_examples = get_examples_dynamic_output_shape;

enum class AccessMode { READ_WRITE, READ_ONLY, WRITE_ONLY };

// Creates cache handles based on provided file groups.
// The outer vector corresponds to handles and the inner vector is for fds held by each handle.
void createCacheHandles(const std::vector<std::vector<std::string>>& fileGroups,
                        const std::vector<AccessMode>& mode, hidl_vec<hidl_handle>* handles) {
    handles->resize(fileGroups.size());
    for (uint32_t i = 0; i < fileGroups.size(); i++) {
        std::vector<int> fds;
        for (const auto& file : fileGroups[i]) {
            int fd;
            if (mode[i] == AccessMode::READ_ONLY) {
                fd = open(file.c_str(), O_RDONLY);
            } else if (mode[i] == AccessMode::WRITE_ONLY) {
                fd = open(file.c_str(), O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
            } else if (mode[i] == AccessMode::READ_WRITE) {
                fd = open(file.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
            } else {
                FAIL();
            }
            ASSERT_GE(fd, 0);
            fds.push_back(fd);
        }
        native_handle_t* cacheNativeHandle = native_handle_create(fds.size(), 0);
        ASSERT_NE(cacheNativeHandle, nullptr);
        std::copy(fds.begin(), fds.end(), &cacheNativeHandle->data[0]);
        (*handles)[i].setTo(cacheNativeHandle, /*shouldOwn=*/true);
    }
}

void createCacheHandles(const std::vector<std::vector<std::string>>& fileGroups, AccessMode mode,
                        hidl_vec<hidl_handle>* handles) {
    createCacheHandles(fileGroups, std::vector<AccessMode>(fileGroups.size(), mode), handles);
}

}  // namespace

// Tag for the compilation caching tests.
class CompilationCachingTest : public NeuralnetworksHidlTest {
  protected:
    void SetUp() override {
        NeuralnetworksHidlTest::SetUp();
        ASSERT_NE(device.get(), nullptr);

        // Create cache directory. The cache directory and a temporary cache file is always created
        // to test the behavior of prepareModelFromCache, even when caching is not supported.
        char cacheDirTemp[] = "/data/local/tmp/TestCompilationCachingXXXXXX";
        char* cacheDir = mkdtemp(cacheDirTemp);
        ASSERT_NE(cacheDir, nullptr);
        mCacheDir = cacheDir;
        mCacheDir.push_back('/');

        Return<void> ret = device->getNumberOfCacheFilesNeeded(
                [this](ErrorStatus status, uint32_t numModelCache, uint32_t numDataCache) {
                    EXPECT_EQ(ErrorStatus::NONE, status);
                    mNumModelCache = numModelCache;
                    mNumDataCache = numDataCache;
                });
        EXPECT_TRUE(ret.isOk());
        mIsCachingSupported = mNumModelCache > 0 || mNumDataCache > 0;

        // Create empty cache files.
        mTmpCache = mCacheDir + "tmp";
        for (uint32_t i = 0; i < mNumModelCache; i++) {
            mModelCache.push_back({mCacheDir + "model" + std::to_string(i)});
        }
        for (uint32_t i = 0; i < mNumDataCache; i++) {
            mDataCache.push_back({mCacheDir + "data" + std::to_string(i)});
        }
        // Dummy handles, use AccessMode::WRITE_ONLY for createCacheHandles to create files.
        hidl_vec<hidl_handle> modelHandle, dataHandle, tmpHandle;
        createCacheHandles(mModelCache, AccessMode::WRITE_ONLY, &modelHandle);
        createCacheHandles(mDataCache, AccessMode::WRITE_ONLY, &dataHandle);
        createCacheHandles({{mTmpCache}}, AccessMode::WRITE_ONLY, &tmpHandle);

        if (!mIsCachingSupported) {
            LOG(INFO) << "NN VTS: Early termination of test because vendor service does not "
                         "support compilation caching.";
            std::cout << "[          ]   Early termination of test because vendor service does not "
                         "support compilation caching."
                      << std::endl;
        }
    }

    void TearDown() override {
        // The tmp directory is only removed when the driver reports caching not supported,
        // otherwise it is kept for debugging purpose.
        if (!mIsCachingSupported) {
            remove(mTmpCache.c_str());
            rmdir(mCacheDir.c_str());
        }
        NeuralnetworksHidlTest::TearDown();
    }

    void saveModelToCache(const V1_2::Model& model, const hidl_vec<hidl_handle>& modelCache,
                          const hidl_vec<hidl_handle>& dataCache, bool* supported,
                          sp<IPreparedModel>* preparedModel = nullptr) {
        if (preparedModel != nullptr) *preparedModel = nullptr;

        // See if service can handle model.
        bool fullySupportsModel = false;
        Return<void> supportedCall = device->getSupportedOperations_1_2(
                model,
                [&fullySupportsModel, &model](ErrorStatus status, const hidl_vec<bool>& supported) {
                    ASSERT_EQ(ErrorStatus::NONE, status);
                    ASSERT_EQ(supported.size(), model.operations.size());
                    fullySupportsModel = std::all_of(supported.begin(), supported.end(),
                                                     [](bool valid) { return valid; });
                });
        ASSERT_TRUE(supportedCall.isOk());
        *supported = fullySupportsModel;
        if (!fullySupportsModel) return;

        // Launch prepare model.
        sp<PreparedModelCallback> preparedModelCallback = new PreparedModelCallback();
        ASSERT_NE(nullptr, preparedModelCallback.get());
        hidl_array<uint8_t, sizeof(mToken)> cacheToken(mToken);
        Return<ErrorStatus> prepareLaunchStatus =
                device->prepareModel_1_2(model, ExecutionPreference::FAST_SINGLE_ANSWER, modelCache,
                                         dataCache, cacheToken, preparedModelCallback);
        ASSERT_TRUE(prepareLaunchStatus.isOk());
        ASSERT_EQ(static_cast<ErrorStatus>(prepareLaunchStatus), ErrorStatus::NONE);

        // Retrieve prepared model.
        preparedModelCallback->wait();
        ASSERT_EQ(preparedModelCallback->getStatus(), ErrorStatus::NONE);
        if (preparedModel != nullptr) {
            *preparedModel =
                    V1_2::IPreparedModel::castFrom(preparedModelCallback->getPreparedModel())
                            .withDefault(nullptr);
        }
    }

    bool checkEarlyTermination(ErrorStatus status) {
        if (status == ErrorStatus::GENERAL_FAILURE) {
            LOG(INFO) << "NN VTS: Early termination of test because vendor service cannot "
                         "save the prepared model that it does not support.";
            std::cout << "[          ]   Early termination of test because vendor service cannot "
                         "save the prepared model that it does not support."
                      << std::endl;
            return true;
        }
        return false;
    }

    bool checkEarlyTermination(bool supported) {
        if (!supported) {
            LOG(INFO) << "NN VTS: Early termination of test because vendor service cannot "
                         "prepare model that it does not support.";
            std::cout << "[          ]   Early termination of test because vendor service cannot "
                         "prepare model that it does not support."
                      << std::endl;
            return true;
        }
        return false;
    }

    void prepareModelFromCache(const hidl_vec<hidl_handle>& modelCache,
                               const hidl_vec<hidl_handle>& dataCache,
                               sp<IPreparedModel>* preparedModel, ErrorStatus* status) {
        // Launch prepare model from cache.
        sp<PreparedModelCallback> preparedModelCallback = new PreparedModelCallback();
        ASSERT_NE(nullptr, preparedModelCallback.get());
        hidl_array<uint8_t, sizeof(mToken)> cacheToken(mToken);
        Return<ErrorStatus> prepareLaunchStatus = device->prepareModelFromCache(
                modelCache, dataCache, cacheToken, preparedModelCallback);
        ASSERT_TRUE(prepareLaunchStatus.isOk());
        if (static_cast<ErrorStatus>(prepareLaunchStatus) != ErrorStatus::NONE) {
            *preparedModel = nullptr;
            *status = static_cast<ErrorStatus>(prepareLaunchStatus);
            return;
        }

        // Retrieve prepared model.
        preparedModelCallback->wait();
        *status = preparedModelCallback->getStatus();
        *preparedModel = V1_2::IPreparedModel::castFrom(preparedModelCallback->getPreparedModel())
                                 .withDefault(nullptr);
    }

    // Absolute path to the temporary cache directory.
    std::string mCacheDir;

    // Groups of file paths for model and data cache in the tmp cache directory, initialized with
    // outer_size = mNum{Model|Data}Cache, inner_size = 1. The outer vector corresponds to handles
    // and the inner vector is for fds held by each handle.
    std::vector<std::vector<std::string>> mModelCache;
    std::vector<std::vector<std::string>> mDataCache;

    // A separate temporary file path in the tmp cache directory.
    std::string mTmpCache;

    uint8_t mToken[static_cast<uint32_t>(Constant::BYTE_SIZE_OF_CACHE_TOKEN)] = {};
    uint32_t mNumModelCache;
    uint32_t mNumDataCache;
    uint32_t mIsCachingSupported;
};

TEST_F(CompilationCachingTest, CacheSavingAndRetrieval) {
    // Create test HIDL model and compile.
    Model testModel = createTestModel();
    sp<IPreparedModel> preparedModel = nullptr;

    // Save the compilation to cache.
    {
        bool supported;
        hidl_vec<hidl_handle> modelCache, dataCache;
        createCacheHandles(mModelCache, AccessMode::READ_WRITE, &modelCache);
        createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
        saveModelToCache(testModel, modelCache, dataCache, &supported);
        if (checkEarlyTermination(supported)) return;
    }

    // Retrieve preparedModel from cache.
    {
        preparedModel = nullptr;
        ErrorStatus status;
        hidl_vec<hidl_handle> modelCache, dataCache;
        createCacheHandles(mModelCache, AccessMode::READ_WRITE, &modelCache);
        createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
        prepareModelFromCache(modelCache, dataCache, &preparedModel, &status);
        if (!mIsCachingSupported) {
            ASSERT_EQ(status, ErrorStatus::GENERAL_FAILURE);
            ASSERT_EQ(preparedModel, nullptr);
            return;
        } else if (checkEarlyTermination(status)) {
            ASSERT_EQ(preparedModel, nullptr);
            return;
        } else {
            ASSERT_EQ(status, ErrorStatus::NONE);
            ASSERT_NE(preparedModel, nullptr);
        }
    }

    // Execute and verify results.
    generated_tests::EvaluatePreparedModel(preparedModel, [](int) { return false; }, get_examples(),
                                           testModel.relaxComputationFloat32toFloat16,
                                           /*testDynamicOutputShape=*/false);
}

TEST_F(CompilationCachingTest, CacheSavingAndRetrievalNonZeroOffset) {
    // Create test HIDL model and compile.
    Model testModel = createTestModel();
    sp<IPreparedModel> preparedModel = nullptr;

    // Save the compilation to cache.
    {
        bool supported;
        hidl_vec<hidl_handle> modelCache, dataCache;
        createCacheHandles(mModelCache, AccessMode::READ_WRITE, &modelCache);
        createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
        uint8_t dummyBytes[] = {0, 0};
        // Write a dummy integer to the cache.
        // The driver should be able to handle non-empty cache and non-zero fd offset.
        for (uint32_t i = 0; i < modelCache.size(); i++) {
            ASSERT_EQ(write(modelCache[i].getNativeHandle()->data[0], &dummyBytes,
                            sizeof(dummyBytes)),
                      sizeof(dummyBytes));
        }
        for (uint32_t i = 0; i < dataCache.size(); i++) {
            ASSERT_EQ(
                    write(dataCache[i].getNativeHandle()->data[0], &dummyBytes, sizeof(dummyBytes)),
                    sizeof(dummyBytes));
        }
        saveModelToCache(testModel, modelCache, dataCache, &supported);
        if (checkEarlyTermination(supported)) return;
    }

    // Retrieve preparedModel from cache.
    {
        preparedModel = nullptr;
        ErrorStatus status;
        hidl_vec<hidl_handle> modelCache, dataCache;
        createCacheHandles(mModelCache, AccessMode::READ_WRITE, &modelCache);
        createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
        uint8_t dummyByte = 0;
        // Advance the offset of each handle by one byte.
        // The driver should be able to handle non-zero fd offset.
        for (uint32_t i = 0; i < modelCache.size(); i++) {
            ASSERT_GE(read(modelCache[i].getNativeHandle()->data[0], &dummyByte, 1), 0);
        }
        for (uint32_t i = 0; i < dataCache.size(); i++) {
            ASSERT_GE(read(dataCache[i].getNativeHandle()->data[0], &dummyByte, 1), 0);
        }
        prepareModelFromCache(modelCache, dataCache, &preparedModel, &status);
        if (!mIsCachingSupported) {
            ASSERT_EQ(status, ErrorStatus::GENERAL_FAILURE);
            ASSERT_EQ(preparedModel, nullptr);
            return;
        } else if (checkEarlyTermination(status)) {
            ASSERT_EQ(preparedModel, nullptr);
            return;
        } else {
            ASSERT_EQ(status, ErrorStatus::NONE);
            ASSERT_NE(preparedModel, nullptr);
        }
    }

    // Execute and verify results.
    generated_tests::EvaluatePreparedModel(preparedModel, [](int) { return false; }, get_examples(),
                                           testModel.relaxComputationFloat32toFloat16,
                                           /*testDynamicOutputShape=*/false);
}

TEST_F(CompilationCachingTest, SaveToCacheInvalidNumCache) {
    // Create test HIDL model and compile.
    Model testModel = createTestModel();

    // Test with number of model cache files greater than mNumModelCache.
    {
        bool supported;
        hidl_vec<hidl_handle> modelCache, dataCache;
        // Pass an additional cache file for model cache.
        mModelCache.push_back({mTmpCache});
        createCacheHandles(mModelCache, AccessMode::READ_WRITE, &modelCache);
        createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
        mModelCache.pop_back();
        sp<IPreparedModel> preparedModel = nullptr;
        saveModelToCache(testModel, modelCache, dataCache, &supported, &preparedModel);
        if (checkEarlyTermination(supported)) return;
        ASSERT_NE(preparedModel, nullptr);
        // Execute and verify results.
        generated_tests::EvaluatePreparedModel(preparedModel, [](int) { return false; },
                                               get_examples(),
                                               testModel.relaxComputationFloat32toFloat16,
                                               /*testDynamicOutputShape=*/false);
        // Check if prepareModelFromCache fails.
        preparedModel = nullptr;
        ErrorStatus status;
        prepareModelFromCache(modelCache, dataCache, &preparedModel, &status);
        if (status != ErrorStatus::INVALID_ARGUMENT) {
            ASSERT_EQ(status, ErrorStatus::GENERAL_FAILURE);
        }
        ASSERT_EQ(preparedModel, nullptr);
    }

    // Test with number of model cache files smaller than mNumModelCache.
    if (mModelCache.size() > 0) {
        bool supported;
        hidl_vec<hidl_handle> modelCache, dataCache;
        // Pop out the last cache file.
        auto tmp = mModelCache.back();
        mModelCache.pop_back();
        createCacheHandles(mModelCache, AccessMode::READ_WRITE, &modelCache);
        createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
        mModelCache.push_back(tmp);
        sp<IPreparedModel> preparedModel = nullptr;
        saveModelToCache(testModel, modelCache, dataCache, &supported, &preparedModel);
        if (checkEarlyTermination(supported)) return;
        ASSERT_NE(preparedModel, nullptr);
        // Execute and verify results.
        generated_tests::EvaluatePreparedModel(preparedModel, [](int) { return false; },
                                               get_examples(),
                                               testModel.relaxComputationFloat32toFloat16,
                                               /*testDynamicOutputShape=*/false);
        // Check if prepareModelFromCache fails.
        preparedModel = nullptr;
        ErrorStatus status;
        prepareModelFromCache(modelCache, dataCache, &preparedModel, &status);
        if (status != ErrorStatus::INVALID_ARGUMENT) {
            ASSERT_EQ(status, ErrorStatus::GENERAL_FAILURE);
        }
        ASSERT_EQ(preparedModel, nullptr);
    }

    // Test with number of data cache files greater than mNumDataCache.
    {
        bool supported;
        hidl_vec<hidl_handle> modelCache, dataCache;
        // Pass an additional cache file for data cache.
        mDataCache.push_back({mTmpCache});
        createCacheHandles(mModelCache, AccessMode::READ_WRITE, &modelCache);
        createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
        mDataCache.pop_back();
        sp<IPreparedModel> preparedModel = nullptr;
        saveModelToCache(testModel, modelCache, dataCache, &supported, &preparedModel);
        if (checkEarlyTermination(supported)) return;
        ASSERT_NE(preparedModel, nullptr);
        // Execute and verify results.
        generated_tests::EvaluatePreparedModel(preparedModel, [](int) { return false; },
                                               get_examples(),
                                               testModel.relaxComputationFloat32toFloat16,
                                               /*testDynamicOutputShape=*/false);
        // Check if prepareModelFromCache fails.
        preparedModel = nullptr;
        ErrorStatus status;
        prepareModelFromCache(modelCache, dataCache, &preparedModel, &status);
        if (status != ErrorStatus::INVALID_ARGUMENT) {
            ASSERT_EQ(status, ErrorStatus::GENERAL_FAILURE);
        }
        ASSERT_EQ(preparedModel, nullptr);
    }

    // Test with number of data cache files smaller than mNumDataCache.
    if (mDataCache.size() > 0) {
        bool supported;
        hidl_vec<hidl_handle> modelCache, dataCache;
        // Pop out the last cache file.
        auto tmp = mDataCache.back();
        mDataCache.pop_back();
        createCacheHandles(mModelCache, AccessMode::READ_WRITE, &modelCache);
        createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
        mDataCache.push_back(tmp);
        sp<IPreparedModel> preparedModel = nullptr;
        saveModelToCache(testModel, modelCache, dataCache, &supported, &preparedModel);
        if (checkEarlyTermination(supported)) return;
        ASSERT_NE(preparedModel, nullptr);
        // Execute and verify results.
        generated_tests::EvaluatePreparedModel(preparedModel, [](int) { return false; },
                                               get_examples(),
                                               testModel.relaxComputationFloat32toFloat16,
                                               /*testDynamicOutputShape=*/false);
        // Check if prepareModelFromCache fails.
        preparedModel = nullptr;
        ErrorStatus status;
        prepareModelFromCache(modelCache, dataCache, &preparedModel, &status);
        if (status != ErrorStatus::INVALID_ARGUMENT) {
            ASSERT_EQ(status, ErrorStatus::GENERAL_FAILURE);
        }
        ASSERT_EQ(preparedModel, nullptr);
    }
}

TEST_F(CompilationCachingTest, PrepareModelFromCacheInvalidNumCache) {
    // Create test HIDL model and compile.
    Model testModel = createTestModel();

    // Save the compilation to cache.
    {
        bool supported;
        hidl_vec<hidl_handle> modelCache, dataCache;
        createCacheHandles(mModelCache, AccessMode::READ_WRITE, &modelCache);
        createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
        saveModelToCache(testModel, modelCache, dataCache, &supported);
        if (checkEarlyTermination(supported)) return;
    }

    // Test with number of model cache files greater than mNumModelCache.
    {
        sp<IPreparedModel> preparedModel = nullptr;
        ErrorStatus status;
        hidl_vec<hidl_handle> modelCache, dataCache;
        mModelCache.push_back({mTmpCache});
        createCacheHandles(mModelCache, AccessMode::READ_WRITE, &modelCache);
        createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
        mModelCache.pop_back();
        prepareModelFromCache(modelCache, dataCache, &preparedModel, &status);
        if (status != ErrorStatus::GENERAL_FAILURE) {
            ASSERT_EQ(status, ErrorStatus::INVALID_ARGUMENT);
        }
        ASSERT_EQ(preparedModel, nullptr);
    }

    // Test with number of model cache files smaller than mNumModelCache.
    if (mModelCache.size() > 0) {
        sp<IPreparedModel> preparedModel = nullptr;
        ErrorStatus status;
        hidl_vec<hidl_handle> modelCache, dataCache;
        auto tmp = mModelCache.back();
        mModelCache.pop_back();
        createCacheHandles(mModelCache, AccessMode::READ_WRITE, &modelCache);
        createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
        mModelCache.push_back(tmp);
        prepareModelFromCache(modelCache, dataCache, &preparedModel, &status);
        if (status != ErrorStatus::GENERAL_FAILURE) {
            ASSERT_EQ(status, ErrorStatus::INVALID_ARGUMENT);
        }
        ASSERT_EQ(preparedModel, nullptr);
    }

    // Test with number of data cache files greater than mNumDataCache.
    {
        sp<IPreparedModel> preparedModel = nullptr;
        ErrorStatus status;
        hidl_vec<hidl_handle> modelCache, dataCache;
        mDataCache.push_back({mTmpCache});
        createCacheHandles(mModelCache, AccessMode::READ_WRITE, &modelCache);
        createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
        mDataCache.pop_back();
        prepareModelFromCache(modelCache, dataCache, &preparedModel, &status);
        if (status != ErrorStatus::GENERAL_FAILURE) {
            ASSERT_EQ(status, ErrorStatus::INVALID_ARGUMENT);
        }
        ASSERT_EQ(preparedModel, nullptr);
    }

    // Test with number of data cache files smaller than mNumDataCache.
    if (mDataCache.size() > 0) {
        sp<IPreparedModel> preparedModel = nullptr;
        ErrorStatus status;
        hidl_vec<hidl_handle> modelCache, dataCache;
        auto tmp = mDataCache.back();
        mDataCache.pop_back();
        createCacheHandles(mModelCache, AccessMode::READ_WRITE, &modelCache);
        createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
        mDataCache.push_back(tmp);
        prepareModelFromCache(modelCache, dataCache, &preparedModel, &status);
        if (status != ErrorStatus::GENERAL_FAILURE) {
            ASSERT_EQ(status, ErrorStatus::INVALID_ARGUMENT);
        }
        ASSERT_EQ(preparedModel, nullptr);
    }
}

TEST_F(CompilationCachingTest, SaveToCacheInvalidNumFd) {
    // Create test HIDL model and compile.
    Model testModel = createTestModel();

    // Go through each handle in model cache, test with NumFd greater than 1.
    for (uint32_t i = 0; i < mNumModelCache; i++) {
        bool supported;
        hidl_vec<hidl_handle> modelCache, dataCache;
        // Pass an invalid number of fds for handle i.
        mModelCache[i].push_back(mTmpCache);
        createCacheHandles(mModelCache, AccessMode::READ_WRITE, &modelCache);
        createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
        mModelCache[i].pop_back();
        sp<IPreparedModel> preparedModel = nullptr;
        saveModelToCache(testModel, modelCache, dataCache, &supported, &preparedModel);
        if (checkEarlyTermination(supported)) return;
        ASSERT_NE(preparedModel, nullptr);
        // Execute and verify results.
        generated_tests::EvaluatePreparedModel(preparedModel, [](int) { return false; },
                                               get_examples(),
                                               testModel.relaxComputationFloat32toFloat16,
                                               /*testDynamicOutputShape=*/false);
        // Check if prepareModelFromCache fails.
        preparedModel = nullptr;
        ErrorStatus status;
        prepareModelFromCache(modelCache, dataCache, &preparedModel, &status);
        if (status != ErrorStatus::INVALID_ARGUMENT) {
            ASSERT_EQ(status, ErrorStatus::GENERAL_FAILURE);
        }
        ASSERT_EQ(preparedModel, nullptr);
    }

    // Go through each handle in model cache, test with NumFd equal to 0.
    for (uint32_t i = 0; i < mNumModelCache; i++) {
        bool supported;
        hidl_vec<hidl_handle> modelCache, dataCache;
        // Pass an invalid number of fds for handle i.
        auto tmp = mModelCache[i].back();
        mModelCache[i].pop_back();
        createCacheHandles(mModelCache, AccessMode::READ_WRITE, &modelCache);
        createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
        mModelCache[i].push_back(tmp);
        sp<IPreparedModel> preparedModel = nullptr;
        saveModelToCache(testModel, modelCache, dataCache, &supported, &preparedModel);
        if (checkEarlyTermination(supported)) return;
        ASSERT_NE(preparedModel, nullptr);
        // Execute and verify results.
        generated_tests::EvaluatePreparedModel(preparedModel, [](int) { return false; },
                                               get_examples(),
                                               testModel.relaxComputationFloat32toFloat16,
                                               /*testDynamicOutputShape=*/false);
        // Check if prepareModelFromCache fails.
        preparedModel = nullptr;
        ErrorStatus status;
        prepareModelFromCache(modelCache, dataCache, &preparedModel, &status);
        if (status != ErrorStatus::INVALID_ARGUMENT) {
            ASSERT_EQ(status, ErrorStatus::GENERAL_FAILURE);
        }
        ASSERT_EQ(preparedModel, nullptr);
    }

    // Go through each handle in data cache, test with NumFd greater than 1.
    for (uint32_t i = 0; i < mNumDataCache; i++) {
        bool supported;
        hidl_vec<hidl_handle> modelCache, dataCache;
        // Pass an invalid number of fds for handle i.
        mDataCache[i].push_back(mTmpCache);
        createCacheHandles(mModelCache, AccessMode::READ_WRITE, &modelCache);
        createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
        mDataCache[i].pop_back();
        sp<IPreparedModel> preparedModel = nullptr;
        saveModelToCache(testModel, modelCache, dataCache, &supported, &preparedModel);
        if (checkEarlyTermination(supported)) return;
        ASSERT_NE(preparedModel, nullptr);
        // Execute and verify results.
        generated_tests::EvaluatePreparedModel(preparedModel, [](int) { return false; },
                                               get_examples(),
                                               testModel.relaxComputationFloat32toFloat16,
                                               /*testDynamicOutputShape=*/false);
        // Check if prepareModelFromCache fails.
        preparedModel = nullptr;
        ErrorStatus status;
        prepareModelFromCache(modelCache, dataCache, &preparedModel, &status);
        if (status != ErrorStatus::INVALID_ARGUMENT) {
            ASSERT_EQ(status, ErrorStatus::GENERAL_FAILURE);
        }
        ASSERT_EQ(preparedModel, nullptr);
    }

    // Go through each handle in data cache, test with NumFd equal to 0.
    for (uint32_t i = 0; i < mNumDataCache; i++) {
        bool supported;
        hidl_vec<hidl_handle> modelCache, dataCache;
        // Pass an invalid number of fds for handle i.
        auto tmp = mDataCache[i].back();
        mDataCache[i].pop_back();
        createCacheHandles(mModelCache, AccessMode::READ_WRITE, &modelCache);
        createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
        mDataCache[i].push_back(tmp);
        sp<IPreparedModel> preparedModel = nullptr;
        saveModelToCache(testModel, modelCache, dataCache, &supported, &preparedModel);
        if (checkEarlyTermination(supported)) return;
        ASSERT_NE(preparedModel, nullptr);
        // Execute and verify results.
        generated_tests::EvaluatePreparedModel(preparedModel, [](int) { return false; },
                                               get_examples(),
                                               testModel.relaxComputationFloat32toFloat16,
                                               /*testDynamicOutputShape=*/false);
        // Check if prepareModelFromCache fails.
        preparedModel = nullptr;
        ErrorStatus status;
        prepareModelFromCache(modelCache, dataCache, &preparedModel, &status);
        if (status != ErrorStatus::INVALID_ARGUMENT) {
            ASSERT_EQ(status, ErrorStatus::GENERAL_FAILURE);
        }
        ASSERT_EQ(preparedModel, nullptr);
    }
}

TEST_F(CompilationCachingTest, PrepareModelFromCacheInvalidNumFd) {
    // Create test HIDL model and compile.
    Model testModel = createTestModel();

    // Save the compilation to cache.
    {
        bool supported;
        hidl_vec<hidl_handle> modelCache, dataCache;
        createCacheHandles(mModelCache, AccessMode::READ_WRITE, &modelCache);
        createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
        saveModelToCache(testModel, modelCache, dataCache, &supported);
        if (checkEarlyTermination(supported)) return;
    }

    // Go through each handle in model cache, test with NumFd greater than 1.
    for (uint32_t i = 0; i < mNumModelCache; i++) {
        sp<IPreparedModel> preparedModel = nullptr;
        ErrorStatus status;
        hidl_vec<hidl_handle> modelCache, dataCache;
        mModelCache[i].push_back(mTmpCache);
        createCacheHandles(mModelCache, AccessMode::READ_WRITE, &modelCache);
        createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
        mModelCache[i].pop_back();
        prepareModelFromCache(modelCache, dataCache, &preparedModel, &status);
        if (status != ErrorStatus::GENERAL_FAILURE) {
            ASSERT_EQ(status, ErrorStatus::INVALID_ARGUMENT);
        }
        ASSERT_EQ(preparedModel, nullptr);
    }

    // Go through each handle in model cache, test with NumFd equal to 0.
    for (uint32_t i = 0; i < mNumModelCache; i++) {
        sp<IPreparedModel> preparedModel = nullptr;
        ErrorStatus status;
        hidl_vec<hidl_handle> modelCache, dataCache;
        auto tmp = mModelCache[i].back();
        mModelCache[i].pop_back();
        createCacheHandles(mModelCache, AccessMode::READ_WRITE, &modelCache);
        createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
        mModelCache[i].push_back(tmp);
        prepareModelFromCache(modelCache, dataCache, &preparedModel, &status);
        if (status != ErrorStatus::GENERAL_FAILURE) {
            ASSERT_EQ(status, ErrorStatus::INVALID_ARGUMENT);
        }
        ASSERT_EQ(preparedModel, nullptr);
    }

    // Go through each handle in data cache, test with NumFd greater than 1.
    for (uint32_t i = 0; i < mNumDataCache; i++) {
        sp<IPreparedModel> preparedModel = nullptr;
        ErrorStatus status;
        hidl_vec<hidl_handle> modelCache, dataCache;
        mDataCache[i].push_back(mTmpCache);
        createCacheHandles(mModelCache, AccessMode::READ_WRITE, &modelCache);
        createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
        mDataCache[i].pop_back();
        prepareModelFromCache(modelCache, dataCache, &preparedModel, &status);
        if (status != ErrorStatus::GENERAL_FAILURE) {
            ASSERT_EQ(status, ErrorStatus::INVALID_ARGUMENT);
        }
        ASSERT_EQ(preparedModel, nullptr);
    }

    // Go through each handle in data cache, test with NumFd equal to 0.
    for (uint32_t i = 0; i < mNumDataCache; i++) {
        sp<IPreparedModel> preparedModel = nullptr;
        ErrorStatus status;
        hidl_vec<hidl_handle> modelCache, dataCache;
        auto tmp = mDataCache[i].back();
        mDataCache[i].pop_back();
        createCacheHandles(mModelCache, AccessMode::READ_WRITE, &modelCache);
        createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
        mDataCache[i].push_back(tmp);
        prepareModelFromCache(modelCache, dataCache, &preparedModel, &status);
        if (status != ErrorStatus::GENERAL_FAILURE) {
            ASSERT_EQ(status, ErrorStatus::INVALID_ARGUMENT);
        }
        ASSERT_EQ(preparedModel, nullptr);
    }
}

TEST_F(CompilationCachingTest, SaveToCacheInvalidAccessMode) {
    // Create test HIDL model and compile.
    Model testModel = createTestModel();
    std::vector<AccessMode> modelCacheMode(mNumModelCache, AccessMode::READ_WRITE);
    std::vector<AccessMode> dataCacheMode(mNumDataCache, AccessMode::READ_WRITE);

    // Go through each handle in model cache, test with invalid access mode.
    for (uint32_t i = 0; i < mNumModelCache; i++) {
        bool supported;
        hidl_vec<hidl_handle> modelCache, dataCache;
        modelCacheMode[i] = AccessMode::READ_ONLY;
        createCacheHandles(mModelCache, modelCacheMode, &modelCache);
        createCacheHandles(mDataCache, dataCacheMode, &dataCache);
        modelCacheMode[i] = AccessMode::READ_WRITE;
        sp<IPreparedModel> preparedModel = nullptr;
        saveModelToCache(testModel, modelCache, dataCache, &supported, &preparedModel);
        if (checkEarlyTermination(supported)) return;
        ASSERT_NE(preparedModel, nullptr);
        // Execute and verify results.
        generated_tests::EvaluatePreparedModel(preparedModel, [](int) { return false; },
                                               get_examples(),
                                               testModel.relaxComputationFloat32toFloat16,
                                               /*testDynamicOutputShape=*/false);
        // Check if prepareModelFromCache fails.
        preparedModel = nullptr;
        ErrorStatus status;
        prepareModelFromCache(modelCache, dataCache, &preparedModel, &status);
        if (status != ErrorStatus::INVALID_ARGUMENT) {
            ASSERT_EQ(status, ErrorStatus::GENERAL_FAILURE);
        }
        ASSERT_EQ(preparedModel, nullptr);
    }

    // Go through each handle in data cache, test with invalid access mode.
    for (uint32_t i = 0; i < mNumDataCache; i++) {
        bool supported;
        hidl_vec<hidl_handle> modelCache, dataCache;
        dataCacheMode[i] = AccessMode::READ_ONLY;
        createCacheHandles(mModelCache, modelCacheMode, &modelCache);
        createCacheHandles(mDataCache, dataCacheMode, &dataCache);
        dataCacheMode[i] = AccessMode::READ_WRITE;
        sp<IPreparedModel> preparedModel = nullptr;
        saveModelToCache(testModel, modelCache, dataCache, &supported, &preparedModel);
        if (checkEarlyTermination(supported)) return;
        ASSERT_NE(preparedModel, nullptr);
        // Execute and verify results.
        generated_tests::EvaluatePreparedModel(preparedModel, [](int) { return false; },
                                               get_examples(),
                                               testModel.relaxComputationFloat32toFloat16,
                                               /*testDynamicOutputShape=*/false);
        // Check if prepareModelFromCache fails.
        preparedModel = nullptr;
        ErrorStatus status;
        prepareModelFromCache(modelCache, dataCache, &preparedModel, &status);
        if (status != ErrorStatus::INVALID_ARGUMENT) {
            ASSERT_EQ(status, ErrorStatus::GENERAL_FAILURE);
        }
        ASSERT_EQ(preparedModel, nullptr);
    }
}

TEST_F(CompilationCachingTest, PrepareModelFromCacheInvalidAccessMode) {
    // Create test HIDL model and compile.
    Model testModel = createTestModel();
    std::vector<AccessMode> modelCacheMode(mNumModelCache, AccessMode::READ_WRITE);
    std::vector<AccessMode> dataCacheMode(mNumDataCache, AccessMode::READ_WRITE);

    // Save the compilation to cache.
    {
        bool supported;
        hidl_vec<hidl_handle> modelCache, dataCache;
        createCacheHandles(mModelCache, AccessMode::READ_WRITE, &modelCache);
        createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
        saveModelToCache(testModel, modelCache, dataCache, &supported);
        if (checkEarlyTermination(supported)) return;
    }

    // Go through each handle in model cache, test with invalid access mode.
    for (uint32_t i = 0; i < mNumModelCache; i++) {
        sp<IPreparedModel> preparedModel = nullptr;
        ErrorStatus status;
        hidl_vec<hidl_handle> modelCache, dataCache;
        modelCacheMode[i] = AccessMode::WRITE_ONLY;
        createCacheHandles(mModelCache, modelCacheMode, &modelCache);
        createCacheHandles(mDataCache, dataCacheMode, &dataCache);
        modelCacheMode[i] = AccessMode::READ_WRITE;
        prepareModelFromCache(modelCache, dataCache, &preparedModel, &status);
        ASSERT_EQ(status, ErrorStatus::GENERAL_FAILURE);
        ASSERT_EQ(preparedModel, nullptr);
    }

    // Go through each handle in data cache, test with invalid access mode.
    for (uint32_t i = 0; i < mNumDataCache; i++) {
        sp<IPreparedModel> preparedModel = nullptr;
        ErrorStatus status;
        hidl_vec<hidl_handle> modelCache, dataCache;
        dataCacheMode[i] = AccessMode::WRITE_ONLY;
        createCacheHandles(mModelCache, modelCacheMode, &modelCache);
        createCacheHandles(mDataCache, dataCacheMode, &dataCache);
        dataCacheMode[i] = AccessMode::READ_WRITE;
        prepareModelFromCache(modelCache, dataCache, &preparedModel, &status);
        ASSERT_EQ(status, ErrorStatus::GENERAL_FAILURE);
        ASSERT_EQ(preparedModel, nullptr);
    }
}

class CompilationCachingSecurityTest : public CompilationCachingTest,
                                       public ::testing::WithParamInterface<uint32_t> {
  protected:
    void SetUp() {
        CompilationCachingTest::SetUp();
        generator.seed(kSeed);
    }

    // Get a random integer within a closed range [lower, upper].
    template <typename T>
    T getRandomInt(T lower, T upper) {
        std::uniform_int_distribution<T> dis(lower, upper);
        return dis(generator);
    }

    const uint32_t kSeed = GetParam();
    std::mt19937 generator;
};

TEST_P(CompilationCachingSecurityTest, CorruptedSecuritySensitiveCache) {
    if (!mIsCachingSupported) return;

    // Create test HIDL model and compile.
    Model testModel = createTestModel();

    for (uint32_t i = 0; i < mNumModelCache; i++) {
        // Save the compilation to cache.
        {
            bool supported;
            hidl_vec<hidl_handle> modelCache, dataCache;
            createCacheHandles(mModelCache, AccessMode::READ_WRITE, &modelCache);
            createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
            saveModelToCache(testModel, modelCache, dataCache, &supported);
            if (checkEarlyTermination(supported)) return;
        }

        // Randomly flip one single bit of the cache entry.
        FILE* pFile = fopen(mModelCache[i][0].c_str(), "r+");
        ASSERT_EQ(fseek(pFile, 0, SEEK_END), 0);
        long int fileSize = ftell(pFile);
        if (fileSize == 0) {
            fclose(pFile);
            continue;
        }
        ASSERT_EQ(fseek(pFile, getRandomInt(0l, fileSize - 1), SEEK_SET), 0);
        int readByte = fgetc(pFile);
        ASSERT_NE(readByte, EOF);
        ASSERT_EQ(fseek(pFile, -1, SEEK_CUR), 0);
        ASSERT_NE(fputc(static_cast<uint8_t>(readByte) ^ (1U << getRandomInt(0, 7)), pFile), EOF);
        fclose(pFile);

        // Retrieve preparedModel from cache, expect failure.
        {
            sp<IPreparedModel> preparedModel = nullptr;
            ErrorStatus status;
            hidl_vec<hidl_handle> modelCache, dataCache;
            createCacheHandles(mModelCache, AccessMode::READ_WRITE, &modelCache);
            createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
            prepareModelFromCache(modelCache, dataCache, &preparedModel, &status);
            ASSERT_EQ(status, ErrorStatus::GENERAL_FAILURE);
            ASSERT_EQ(preparedModel, nullptr);
        }
    }
}

TEST_P(CompilationCachingSecurityTest, WrongLengthSecuritySensitiveCache) {
    if (!mIsCachingSupported) return;

    // Create test HIDL model and compile.
    Model testModel = createTestModel();

    for (uint32_t i = 0; i < mNumModelCache; i++) {
        // Save the compilation to cache.
        {
            bool supported;
            hidl_vec<hidl_handle> modelCache, dataCache;
            createCacheHandles(mModelCache, AccessMode::READ_WRITE, &modelCache);
            createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
            saveModelToCache(testModel, modelCache, dataCache, &supported);
            if (checkEarlyTermination(supported)) return;
        }

        // Randomly append bytes to the cache entry.
        FILE* pFile = fopen(mModelCache[i][0].c_str(), "a");
        uint32_t appendLength = getRandomInt(1, 256);
        for (uint32_t i = 0; i < appendLength; i++) {
            ASSERT_NE(fputc(getRandomInt<uint8_t>(0, 255), pFile), EOF);
        }
        fclose(pFile);

        // Retrieve preparedModel from cache, expect failure.
        {
            sp<IPreparedModel> preparedModel = nullptr;
            ErrorStatus status;
            hidl_vec<hidl_handle> modelCache, dataCache;
            createCacheHandles(mModelCache, AccessMode::READ_WRITE, &modelCache);
            createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
            prepareModelFromCache(modelCache, dataCache, &preparedModel, &status);
            ASSERT_EQ(status, ErrorStatus::GENERAL_FAILURE);
            ASSERT_EQ(preparedModel, nullptr);
        }
    }
}

TEST_P(CompilationCachingSecurityTest, WrongToken) {
    if (!mIsCachingSupported) return;

    // Create test HIDL model and compile.
    Model testModel = createTestModel();

    // Save the compilation to cache.
    {
        bool supported;
        hidl_vec<hidl_handle> modelCache, dataCache;
        createCacheHandles(mModelCache, AccessMode::READ_WRITE, &modelCache);
        createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
        saveModelToCache(testModel, modelCache, dataCache, &supported);
        if (checkEarlyTermination(supported)) return;
    }

    // Randomly flip one single bit in mToken.
    uint32_t ind = getRandomInt(0u, static_cast<uint32_t>(Constant::BYTE_SIZE_OF_CACHE_TOKEN) - 1);
    mToken[ind] ^= (1U << getRandomInt(0, 7));

    // Retrieve the preparedModel from cache, expect failure.
    {
        sp<IPreparedModel> preparedModel = nullptr;
        ErrorStatus status;
        hidl_vec<hidl_handle> modelCache, dataCache;
        createCacheHandles(mModelCache, AccessMode::READ_WRITE, &modelCache);
        createCacheHandles(mDataCache, AccessMode::READ_WRITE, &dataCache);
        prepareModelFromCache(modelCache, dataCache, &preparedModel, &status);
        ASSERT_EQ(status, ErrorStatus::GENERAL_FAILURE);
        ASSERT_EQ(preparedModel, nullptr);
    }
}

INSTANTIATE_TEST_CASE_P(TestCompilationCaching, CompilationCachingSecurityTest,
                        ::testing::Range(0U, 10U));

}  // namespace functional
}  // namespace vts
}  // namespace V1_2
}  // namespace neuralnetworks
}  // namespace hardware
}  // namespace android
