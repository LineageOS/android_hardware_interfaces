mod default_vehicle_hal;

use android_hardware_automotive_vehicle::aidl::android::hardware::automotive::vehicle::IVehicle::BnVehicle;
use crate::default_vehicle_hal::DefaultVehicleHal;

fn main() {
	binder::ProcessState::start_thread_pool();
	let my_service = DefaultVehicleHal;
	let service_name = "android.hardware.automotive.vehicle.IVehicle/default";
    let my_service_binder = BnVehicle::new_binder(
        my_service,
        binder::BinderFeatures::default(),
    );
    binder::add_service(service_name, my_service_binder.as_binder())
    		.expect(format!("Failed to register {}?", service_name).as_str());
    // Does not return.
    binder::ProcessState::join_thread_pool()
}
