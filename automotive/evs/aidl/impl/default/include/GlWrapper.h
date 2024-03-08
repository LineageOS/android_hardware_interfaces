/*
 * Copyright (C) 2023 The Android Open Source Project
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

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#include <aidl/android/frameworks/automotive/display/ICarDisplayProxy.h>
#include <aidl/android/hardware/automotive/evs/BufferDesc.h>
#include <android-base/logging.h>
#include <cutils/native_handle.h>

namespace aidl::android::hardware::automotive::evs::implementation {

namespace automotivedisplay = ::aidl::android::frameworks::automotive::display;

class GlWrapper {
  public:
    bool initialize(const std::shared_ptr<automotivedisplay::ICarDisplayProxy>& svc,
                    uint64_t displayId);
    void shutdown();

    bool updateImageTexture(
            buffer_handle_t handle,
            const ::aidl::android::hardware::graphics::common::HardwareBufferDescription&
                    description);
    void renderImageToScreen();

    void showWindow(const std::shared_ptr<automotivedisplay::ICarDisplayProxy>& svc,
                    uint64_t displayId);
    void hideWindow(const std::shared_ptr<automotivedisplay::ICarDisplayProxy>& svc,
                    uint64_t displayId);

    unsigned getWidth() { return mWidth; };
    unsigned getHeight() { return mHeight; };

  private:
    EGLDisplay mDisplay;
    EGLSurface mSurface;
    EGLContext mContext;

    unsigned mWidth = 0;
    unsigned mHeight = 0;

    EGLImageKHR mKHRimage = EGL_NO_IMAGE_KHR;

    GLuint mTextureMap = 0;
    GLuint mShaderProgram = 0;

    // Opaque handle for a native hardware buffer defined in
    // frameworks/native/opengl/include/EGL/eglplatform.h
    ANativeWindow* mWindow;
};

}  // namespace aidl::android::hardware::automotive::evs::implementation
