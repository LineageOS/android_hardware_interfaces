# Rust Skeleton BroadcastRadio HAL implementation.

WARNING: This is not a reference BroadcastRadio HAL implementation and does
not contain any actual implementation.

This folder contains a skeleton broadcast radio HAL implementation in Rust to
demonstrate  how vendor may implement a Rust broadcast radio HAL. To run this
broadcast radio HAL, include `android.hardware.broadcastradio-rust-service`
in your image.

This implementation returns `StatusCode::UNKNOWN_ERROR` for all operations
and does not pass VTS/CTS. Vendor must replace the logic in
`default_broadcastradio_hal.rs` with the actual implementation