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

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include "FormatConvert.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, std::size_t size) {
    if (size < 256) {
        return 0;
    }

    std::srand(std::time(nullptr));  // use current time as seed for random generator
    int random_variable = std::rand() % 10;
    int width = (int)sqrt(size);
    int height = width * ((float)random_variable / 10.0);

    uint8_t* src = (uint8_t*)malloc(sizeof(uint8_t) * size);
    memcpy(src, data, sizeof(uint8_t) * (size));
    uint32_t* tgt = (uint32_t*)malloc(sizeof(uint32_t) * size);

#ifdef COPY_NV21_TO_RGB32
    android::hardware::automotive::evs::common::Utils::copyNV21toRGB32(width, height, src, tgt, 0);
#elif COPY_NV21_TO_BGR32
    android::hardware::automotive::evs::common::Utils::copyNV21toBGR32(width, height, src, tgt, 0);
#elif COPY_YV12_TO_RGB32
    android::hardware::automotive::evs::common::Utils::copyYV12toRGB32(width, height, src, tgt, 0);
#elif COPY_YV12_TO_BGR32
    android::hardware::automotive::evs::common::Utils::copyYV12toBGR32(width, height, src, tgt, 0);
#elif COPY_YUYV_TO_RGB32
    android::hardware::automotive::evs::common::Utils::copyYUYVtoRGB32(width, height, src, 0, tgt,
                                                                       0);
#elif COPY_YUYV_TO_BGR32
    android::hardware::automotive::evs::common::Utils::copyYUYVtoBGR32(width, height, src, 0, tgt,
                                                                       0);
#endif

    free(src);
    free(tgt);

    return 0;
}
