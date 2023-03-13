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

#include <android/binder_auto_utils.h>
#include <android/binder_ibinder_jni.h>
#include <android/binder_manager.h>
#include <jni.h>
#include <log/log.h>

extern "C" JNIEXPORT jobject JNICALL
Java_example_vib_MyActivity_gimme__Ljava_lang_String_2(JNIEnv* env, jclass /**/, jstring str) {
    ALOGI("%s", __func__);

    // Best practice is probably libnativehelper ScopedUtfChars or
    // libbase ScopeGuard (for platform code), but this is with minimal
    // dependencies.
    const char* name = env->GetStringUTFChars(str, nullptr);

    ALOGI("example vib gimme %s", name);

    jobject jbinder = nullptr;

    // Java does not have vendor variants. It's only safe to pass a service when
    // 'vendor: true' if it is @VintfStability.
    if (AServiceManager_isDeclared(name)) {
        ndk::SpAIBinder binder = ndk::SpAIBinder(AServiceManager_waitForService(name));
        jbinder = AIBinder_toJavaBinder(env, binder.get());
    } else {
        ALOGI("not declared");
    }

    env->ReleaseStringUTFChars(str, name);

    return jbinder;
}
