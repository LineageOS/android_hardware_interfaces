# Rust Skeleton VHAL implementation.

WARNING: This is not a reference VHAL implementation and does not contain
any actual implementation.

This folder contains a skeleton VHAL implementation in Rust to demonstrate
how vendor may implement a Rust VHAL. To run this VHAL, include
`android.hardware.automotive.vehicle-V3-rust-service` in your image.

This implementation returns `StatusCode::UNKNOWN_ERROR` for all operations
and does not pass VTS/CTS. Vendor must replace the logic in
`default_vehicle_hal.rs` with the actual implementation.
