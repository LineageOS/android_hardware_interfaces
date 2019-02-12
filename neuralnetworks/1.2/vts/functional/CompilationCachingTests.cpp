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

enum class AccessMode { READ_ONLY, WRITE_ONLY };

void createCacheHandle(const std::vector<std::string>& files, AccessMode mode,
                       hidl_handle* handle) {
    std::vector<int> fds;
    for (const auto& file : files) {
        int fd;
        if (mode == AccessMode::READ_ONLY) {
            fd = open(file.c_str(), O_RDONLY);
        } else if (mode == AccessMode::WRITE_ONLY) {
            fd = open(file.c_str(), O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR);
        } else {
            FAIL();
        }
        ASSERT_GE(fd, 0);
        fds.push_back(fd);
    }
    native_handle_t* cacheNativeHandle = native_handle_create(fds.size(), 0);
    ASSERT_NE(cacheNativeHandle, nullptr);
    for (uint32_t i = 0; i < fds.size(); i++) {
        cacheNativeHandle->data[i] = fds[i];
    }
    handle->setTo(cacheNativeHandle, /*shouldOwn=*/true);
}

}  // namespace

// Tag for the compilation caching tests.
class CompilationCachingTest : public NeuralnetworksHidlTest {
  protected:
    void SetUp() override {
        NeuralnetworksHidlTest::SetUp();

        // Create cache directory.
        char cacheDirTemp[] = "/data/local/tmp/TestCompilationCachingXXXXXX";
        char* cacheDir = mkdtemp(cacheDirTemp);
        ASSERT_NE(cacheDir, nullptr);
        mCache1 = cacheDir + mCache1;
        mCache2 = cacheDir + mCache2;
        mCache3 = cacheDir + mCache3;

        // Check if caching is supported.
        bool isCachingSupported;
        Return<void> ret = device->isCachingSupported(
                [&isCachingSupported](ErrorStatus status, bool supported) {
                    EXPECT_EQ(ErrorStatus::NONE, status);
                    isCachingSupported = supported;
                });
        EXPECT_TRUE(ret.isOk());
        if (isCachingSupported) {
            mIsCachingSupported = true;
        } else {
            LOG(INFO) << "NN VTS: Early termination of test because vendor service does not "
                         "support compilation caching.";
            std::cout << "[          ]   Early termination of test because vendor service does not "
                         "support compilation caching."
                      << std::endl;
            mIsCachingSupported = false;
        }

        // Create empty cache files.
        hidl_handle handle;
        createCacheHandle({mCache1, mCache2, mCache3}, AccessMode::WRITE_ONLY, &handle);
    }

