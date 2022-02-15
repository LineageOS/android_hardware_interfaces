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

#ifndef ANDROID_HARDWARE_AUTOMOTIVE_EVS_V1_1_DISPLAY_GLWRAPPER_H
#define ANDROID_HARDWARE_AUTOMOTIVE_EVS_V1_1_DISPLAY_GLWRAPPER_H

#include <android-base/logging.h>
#include <android/frameworks/automotive/display/1.0/IAutomotiveDisplayProxyService.h>
#include <android/hardware/automotive/evs/1.1/types.h>
#include <bufferqueueconverter/BufferQueueConverter.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>

namespace android::hardware::automotive::evs::V1_1::implementation {

using frameworks::automotive::display::V1_0::IAutomotiveDisplayProxyService;
using hardware::graphics::bufferqueue::V2_0::IGraphicBufferProducer;

class GlWrapper {
  public:
    GlWrapper() : mSurfaceHolder(android::SurfaceHolderUniquePtr(nullptr, nullptr)) {}
    bool initialize(const sp<IAutomotiveDisplayProxyService>& service, uint64_t displayId);
    void shutdown();

    bool updateImageTexture(const V1_0::BufferDesc& buffer);
    bool updateImageTexture(const BufferDesc& buffer);
    void renderImageToScreen();

    void showWindow(sp<IAutomotiveDisplayProxyService>& service, uint64_t id);
    void hideWindow(sp<IAutomotiveDisplayProxyService>& service, uint64_t id);

    unsigned getWidth() { return mWidth; };
    unsigned getHeight() { return mHeight; };

  private:
    sp<IGraphicBufferProducer> mGfxBufferProducer;

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

    // Pointer to a Surface wrapper.
    android::SurfaceHolderUniquePtr mSurfaceHolder;
};

}  // namespace android::hardware::automotive::evs::V1_1::implementation

#endif  // ANDROID_HARDWARE_AUTOMOTIVE_EVS_V1_1_DISPLAY_GLWRAPPER_H
