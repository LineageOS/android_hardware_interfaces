use android_hardware_uwb::aidl::android::hardware::uwb::{IUwb, IUwbChip};
use android_hardware_uwb::binder;
use binder::{Result, Strong};
use binder_tokio::TokioRuntime;
use tokio::runtime::Handle as TokioHandle;

use crate::uwb_chip;

pub struct Uwb {
    chips: Vec<Strong<dyn IUwbChip::IUwbChip>>,
}

impl Uwb {
    pub fn from_chips(
        chips: impl IntoIterator<Item = uwb_chip::UwbChip>,
        handle: TokioHandle,
    ) -> Self {
        Self {
            chips: chips
                .into_iter()
                .map(|chip| {
                    IUwbChip::BnUwbChip::new_async_binder(
                        chip,
                        TokioRuntime(handle.clone()),
                        binder::BinderFeatures::default(),
                    )
                })
                .collect(),
        }
    }
}

impl binder::Interface for Uwb {}

impl IUwb::IUwb for Uwb {
    fn getChips(&self) -> Result<Vec<String>> {
        log::debug!("getChips");
        self.chips.iter().map(|chip| chip.getName()).collect()
    }

    fn getChip(&self, name: &str) -> Result<Strong<dyn IUwbChip::IUwbChip>> {
        log::debug!("getChip {}", name);
        let chip = self
            .chips
            .iter()
            .find(|chip| chip.getName().as_deref() == Ok(name));
        if let Some(chip) = chip {
            Ok(chip.clone())
        } else {
            Err(binder::ExceptionCode::ILLEGAL_ARGUMENT.into())
        }
    }
}
