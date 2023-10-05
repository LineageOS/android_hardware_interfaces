package android.hardware.wifi.supplicant;

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
}
