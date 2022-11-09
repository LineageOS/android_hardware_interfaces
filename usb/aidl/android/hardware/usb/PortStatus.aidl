/*
 * Copyright (C) 2021 The Android Open Source Project
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

package android.hardware.usb;

import android.hardware.usb.AltModeData;
import android.hardware.usb.ComplianceWarning;
import android.hardware.usb.ContaminantDetectionStatus;
import android.hardware.usb.ContaminantProtectionMode;
import android.hardware.usb.ContaminantProtectionStatus;
import android.hardware.usb.PlugOrientation;
import android.hardware.usb.PortDataRole;
import android.hardware.usb.PortMode;
import android.hardware.usb.PortPowerRole;
import android.hardware.usb.PowerBrickStatus;
import android.hardware.usb.UsbDataStatus;

@VintfStability
parcelable PortStatus {
    /**
     * Name of the port.
     * Used as the port's id by the caller.
     */
    String portName;
    /**
     * Data role of the port.
     */
    PortDataRole currentDataRole = PortDataRole.NONE;
    /**
     * Power Role of thte port.
     */
    PortPowerRole currentPowerRole = PortPowerRole.NONE;
    /**
     * Mode in which the port is connected.
     * Can be UFP or DFP or AUDIO_ACCESSORY or
     * DEBUG_ACCESSORY.
     */
    PortMode currentMode = PortMode.NONE;
    /**
     * True indicates that the port's mode can
     * be changed. False otherwise.
     */
    boolean canChangeMode;
    /**
     * True indicates that the port's data role
     * can be changed. False otherwise.
     * For example, true if Type-C PD PD_SWAP
     * is supported.
     */
    boolean canChangeDataRole;
    /**
     * True indicates that the port's power role
     * can be changed. False otherwise.
     * For example, true if Type-C PD PR_SWAP
     * is supported.
     */
    boolean canChangePowerRole;
    /**
     * Identifies the type of the local port.
     *
     * UFP - Indicates that port can only act as device for
     *       data and sink for power.
     * DFP - Indicates the port can only act as host for data
     *       and source for power.
     * DRP - Indicates can either act as UFP or DFP at a
     *       given point of time.
     * AUDIO_ACCESSORY -  Indicates that the port supports
     *                    Audio Accessory mode.
     * DEBUG_ACCESSORY - Indicates that the port supports
     *                   Debug Accessory mode.
     */
    PortMode[] supportedModes;
    /**
     * Contaminant presence protection modes supported by the port.
     */
    ContaminantProtectionMode[] supportedContaminantProtectionModes;
    /**
     * Client can enable/disable contaminant presence protection through
     * enableContaminantPresenceProtection when true.
     */
    boolean supportsEnableContaminantPresenceProtection;
    /**
     * Contaminant presence protection modes currently active for the port.
     */
    ContaminantProtectionStatus contaminantProtectionStatus = ContaminantProtectionStatus.NONE;
    /**
     * Client can enable/disable contaminant presence detection through
     * enableContaminantPresenceDetection when true.
     */
    boolean supportsEnableContaminantPresenceDetection;
    /**
     * Current status of contaminant detection algorithm.
     */
    ContaminantDetectionStatus contaminantDetectionStatus =
            ContaminantDetectionStatus.NOT_SUPPORTED;
    /**
     * UsbData status of the port.
     * Lists reasons for USB data being disabled.
     */
    UsbDataStatus[] usbDataStatus;
    /**
     * Denoted whether power transfer is limited in the port.
     */
    boolean powerTransferLimited;
    /**
     * Denotes whether Power brick is connected.
     */
    PowerBrickStatus powerBrickStatus;
    /**
     * True if the hal implementation can support identifying
     * non compliant USB power source/cable/accessory. False other
     * otherwise.
     */
    boolean supportsComplianceWarnings = false;
    /**
     * List of reasons as to why the attached USB
     * power source/cable/accessory is non compliant.
     */
    ComplianceWarning[] complianceWarnings = {};
    /**
     * Indicates the current orientation of the cable/adapter
     * plugged into the device.
     */
    PlugOrientation plugOrientation = PlugOrientation.UNKNOWN;
    /**
     * Lists Alt Modes supported by the device and holds their
     * current information.
     */
    AltModeData[] supportedAltModes = {};
}