    void saveModelToCache(sp<IPreparedModel> preparedModel, const hidl_handle& cache1,
                          const hidl_handle& cache2, ErrorStatus* status) {
        // Save IPreparedModel to cache.
        hidl_array<uint8_t, sizeof(mToken)> cacheToken(mToken);
        Return<ErrorStatus> saveToCacheStatus =
                preparedModel->saveToCache(cache1, cache2, cacheToken);
        ASSERT_TRUE(saveToCacheStatus.isOk());
        *status = static_cast<ErrorStatus>(saveToCacheStatus);
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

    void prepareModelFromCache(const hidl_handle& cache1, const hidl_handle& cache2,
                               sp<IPreparedModel>* preparedModel, ErrorStatus* status) {
        // Launch prepare model from cache.
        sp<PreparedModelCallback> preparedModelCallback = new PreparedModelCallback();
        ASSERT_NE(nullptr, preparedModelCallback.get());
        hidl_array<uint8_t, sizeof(mToken)> cacheToken(mToken);
        Return<ErrorStatus> prepareLaunchStatus =
                device->prepareModelFromCache(cache1, cache2, cacheToken, preparedModelCallback);
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

    std::string mCache1 = "/cache1";
    std::string mCache2 = "/cache2";
    std::string mCache3 = "/cache3";
    uint8_t mToken[static_cast<uint32_t>(Constant::BYTE_SIZE_OF_CACHE_TOKEN)] = {};
    bool mIsCachingSupported;
};

TEST_F(CompilationCachingTest, CacheSavingAndRetrieval) {
    // Create test HIDL model and compile.
    Model testModel = createTestModel();
    sp<IPreparedModel> preparedModel = nullptr;
    generated_tests::PrepareModel(device, testModel, &preparedModel);
    // Terminate early if the driver cannot prepare the model.
    if (preparedModel == nullptr) return;

    // Save the compilation to cache.
    {
        ErrorStatus status;
        hidl_handle cache1, cache2;
        createCacheHandle({mCache1}, AccessMode::WRITE_ONLY, &cache1);
        createCacheHandle({mCache2}, AccessMode::WRITE_ONLY, &cache2);
        saveModelToCache(preparedModel, cache1, cache2, &status);
        if (!mIsCachingSupported) {
            EXPECT_EQ(status, ErrorStatus::GENERAL_FAILURE);
        } else {
            if (checkEarlyTermination(status)) return;
            ASSERT_EQ(status, ErrorStatus::NONE);
        }
    }

    // Retrieve preparedModel from cache.
    {
        preparedModel = nullptr;
        ErrorStatus status;
        hidl_handle cache1, cache2;
        createCacheHandle({mCache1}, AccessMode::READ_ONLY, &cache1);
        createCacheHandle({mCache2}, AccessMode::READ_ONLY, &cache2);
        prepareModelFromCache(cache1, cache2, &preparedModel, &status);
        if (!mIsCachingSupported) {
            ASSERT_EQ(status, ErrorStatus::GENERAL_FAILURE);
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
    generated_tests::PrepareModel(device, testModel, &preparedModel);
    // Terminate early if the driver cannot prepare the model.
    if (preparedModel == nullptr) return;

    // Save the compilation to cache.
    {
        ErrorStatus status;
        hidl_handle cache1, cache2;
        createCacheHandle({mCache1}, AccessMode::WRITE_ONLY, &cache1);
        createCacheHandle({mCache2}, AccessMode::WRITE_ONLY, &cache2);
        saveModelToCache(preparedModel, cache1, cache2, &status);
        if (!mIsCachingSupported) {
            EXPECT_EQ(status, ErrorStatus::GENERAL_FAILURE);
        } else {
            if (checkEarlyTermination(status)) return;
            ASSERT_EQ(status, ErrorStatus::NONE);
        }
    }

    // Retrieve preparedModel from cache.
    {
        preparedModel = nullptr;
        ErrorStatus status;
        hidl_handle cache1, cache2;
        createCacheHandle({mCache1}, AccessMode::READ_ONLY, &cache1);
        createCacheHandle({mCache2}, AccessMode::READ_ONLY, &cache2);
        uint8_t dummyByte = 0;
        // Advance offset by one byte.
        ASSERT_GE(read(cache1.getNativeHandle()->data[0], &dummyByte, 1), 0);
        ASSERT_GE(read(cache2.getNativeHandle()->data[0], &dummyByte, 1), 0);
        prepareModelFromCache(cache1, cache2, &preparedModel, &status);
        if (!mIsCachingSupported) {
            ASSERT_EQ(status, ErrorStatus::GENERAL_FAILURE);
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

TEST_F(CompilationCachingTest, SaveToCacheInvalidNumFd) {
    // Create test HIDL model and compile.
    Model testModel = createTestModel();
    sp<IPreparedModel> preparedModel = nullptr;
    generated_tests::PrepareModel(device, testModel, &preparedModel);
    // Terminate early if the driver cannot prepare the model.
    if (preparedModel == nullptr) return;

    // cache1 with invalid NumFd.
    {
        ErrorStatus status;
        hidl_handle cache1, cache2;
        createCacheHandle({mCache1, mCache3}, AccessMode::WRITE_ONLY, &cache1);
        createCacheHandle({mCache2}, AccessMode::WRITE_ONLY, &cache2);
        saveModelToCache(preparedModel, cache1, cache2, &status);
        if (status != ErrorStatus::GENERAL_FAILURE) {
            ASSERT_EQ(status, ErrorStatus::INVALID_ARGUMENT);
        }
    }

    // cache2 with invalid NumFd.
    {
        ErrorStatus status;
        hidl_handle cache1, cache2;
        createCacheHandle({mCache1}, AccessMode::WRITE_ONLY, &cache1);
        createCacheHandle({mCache2, mCache3}, AccessMode::WRITE_ONLY, &cache2);
        saveModelToCache(preparedModel, cache1, cache2, &status);
        if (status != ErrorStatus::GENERAL_FAILURE) {
            ASSERT_EQ(status, ErrorStatus::INVALID_ARGUMENT);
        }
    }
}

TEST_F(CompilationCachingTest, PrepareModelFromCacheInvalidNumFd) {
    // Create test HIDL model and compile.
    Model testModel = createTestModel();
    sp<IPreparedModel> preparedModel = nullptr;
    generated_tests::PrepareModel(device, testModel, &preparedModel);
    // Terminate early if the driver cannot prepare the model.
    if (preparedModel == nullptr) return;

    // Save the compilation to cache.
    {
        ErrorStatus status;
        hidl_handle cache1, cache2;
        createCacheHandle({mCache1}, AccessMode::WRITE_ONLY, &cache1);
        createCacheHandle({mCache2}, AccessMode::WRITE_ONLY, &cache2);
        saveModelToCache(preparedModel, cache1, cache2, &status);
        if (status != ErrorStatus::GENERAL_FAILURE) {
            ASSERT_EQ(status, ErrorStatus::NONE);
        }
    }

    // cache1 with invalid NumFd.
    {
        preparedModel = nullptr;
        ErrorStatus status;
        hidl_handle cache1, cache2;
        createCacheHandle({mCache1, mCache3}, AccessMode::READ_ONLY, &cache1);
        createCacheHandle({mCache2}, AccessMode::READ_ONLY, &cache2);
        prepareModelFromCache(cache1, cache2, &preparedModel, &status);
        if (status != ErrorStatus::GENERAL_FAILURE) {
            ASSERT_EQ(status, ErrorStatus::INVALID_ARGUMENT);
            ASSERT_EQ(preparedModel, nullptr);
        }
    }

    // cache2 with invalid NumFd.
    {
        preparedModel = nullptr;
        ErrorStatus status;
        hidl_handle cache1, cache2;
        createCacheHandle({mCache1}, AccessMode::READ_ONLY, &cache1);
        createCacheHandle({mCache2, mCache3}, AccessMode::READ_ONLY, &cache2);
        prepareModelFromCache(cache1, cache2, &preparedModel, &status);
        if (status != ErrorStatus::GENERAL_FAILURE) {
            ASSERT_EQ(status, ErrorStatus::INVALID_ARGUMENT);
            ASSERT_EQ(preparedModel, nullptr);
        }
    }
}

TEST_F(CompilationCachingTest, SaveToCacheInvalidAccessMode) {
    // Create test HIDL model and compile.
    Model testModel = createTestModel();
    sp<IPreparedModel> preparedModel = nullptr;
    generated_tests::PrepareModel(device, testModel, &preparedModel);
    // Terminate early if the driver cannot prepare the model.
    if (preparedModel == nullptr) return;

    // cache1 with invalid access mode.
    {
        ErrorStatus status;
        hidl_handle cache1, cache2;
        createCacheHandle({mCache1}, AccessMode::READ_ONLY, &cache1);
        createCacheHandle({mCache2}, AccessMode::WRITE_ONLY, &cache2);
        saveModelToCache(preparedModel, cache1, cache2, &status);
        ASSERT_EQ(status, ErrorStatus::GENERAL_FAILURE);
    }

    // cache2 with invalid access mode.
    {
        ErrorStatus status;
        hidl_handle cache1, cache2;
        createCacheHandle({mCache1}, AccessMode::WRITE_ONLY, &cache1);
        createCacheHandle({mCache2}, AccessMode::READ_ONLY, &cache2);
        saveModelToCache(preparedModel, cache1, cache2, &status);
        ASSERT_EQ(status, ErrorStatus::GENERAL_FAILURE);
    }
}

TEST_F(CompilationCachingTest, PrepareModelFromCacheInvalidAccessMode) {
    // Create test HIDL model and compile.
    Model testModel = createTestModel();
    sp<IPreparedModel> preparedModel = nullptr;
    generated_tests::PrepareModel(device, testModel, &preparedModel);
    // Terminate early if the driver cannot prepare the model.
    if (preparedModel == nullptr) return;

    // Save the compilation to cache.
    {
        ErrorStatus status;
        hidl_handle cache1, cache2;
        createCacheHandle({mCache1}, AccessMode::WRITE_ONLY, &cache1);
        createCacheHandle({mCache2}, AccessMode::WRITE_ONLY, &cache2);
        saveModelToCache(preparedModel, cache1, cache2, &status);
        if (status != ErrorStatus::GENERAL_FAILURE) {
            ASSERT_EQ(status, ErrorStatus::NONE);
        }
    }

    // cache1 with invalid access mode.
    {
        preparedModel = nullptr;
        ErrorStatus status;
        hidl_handle cache1, cache2;
        createCacheHandle({mCache1}, AccessMode::WRITE_ONLY, &cache1);
        createCacheHandle({mCache2}, AccessMode::READ_ONLY, &cache2);
        prepareModelFromCache(cache1, cache2, &preparedModel, &status);
        ASSERT_EQ(status, ErrorStatus::GENERAL_FAILURE);
        ASSERT_EQ(preparedModel, nullptr);
    }

    // cache2 with invalid access mode.
    {
        preparedModel = nullptr;
        ErrorStatus status;
        hidl_handle cache1, cache2;
        createCacheHandle({mCache1}, AccessMode::READ_ONLY, &cache1);
        createCacheHandle({mCache2}, AccessMode::WRITE_ONLY, &cache2);
        prepareModelFromCache(cache1, cache2, &preparedModel, &status);
        ASSERT_EQ(status, ErrorStatus::GENERAL_FAILURE);
        ASSERT_EQ(preparedModel, nullptr);
    }
}

TEST_F(CompilationCachingTest, SaveToCacheInvalidOffset) {
    // Create test HIDL model and compile.
    Model testModel = createTestModel();
    sp<IPreparedModel> preparedModel = nullptr;
    generated_tests::PrepareModel(device, testModel, &preparedModel);
    // Terminate early if the driver cannot prepare the model.
    if (preparedModel == nullptr) return;

    // cache1 with invalid file descriptor offset.
    {
        ErrorStatus status;
        hidl_handle cache1, cache2;
        createCacheHandle({mCache1}, AccessMode::WRITE_ONLY, &cache1);
        createCacheHandle({mCache2}, AccessMode::WRITE_ONLY, &cache2);
        uint8_t dummyByte = 0;
        // Advance offset by one byte.
        ASSERT_EQ(write(cache1.getNativeHandle()->data[0], &dummyByte, 1), 1);
        saveModelToCache(preparedModel, cache1, cache2, &status);
        ASSERT_EQ(status, ErrorStatus::GENERAL_FAILURE);
    }

    // cache2 with invalid file descriptor offset.
    {
        ErrorStatus status;
        hidl_handle cache1, cache2;
        createCacheHandle({mCache1}, AccessMode::WRITE_ONLY, &cache1);
        createCacheHandle({mCache2}, AccessMode::WRITE_ONLY, &cache2);
        uint8_t dummyByte = 0;
        // Advance offset by one byte.
        ASSERT_EQ(write(cache2.getNativeHandle()->data[0], &dummyByte, 1), 1);
        saveModelToCache(preparedModel, cache1, cache2, &status);
        ASSERT_EQ(status, ErrorStatus::GENERAL_FAILURE);
    }
}

TEST_F(CompilationCachingTest, SaveToCacheInvalidFileSize) {
    // Create test HIDL model and compile.
    Model testModel = createTestModel();
    sp<IPreparedModel> preparedModel = nullptr;
    generated_tests::PrepareModel(device, testModel, &preparedModel);
    // Terminate early if the driver cannot prepare the model.
    if (preparedModel == nullptr) return;

    // cache1 with invalid file size.
    {
        ErrorStatus status;
        hidl_handle cache1, cache2;
        createCacheHandle({mCache1}, AccessMode::WRITE_ONLY, &cache1);
        createCacheHandle({mCache2}, AccessMode::WRITE_ONLY, &cache2);
        uint8_t dummyByte = 0;
        // Write one byte and seek back to the beginning.
        ASSERT_EQ(write(cache1.getNativeHandle()->data[0], &dummyByte, 1), 1);
        ASSERT_EQ(lseek(cache1.getNativeHandle()->data[0], 0, SEEK_SET), 0);
        saveModelToCache(preparedModel, cache1, cache2, &status);
        ASSERT_EQ(status, ErrorStatus::GENERAL_FAILURE);
    }

    // cache2 with invalid file size.
    {
        ErrorStatus status;
        hidl_handle cache1, cache2;
        createCacheHandle({mCache1}, AccessMode::WRITE_ONLY, &cache1);
        createCacheHandle({mCache2}, AccessMode::WRITE_ONLY, &cache2);
        uint8_t dummyByte = 0;
        // Write one byte and seek back to the beginning.
        ASSERT_EQ(write(cache2.getNativeHandle()->data[0], &dummyByte, 1), 1);
        ASSERT_EQ(lseek(cache2.getNativeHandle()->data[0], 0, SEEK_SET), 0);
        saveModelToCache(preparedModel, cache1, cache2, &status);
        ASSERT_EQ(status, ErrorStatus::GENERAL_FAILURE);
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
    sp<IPreparedModel> preparedModel = nullptr;
    generated_tests::PrepareModel(device, testModel, &preparedModel);
    // Terminate early if the driver cannot prepare the model.
    if (preparedModel == nullptr) return;

    // Save the compilation to cache.
    {
        ErrorStatus status;
        hidl_handle cache1, cache2;
        createCacheHandle({mCache1}, AccessMode::WRITE_ONLY, &cache1);
        createCacheHandle({mCache2}, AccessMode::WRITE_ONLY, &cache2);
        saveModelToCache(preparedModel, cache1, cache2, &status);
        if (checkEarlyTermination(status)) return;
        ASSERT_EQ(status, ErrorStatus::NONE);
    }

    // Randomly flip one single bit of the cache entry.
    FILE* pFile = fopen(mCache1.c_str(), "r+");
    ASSERT_EQ(fseek(pFile, 0, SEEK_END), 0);
    long int fileSize = ftell(pFile);
    ASSERT_GT(fileSize, 0);
    ASSERT_EQ(fseek(pFile, getRandomInt(0l, fileSize - 1), SEEK_SET), 0);
    int readByte = fgetc(pFile);
    ASSERT_NE(readByte, EOF);
    ASSERT_EQ(fseek(pFile, -1, SEEK_CUR), 0);
    ASSERT_NE(fputc(static_cast<uint8_t>(readByte) ^ (1U << getRandomInt(0, 7)), pFile), EOF);
    fclose(pFile);

    // Retrieve preparedModel from cache, expect failure.
    {
        preparedModel = nullptr;
        ErrorStatus status;
        hidl_handle cache1, cache2;
        createCacheHandle({mCache1}, AccessMode::READ_ONLY, &cache1);
        createCacheHandle({mCache2}, AccessMode::READ_ONLY, &cache2);
        prepareModelFromCache(cache1, cache2, &preparedModel, &status);
        ASSERT_EQ(status, ErrorStatus::GENERAL_FAILURE);
        ASSERT_EQ(preparedModel, nullptr);
    }
}

TEST_P(CompilationCachingSecurityTest, WrongLengthSecuritySensitiveCache) {
    if (!mIsCachingSupported) return;

    // Create test HIDL model and compile.
    Model testModel = createTestModel();
    sp<IPreparedModel> preparedModel = nullptr;
    generated_tests::PrepareModel(device, testModel, &preparedModel);
    // Terminate early if the driver cannot prepare the model.
    if (preparedModel == nullptr) return;

    // Save the compilation to cache.
    {
        ErrorStatus status;
        hidl_handle cache1, cache2;
        createCacheHandle({mCache1}, AccessMode::WRITE_ONLY, &cache1);
        createCacheHandle({mCache2}, AccessMode::WRITE_ONLY, &cache2);
        saveModelToCache(preparedModel, cache1, cache2, &status);
        if (checkEarlyTermination(status)) return;
        ASSERT_EQ(status, ErrorStatus::NONE);
    }

    // Randomly append bytes to the cache entry.
    FILE* pFile = fopen(mCache1.c_str(), "a");
    uint32_t appendLength = getRandomInt(1, 256);
    for (uint32_t i = 0; i < appendLength; i++) {
        ASSERT_NE(fputc(getRandomInt<uint8_t>(0, 255), pFile), EOF);
    }
    fclose(pFile);

    // Retrieve preparedModel from cache, expect failure.
    {
        preparedModel = nullptr;
        ErrorStatus status;
        hidl_handle cache1, cache2;
        createCacheHandle({mCache1}, AccessMode::READ_ONLY, &cache1);
        createCacheHandle({mCache2}, AccessMode::READ_ONLY, &cache2);
        prepareModelFromCache(cache1, cache2, &preparedModel, &status);
        ASSERT_EQ(status, ErrorStatus::GENERAL_FAILURE);
        ASSERT_EQ(preparedModel, nullptr);
    }
}

TEST_P(CompilationCachingSecurityTest, WrongToken) {
    if (!mIsCachingSupported) return;

    // Create test HIDL model and compile.
    Model testModel = createTestModel();
    sp<IPreparedModel> preparedModel = nullptr;
    generated_tests::PrepareModel(device, testModel, &preparedModel);
    // Terminate early if the driver cannot prepare the model.
    if (preparedModel == nullptr) return;

    // Save the compilation to cache.
    {
        ErrorStatus status;
        hidl_handle cache1, cache2;
        createCacheHandle({mCache1}, AccessMode::WRITE_ONLY, &cache1);
        createCacheHandle({mCache2}, AccessMode::WRITE_ONLY, &cache2);
        saveModelToCache(preparedModel, cache1, cache2, &status);
        if (checkEarlyTermination(status)) return;
        ASSERT_EQ(status, ErrorStatus::NONE);
    }

    // Randomly flip one single bit in mToken.
    uint32_t ind = getRandomInt(0u, static_cast<uint32_t>(Constant::BYTE_SIZE_OF_CACHE_TOKEN) - 1);
    mToken[ind] ^= (1U << getRandomInt(0, 7));

    // Retrieve the preparedModel from cache, expect failure.
    {
        preparedModel = nullptr;
        ErrorStatus status;
        hidl_handle cache1, cache2;
        createCacheHandle({mCache1}, AccessMode::READ_ONLY, &cache1);
        createCacheHandle({mCache2}, AccessMode::READ_ONLY, &cache2);
        prepareModelFromCache(cache1, cache2, &preparedModel, &status);
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
