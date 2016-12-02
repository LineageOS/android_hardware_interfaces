LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := android.hardware.gnss@1.0-impl
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_SRC_FILES := \
    ThreadCreationWrapper.cpp \
    AGnss.cpp \
    AGnssRil.cpp \
    Gnss.cpp \
    GnssDebug.cpp \
    GnssGeofencing.cpp \
    GnssMeasurement.cpp \
    GnssNavigationMessage.cpp \
    GnssNi.cpp \
    GnssXtra.cpp \
    GnssConfiguration.cpp \
    GnssUtils.cpp

LOCAL_SHARED_LIBRARIES := \
    liblog \
    libhidlbase \
    libhidltransport \
    libhwbinder \
    libutils \
    android.hardware.gnss@1.0 \
    libhardware

include $(BUILD_SHARED_LIBRARY)
