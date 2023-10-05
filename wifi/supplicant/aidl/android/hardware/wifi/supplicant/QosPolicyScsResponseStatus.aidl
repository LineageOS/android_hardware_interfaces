package android.hardware.wifi.supplicant;

import android.hardware.wifi.supplicant.QosPolicyScsResponseStatusCode;

/**
 * QoS policy status info per scsId. Returned in a callback once replies are
 * received from the AP.
 */
@VintfStability
parcelable QosPolicyScsResponseStatus {
    byte policyId;
    QosPolicyScsResponseStatusCode qosPolicyScsResponseStatusCode;
}
