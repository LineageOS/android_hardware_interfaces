use android_hardware_automotive_vehicle::aidl::android::hardware::automotive::vehicle::{
    IVehicle::IVehicle,
    IVehicleCallback::IVehicleCallback,
    VehiclePropConfigs::VehiclePropConfigs,
    GetValueRequests::GetValueRequests,
    SetValueRequests::SetValueRequests,
    SubscribeOptions::SubscribeOptions,
};
use binder::{Interface, Result as BinderResult, StatusCode, Strong};

/// This struct is defined to implement IVehicle AIDL interface.
pub struct DefaultVehicleHal;

impl Interface for DefaultVehicleHal {}

impl IVehicle for DefaultVehicleHal {
    fn getAllPropConfigs(&self) -> BinderResult<VehiclePropConfigs> {
        Err(StatusCode::UNKNOWN_ERROR.into())
    }

    fn getPropConfigs(&self, _props: &[i32]) -> BinderResult<VehiclePropConfigs> {
        Err(StatusCode::UNKNOWN_ERROR.into())
    }

    fn getValues(
            &self, _callback: &Strong<dyn IVehicleCallback>, _requests: &GetValueRequests
        ) -> BinderResult<()> {
        Err(StatusCode::UNKNOWN_ERROR.into())
    }

    fn setValues(
            &self, _callback: &Strong<dyn IVehicleCallback>, _requests: &SetValueRequests
        ) -> BinderResult<()> {
        Err(StatusCode::UNKNOWN_ERROR.into())
    }

    fn subscribe(
            &self, _callback: &Strong<dyn IVehicleCallback>, _options: &[SubscribeOptions],
            _max_shared_memory_file_count: i32
        ) -> BinderResult<()> {
        Err(StatusCode::UNKNOWN_ERROR.into())
    }

    fn unsubscribe(
            &self, _callback: &Strong<dyn IVehicleCallback>, _prop_ids: &[i32]
        ) -> BinderResult<()> {
        Err(StatusCode::UNKNOWN_ERROR.into())
    }

    fn returnSharedMemory(
            &self, _callback: &Strong<dyn IVehicleCallback>, _shared_memory_id: i64
        ) -> BinderResult<()> {
        Err(StatusCode::UNKNOWN_ERROR.into())
    }
}
