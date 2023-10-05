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

#include "GlWrapper.h"

#include <ui/DisplayMode.h>
#include <ui/DisplayState.h>
#include <ui/GraphicBuffer.h>

#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <utility>

using android::GraphicBuffer;
using android::sp;

namespace {

// Defines a default color to clear the screen in RGBA format
constexpr float kDefaultColorInRgba[] = {0.1f, 0.5f, 0.1f, 1.0f};

// Defines the size of the preview area relative to the entire display
constexpr float kDisplayAreaRatio = 0.8f;

constexpr const char vertexShaderSource[] =
        "attribute vec4 pos;                    \n"
        "attribute vec2 tex;                    \n"
        "varying vec2 uv;                       \n"
        "void main()                            \n"
        "{                                      \n"
        "   gl_Position = pos;                  \n"
        "   uv = tex;                           \n"
        "}                                      \n";

constexpr const char pixelShaderSource[] =
        "precision mediump float;               \n"
        "uniform sampler2D tex;                 \n"
        "varying vec2 uv;                       \n"
        "void main()                            \n"
        "{                                      \n"
        "    gl_FragColor = texture2D(tex, uv); \n"
        "}                                      \n";

const char* getEGLError(void) {
    switch (eglGetError()) {
        case EGL_SUCCESS:
            return "EGL_SUCCESS";
        case EGL_NOT_INITIALIZED:
            return "EGL_NOT_INITIALIZED";
        case EGL_BAD_ACCESS:
            return "EGL_BAD_ACCESS";
        case EGL_BAD_ALLOC:
            return "EGL_BAD_ALLOC";
        case EGL_BAD_ATTRIBUTE:
            return "EGL_BAD_ATTRIBUTE";
        case EGL_BAD_CONTEXT:
            return "EGL_BAD_CONTEXT";
        case EGL_BAD_CONFIG:
            return "EGL_BAD_CONFIG";
        case EGL_BAD_CURRENT_SURFACE:
            return "EGL_BAD_CURRENT_SURFACE";
        case EGL_BAD_DISPLAY:
            return "EGL_BAD_DISPLAY";
        case EGL_BAD_SURFACE:
            return "EGL_BAD_SURFACE";
        case EGL_BAD_MATCH:
            return "EGL_BAD_MATCH";
        case EGL_BAD_PARAMETER:
            return "EGL_BAD_PARAMETER";
        case EGL_BAD_NATIVE_PIXMAP:
            return "EGL_BAD_NATIVE_PIXMAP";
        case EGL_BAD_NATIVE_WINDOW:
            return "EGL_BAD_NATIVE_WINDOW";
        case EGL_CONTEXT_LOST:
            return "EGL_CONTEXT_LOST";
        default:
            return "Unknown error";
    }
}

// Given shader source, load and compile it
GLuint loadShader(GLenum type, const char* shaderSrc) {
    // Create the shader object
    GLuint shader = glCreateShader(type);
    if (shader == 0) {
        LOG(ERROR) << "glCreateSharder() failed with error = " << glGetError();
        return 0;
    }

    // Load and compile the shader
    glShaderSource(shader, 1, &shaderSrc, nullptr);
    glCompileShader(shader);

    // Verify the compilation worked as expected
    GLint compiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        LOG(ERROR) << "Error compiling shader";

        GLint size = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &size);
        if (size > 0) {
            // Get and report the error message
            char infoLog[size];
            glGetShaderInfoLog(shader, size, nullptr, infoLog);
            LOG(ERROR) << "  msg:" << std::endl << infoLog;
        }

        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

// Create a program object given vertex and pixels shader source
GLuint buildShaderProgram(const char* vtxSrc, const char* pxlSrc) {
    GLuint program = glCreateProgram();
    if (program == 0) {
        LOG(ERROR) << "Failed to allocate program object";
        return 0;
    }

    // Compile the shaders and bind them to this program
    GLuint vertexShader = loadShader(GL_VERTEX_SHADER, vtxSrc);
    if (vertexShader == 0) {
        LOG(ERROR) << "Failed to load vertex shader";
        glDeleteProgram(program);
        return 0;
    }
    GLuint pixelShader = loadShader(GL_FRAGMENT_SHADER, pxlSrc);
    if (pixelShader == 0) {
        LOG(ERROR) << "Failed to load pixel shader";
        glDeleteProgram(program);
        glDeleteShader(vertexShader);
        return 0;
    }
    glAttachShader(program, vertexShader);
    glAttachShader(program, pixelShader);

    glBindAttribLocation(program, 0, "pos");
    glBindAttribLocation(program, 1, "tex");

    // Link the program
    glLinkProgram(program);
    GLint linked = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked) {
        LOG(ERROR) << "Error linking program";
        GLint size = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &size);
        if (size > 0) {
            // Get and report the error message
            char* infoLog = (char*)malloc(size);
            glGetProgramInfoLog(program, size, nullptr, infoLog);
            LOG(ERROR) << "  msg:  " << infoLog;
            free(infoLog);
        }

