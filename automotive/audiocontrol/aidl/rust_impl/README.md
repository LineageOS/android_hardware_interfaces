# Rust Skeleton Audio Control HAL implementation.

WARNING: This is not a reference audio control HAl implementation and does
not contain any actual implementation.

This folder contains a skeleton audio control HAL implementation in Rust to
demonstrate  how vendor may implement a Rust audio control HAL. To run this
audio control HAL, include
`android.hardware.automotive.audiocontrol-V4-rust-service` in your image.

This implementation returns `StatusCode::UNKNOWN_ERROR` for all operations
and does not pass VTS/CTS. Vendor must replace the logic in
`default_audio_control_hal.rs` with the actual implementation.
