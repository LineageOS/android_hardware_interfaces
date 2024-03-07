package android.hardware.wifi.supplicant;

import android.hardware.wifi.supplicant.QosCharacteristics;
import android.hardware.wifi.supplicant.QosPolicyClassifierParams;

/**
 * QoS policy information in SCS request.
 * TCLAS Processing element is always set to 0.
 */
@VintfStability
parcelable QosPolicyScsData {
    /** SCS QoS Policy identifier. */
    byte policyId;

    /**
     * User Priority (UP) which the AP should apply to streams that match
     * the classifier parameters.
     */
    byte userPriority;

    /**
     * QoS policy SCS classifier type information.
     */
    QosPolicyClassifierParams classifierParams;

    /**
     * Enum values for the |direction| field.
     */
    @VintfStability
    @Backing(type="byte")
    enum LinkDirection {
        DOWNLINK,
        UPLINK, // Only applies to trigger-based traffic (Wi-Fi 6+)
    }

    /**
     * Direction of data described by this element.
     */
    LinkDirection direction;

    /**
     * Additional parameters available in QoS R3.
     */
    @nullable QosCharacteristics QosCharacteristics;
}