        glDeleteProgram(program);
        glDeleteShader(vertexShader);
        glDeleteShader(pixelShader);
        return 0;
    }

    return program;
}

}  // namespace

namespace android::hardware::automotive::evs::V1_1::implementation {

// Main entry point
bool GlWrapper::initialize(const sp<IAutomotiveDisplayProxyService>& service, uint64_t displayId) {
    LOG(DEBUG) << __FUNCTION__;

    if (!service) {
        LOG(WARNING) << "IAutomotiveDisplayProxyService is invalid.";
        return false;
    }

    // We will use the first display in the list as the primary.
    service->getDisplayInfo(displayId, [this](auto dpyConfig, auto dpyState) {
        ui::DisplayMode* pConfig = reinterpret_cast<ui::DisplayMode*>(dpyConfig.data());
        mWidth = pConfig->resolution.getWidth();
        mHeight = pConfig->resolution.getHeight();

        ui::DisplayState* pState = reinterpret_cast<ui::DisplayState*>(dpyState.data());
        if (pState->orientation != ui::ROTATION_0 && pState->orientation != ui::ROTATION_180) {
            // rotate
            std::swap(mWidth, mHeight);
        }

        LOG(DEBUG) << "Display resolution is " << mWidth << " x " << mHeight;
    });

    mGfxBufferProducer = service->getIGraphicBufferProducer(displayId);
    if (mGfxBufferProducer == nullptr) {
        LOG(ERROR) << "Failed to get IGraphicBufferProducer from IAutomotiveDisplayProxyService.";
        return false;
    }

    mSurfaceHolder = getSurfaceFromHGBP(mGfxBufferProducer);
    if (mSurfaceHolder == nullptr) {
        LOG(ERROR) << "Failed to get a Surface from HGBP.";
        return false;
    }

    mWindow = getNativeWindow(mSurfaceHolder.get());
    if (mWindow == nullptr) {
        LOG(ERROR) << "Failed to get a native window from Surface.";
        return false;
    }

    // Set up our OpenGL ES context associated with the default display
    mDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (mDisplay == EGL_NO_DISPLAY) {
        LOG(ERROR) << "Failed to get egl display";
        return false;
    }

    EGLint major = 2;
    EGLint minor = 0;
    if (!eglInitialize(mDisplay, &major, &minor)) {
        LOG(ERROR) << "Failed to initialize EGL: " << getEGLError();
        return false;
    }

    const EGLint config_attribs[] = {
            // Tag                  Value
            EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_DEPTH_SIZE, 0, EGL_NONE};

    // Pick the default configuration without constraints (is this good enough?)
    EGLConfig egl_config = {0};
    EGLint numConfigs = -1;
    eglChooseConfig(mDisplay, config_attribs, &egl_config, 1, &numConfigs);
    if (numConfigs != 1) {
        LOG(ERROR) << "Didn't find a suitable format for our display window";
        return false;
    }

    // Create the EGL render target surface
    mSurface = eglCreateWindowSurface(mDisplay, egl_config, mWindow, nullptr);
    if (mSurface == EGL_NO_SURFACE) {
        LOG(ERROR) << "eglCreateWindowSurface failed: " << getEGLError();
        ;
        return false;
    }

    // Create the EGL context
    // NOTE:  Our shader is (currently at least) written to require version 3, so this
    //        is required.
    const EGLint context_attribs[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    mContext = eglCreateContext(mDisplay, egl_config, EGL_NO_CONTEXT, context_attribs);
    if (mContext == EGL_NO_CONTEXT) {
        LOG(ERROR) << "Failed to create OpenGL ES Context: " << getEGLError();
        return false;
    }

    // Activate our render target for drawing
    if (!eglMakeCurrent(mDisplay, mSurface, mSurface, mContext)) {
        LOG(ERROR) << "Failed to make the OpenGL ES Context current: " << getEGLError();
        return false;
    }

    // Create the shader program for our simple pipeline
    mShaderProgram = buildShaderProgram(vertexShaderSource, pixelShaderSource);
    if (!mShaderProgram) {
        LOG(ERROR) << "Failed to build shader program: " << getEGLError();
        return false;
    }

    // Create a GL texture that will eventually wrap our externally created texture surface(s)
    glGenTextures(1, &mTextureMap);
    if (mTextureMap <= 0) {
        LOG(ERROR) << "Didn't get a texture handle allocated: " << getEGLError();
        return false;
    }

    // Turn off mip-mapping for the created texture surface
    // (the inbound camera imagery doesn't have MIPs)
    glBindTexture(GL_TEXTURE_2D, mTextureMap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    return true;
}

void GlWrapper::shutdown() {
    // Drop our device textures
    if (mKHRimage != EGL_NO_IMAGE_KHR) {
        eglDestroyImageKHR(mDisplay, mKHRimage);
        mKHRimage = EGL_NO_IMAGE_KHR;
    }

    // Release all GL resources
    eglMakeCurrent(mDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroySurface(mDisplay, mSurface);
    eglDestroyContext(mDisplay, mContext);
    eglTerminate(mDisplay);
    mSurface = EGL_NO_SURFACE;
    mContext = EGL_NO_CONTEXT;
    mDisplay = EGL_NO_DISPLAY;

    // Release the window
    mSurfaceHolder = nullptr;
}

void GlWrapper::showWindow(sp<IAutomotiveDisplayProxyService>& service, uint64_t id) {
    if (service != nullptr) {
        service->showWindow(id);
    } else {
        LOG(ERROR) << "IAutomotiveDisplayProxyService is not available.";
    }
}

void GlWrapper::hideWindow(sp<IAutomotiveDisplayProxyService>& service, uint64_t id) {
    if (service != nullptr) {
        service->hideWindow(id);
    } else {
        LOG(ERROR) << "IAutomotiveDisplayProxyService is not available.";
    }
}

bool GlWrapper::updateImageTexture(const V1_0::BufferDesc& buffer) {
    BufferDesc newBuffer = {
            .buffer =
                    {
                            .nativeHandle = buffer.memHandle,
                    },
            .pixelSize = buffer.pixelSize,
            .bufferId = buffer.bufferId,
    };
    AHardwareBuffer_Desc* pDesc =
            reinterpret_cast<AHardwareBuffer_Desc*>(&newBuffer.buffer.description);
    *pDesc = {
            .width = buffer.width,
            .height = buffer.height,
            .layers = 1,
            .format = buffer.format,
            .usage = buffer.usage,
    };
    return updateImageTexture(newBuffer);
}

bool GlWrapper::updateImageTexture(const BufferDesc& aFrame) {
    // If we haven't done it yet, create an "image" object to wrap the gralloc buffer
    if (mKHRimage == EGL_NO_IMAGE_KHR) {
        // create a temporary GraphicBuffer to wrap the provided handle
        const AHardwareBuffer_Desc* pDesc =
                reinterpret_cast<const AHardwareBuffer_Desc*>(&aFrame.buffer.description);
        sp<GraphicBuffer> pGfxBuffer = new GraphicBuffer(
                pDesc->width, pDesc->height, pDesc->format, pDesc->layers, pDesc->usage,
                pDesc->stride,
                const_cast<native_handle_t*>(aFrame.buffer.nativeHandle.getNativeHandle()),
                false /* keep ownership */
        );
        if (pGfxBuffer.get() == nullptr) {
            LOG(ERROR) << "Failed to allocate GraphicBuffer to wrap our native handle";
            return false;
        }

        // Get a GL compatible reference to the graphics buffer we've been given
        EGLint eglImageAttributes[] = {EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, EGL_NONE};
        EGLClientBuffer cbuf = static_cast<EGLClientBuffer>(pGfxBuffer->getNativeBuffer());
        mKHRimage = eglCreateImageKHR(mDisplay, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID, cbuf,
                                      eglImageAttributes);
        if (mKHRimage == EGL_NO_IMAGE_KHR) {
            LOG(ERROR) << "Error creating EGLImage: " << getEGLError();
            return false;
        }

        // Update the texture handle we already created to refer to this gralloc buffer
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mTextureMap);
        glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, static_cast<GLeglImageOES>(mKHRimage));
    }

    return true;
}

void GlWrapper::renderImageToScreen() {
    // Set the viewport
    glViewport(0, 0, mWidth, mHeight);

    // Clear the color buffer
    glClearColor(kDefaultColorInRgba[0], kDefaultColorInRgba[1],
                 kDefaultColorInRgba[2], kDefaultColorInRgba[3]);
    glClear(GL_COLOR_BUFFER_BIT);

    // Select our screen space simple texture shader
    glUseProgram(mShaderProgram);

    // Bind the texture and assign it to the shader's sampler
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mTextureMap);
    GLint sampler = glGetUniformLocation(mShaderProgram, "tex");
    glUniform1i(sampler, 0);

    // We want our image to show up opaque regardless of alpha values
    glDisable(GL_BLEND);

    // Draw a rectangle on the screen
    GLfloat vertsCarPos[] = {
            -kDisplayAreaRatio,  kDisplayAreaRatio, 0.0f,  // left top in window space
             kDisplayAreaRatio,  kDisplayAreaRatio, 0.0f,  // right top
            -kDisplayAreaRatio, -kDisplayAreaRatio, 0.0f,  // left bottom
             kDisplayAreaRatio, -kDisplayAreaRatio, 0.0f   // right bottom
    };

    // NOTE:  We didn't flip the image in the texture, so V=0 is actually the top of the image
    GLfloat vertsCarTex[] = {
            0.0f, 0.0f,  // left top
            1.0f, 0.0f,  // right top
            0.0f, 1.0f,  // left bottom
            1.0f, 1.0f   // right bottom
    };
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vertsCarPos);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, vertsCarTex);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // Clean up and flip the rendered result to the front so it is visible
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);

    glFinish();

    eglSwapBuffers(mDisplay, mSurface);
}

}  // namespace android::hardware::automotive::evs::V1_1::implementation
