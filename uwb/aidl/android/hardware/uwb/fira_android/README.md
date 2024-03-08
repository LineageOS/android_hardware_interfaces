The `android.hardware.uwb.fira_android` package is used to add any Android specific
additions to the UCI specification defined by FIRA standards body. These
additions should be added to the vendor specific portions carved out in the UCI
specification.

These include:
 - Android specific GIDs/OIDs for commands/responses/notifications.
 - Andriod specific params in an existing UCI specified command/response/notification.

All other interactions sent/received over the HAL interface is expected to
comply with the UCI specification that can be found [here](
https://groups.firaconsortium.org/wg/Technical/document/folder/127).
