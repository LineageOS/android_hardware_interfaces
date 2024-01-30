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
 *
 *
 * This interface is used by telephony and telecom to talk to cellular radio for the purpose of
 * radio configuration, and it is not associated with any specific modem or slot.
 * All the functions have minimum one parameter:
 * serial: which corresponds to serial no. of request. Serial numbers must only be memorized for the
 * duration of a method call. If clients provide colliding serials (including passing the same
 * serial to different methods), multiple responses (one for each method call) must still be served.
 */

package android.hardware.radio.config;

import android.hardware.radio.config.IRadioConfigIndication;
import android.hardware.radio.config.IRadioConfigResponse;
import android.hardware.radio.config.SlotPortMapping;

/** @hide */
@VintfStability
oneway interface IRadioConfig {
    /**
     * Gets the available Radio Hal capabilities on the current device.
     *
     * This is called once per device boot up.
     *
     * @param serial Serial number of request
     *
     * Response callback is
     * IRadioConfigResponse.getHalDeviceCapabilitiesResponse()
     *
     * This is available when android.hardware.telephony is defined.
     */
    void getHalDeviceCapabilities(in int serial);

    /**
     * Get the number of live modems (i.e modems that are
     * enabled and actively working as part of a working telephony stack)
     *
     * Note: in order to get the overall number of modems available on the phone,
     * refer to getPhoneCapability API
     *
     * @param serial Serial number of request.
     *
     * Response callback is IRadioConfigResponse.getNumOfLiveModemsResponse() which
     * will return <byte>.
     *
     * This is available when android.hardware.telephony is defined.
     */
    void getNumOfLiveModems(in int serial);

    /**
     * Request current phone capability.
     *
     * @param serial Serial number of request.
     *
     * Response callback is IRadioResponse.getPhoneCapabilityResponse() which
     * will return <PhoneCapability>.
     *
     * This is available when android.hardware.telephony is defined.
     */
    void getPhoneCapability(in int serial);

    /**
     * Get SIM Slot status.
     *
     * Request provides the slot status of all active and inactive SIM slots and whether card is
     * present in the slots or not.
     *
     * @param serial Serial number of request.
     *
     * Response callback is IRadioConfigResponse.getSimSlotsStatusResponse()
     *
     * This is available when android.hardware.telephony.subscription is defined.
     */
    void getSimSlotsStatus(in int serial);

    /**
     * Set modems configurations by specifying the number of live modems (i.e modems that are
     * enabled and actively working as part of a working telephony stack).
     *
     * Example: this interface can be used to switch to single/multi sim mode by specifying
     * the number of live modems as 1, 2, etc
     *
     * Note: by setting the number of live modems in this API, that number of modems will
     * subsequently get enabled/disabled
     *
     * @param serial serial number of request.
     * @param modemsConfig byte object including the number of live modems
     *
     * Response callback is IRadioResponse.setNumOfLiveModemsResponse()
     *
     * This is available when android.hardware.telephony is defined.
     */
    void setNumOfLiveModems(in int serial, in byte numOfLiveModems);

    /**
     * Set preferred data modem Id.
     * In a multi-SIM device, notify modem layer which logical modem will be used primarily
     * for data. It helps modem with resource optimization and decisions of what data connections
     * should be satisfied.
     *
     * @param serial Serial number of request.
     * @param modem Id the logical modem ID, which should match one of modem IDs returned
     * from getPhoneCapability().
     *
     * Response callback is IRadioConfigResponse.setPreferredDataModemResponse()
     *
     * This is available when android.hardware.telephony.data is defined.
     */
    void setPreferredDataModem(in int serial, in byte modemId);

    /**
     * Set response functions for radio config requests & radio config indications.
     *
     * @param radioConfigResponse Object containing radio config response functions
     * @param radioConfigIndication Object containing radio config indications
     *
     * This is available when android.hardware.telephony is defined.
     */
    void setResponseFunctions(in IRadioConfigResponse radioConfigResponse,
            in IRadioConfigIndication radioConfigIndication);

    /**
     * Set SIM Slot mapping.
     *
     * Maps the logical slots to the SlotPortMapping which consist of both physical slot id and port
     * id. Logical slot is the slot that is seen by modem. Physical slot is the actual physical
     * slot. PortId is the id (enumerated value) for the associated port available on the SIM. Each
     * physical slot can have multiple ports which enables multi-enabled profile(MEP). If eUICC
     * physical slot supports 2 ports, then the portId is numbered 0,1 and if eUICC2 supports 4
     * ports then the portID is numbered 0,1,2,3. Each portId is unique within a UICC physical slot
     * but not necessarily unique across UICC’s. SEP(Single enabled profile) eUICC and non-eUICC
     * will only have portId 0.
     *
     * Logical slots that are already mapped to the requested SlotPortMapping are not impacted.
     *
     * Example no. of logical slots 1 and physical slots 2 do not support MEP, each physical slot
     * has one port:
     * The only logical slot (index 0) can be mapped to first physical slot (value 0), port(index
     * 0). or second
     * physical slot(value 1), port (index 0), while the other physical slot remains unmapped and
     * inactive.
     * slotMap[0] = SlotPortMapping{0 //physical slot//, 0 //port//}
     * slotMap[0] = SlotPortMapping{1 //physical slot//, 0 //port//}
     *
     * Example no. of logical slots 2 and physical slots 2 supports MEP with 2 ports available:
     * Each logical slot must be mapped to a port (physical slot and port combination).
     * First logical slot (index 0) can be mapped to physical slot 1 and the second logical slot
     * can be mapped to either port from physical slot 2.
     *
     * slotMap[0] = SlotPortMapping{0, 0} and slotMap[1] = SlotPortMapping{1, 0} or
     * slotMap[0] = SlotPortMapping{0, 0} and slotMap[1] = SlotPortMapping{1, 1}
     *
     * or the other way around, the second logical slot(index 1) can be mapped to physical slot 1
     * and the first logical slot can be mapped to either port from physical slot 2.
     *
     * slotMap[1] = SlotPortMapping{0, 0} and slotMap[0] = SlotPortMapping{1, 0} or
     * slotMap[1] = SlotPortMapping{0, 0} and slotMap[0] = SlotPortMapping{1, 1}
     *
     * another possible mapping is each logical slot maps to each port of physical slot 2 and there
     * is no active logical modem mapped to physical slot 1.
     *
     * slotMap[0] = SlotPortMapping{1, 0} and slotMap[1] = SlotPortMapping{1, 1} or
     * slotMap[0] = SlotPortMapping{1, 1} and slotMap[1] = SlotPortMapping{1, 0}
     *
     * @param serial Serial number of request
     * @param slotMap Logical to physical slot and port mapping.
     *        Index is mapping to logical slot and value to physical slot and port id, need to
     *        provide all the slots mapping when sending request in case of multi slot device.
     *
     *        EX: SlotPortMapping(physical slot, port id)
     *        index 0 is the first logical_slot number of logical slots is equal to number of Radio
     *        instances and number of physical slots is equal to size of slotStatus in
     *        getSimSlotsStatusResponse
     *
     * Response callback is IRadioConfigResponse.setSimSlotsMappingResponse()
     *
     * This is available when android.hardware.telephony.subscription is defined.
     */
    void setSimSlotsMapping(in int serial, in SlotPortMapping[] slotMap);
}
