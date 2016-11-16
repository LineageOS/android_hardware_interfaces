LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := android.hardware.audio.effect@2.0-impl
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_SRC_FILES := \
    AcousticEchoCancelerEffect.cpp \
    AutomaticGainControlEffect.cpp \
    BassBoostEffect.cpp \
    Conversions.cpp \
    DownmixEffect.cpp \
    Effect.cpp \
    EffectsFactory.cpp \
    EnvironmentalReverbEffect.cpp \
    EqualizerEffect.cpp \
    LoudnessEnhancerEffect.cpp \
    NoiseSuppressionEffect.cpp \
    PresetReverbEffect.cpp \
    VirtualizerEffect.cpp \
    VisualizerEffect.cpp \

LOCAL_SHARED_LIBRARIES := \
    libhidlbase \
    libhidltransport \
    libhwbinder \
    libutils \
    libeffects \
    liblog \
    android.hardware.audio.common@2.0 \
    android.hardware.audio.common@2.0-util \
    android.hardware.audio.effect@2.0 \

include $(BUILD_SHARED_LIBRARY)
