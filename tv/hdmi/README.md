# HDMI-related AIDL HALs for Android devices

This directory bundles 3 HDMI-related AIDL HALs: HDMI Connection HAL, CEC HAL and eARC HAL.

The HDMI Connection HAL contains general APIs for the HDMI Connection. These methods are required by
the CEC and the eARC implementation. Therefore, devices that implement CEC need to support the HDMI
Connection HAL and the CEC HAL. Devices that implement eARC need to support the HDMI Connection HAL
and the eARC HAL.

Other Android HALs are related to HDMI as well, but not included in this directory for historical
reasons, e.g. Display HAL and TV Input HAL.
