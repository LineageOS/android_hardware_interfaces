package android.hardware.wifi.supplicant;

import android.hardware.wifi.supplicant.QosPolicyScsRequestStatusCode;

/**
 * QoS policy status info per scsId. Returned immediately by supplicant
 * upon SCS request.
 */
@VintfStability
parcelable QosPolicyScsRequestStatus {
    byte policyId;
    QosPolicyScsRequestStatusCode qosPolicyScsRequestStatusCode;
}
