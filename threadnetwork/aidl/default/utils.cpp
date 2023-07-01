
/*
 * Copyright (C) 2022 The Android Open Source Project
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

#include <openthread/instance.h>
#include <openthread/logging.h>
#include <openthread/platform/alarm-milli.h>
#include <utils/Log.h>

void otLogCritPlat(const char* format, ...) {
    va_list args;

    va_start(args, format);
    __android_log_vprint(ANDROID_LOG_FATAL, LOG_TAG, format, args);
    va_end(args);
}

void otLogWarnPlat(const char* format, ...) {
    va_list args;

    va_start(args, format);
    __android_log_vprint(ANDROID_LOG_WARN, LOG_TAG, format, args);
    va_end(args);
}

OT_TOOL_WEAK void otPlatAlarmMilliFired(otInstance* aInstance) {
    OT_UNUSED_VARIABLE(aInstance);
}
