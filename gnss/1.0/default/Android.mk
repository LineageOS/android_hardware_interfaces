LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := android.hardware.gnss@1.0-impl
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_SRC_FILES := \
    ThreadCreationWrapper.cpp \
    AGnss.cpp \
    AGnssRil.cpp \
    Gnss.cpp \
    GnssBatching.cpp \
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

include $(CLEAR_VARS)
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_MODULE := android.hardware.gnss@1.0-service
LOCAL_INIT_RC := android.hardware.gnss@1.0-service.rc
LOCAL_SRC_FILES := \
    service.cpp \

LOCAL_SHARED_LIBRARIES := \
    liblog \
    libcutils \
    libdl \
    libbase \
    libutils \
    libhardware_legacy \
    libhardware \

LOCAL_SHARED_LIBRARIES += \
    libhwbinder \
    libhidlbase \
    libhidltransport \
    android.hardware.gnss@1.0 \

include $(BUILD_EXECUTABLE)
