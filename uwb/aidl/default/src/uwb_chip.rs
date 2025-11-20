use android_hardware_uwb::aidl::android::hardware::uwb::{
    IUwbChip::IUwbChipAsyncServer, IUwbClientCallback::IUwbClientCallback, UwbEvent::UwbEvent,
    UwbStatus::UwbStatus,
};
use android_hardware_uwb::binder;
use async_trait::async_trait;
use binder::{DeathRecipient, IBinder, Result, Strong};

use std::sync::Arc;
use tokio::fs;
use tokio::io::{AsyncReadExt, AsyncWriteExt};
use tokio::sync::Mutex;

enum ClientState {
    Closed,
    Opened {
        callbacks: Strong<dyn IUwbClientCallback>,
        _death_recipient: DeathRecipient,
        initialized: bool,
    },
}

struct ServiceState {
    client_state: ClientState,
    writer: fs::File,
}

pub struct UwbChip {
    name: String,
    _handle: tokio::task::JoinHandle<()>,
    service_state: Arc<Mutex<ServiceState>>,
}

/// Configure a file descriptor as raw fd.
pub fn makeraw(file: fs::File) -> std::io::Result<fs::File> {
    use nix::sys::termios::*;
    let mut attrs = tcgetattr(&file)?;
    cfmakeraw(&mut attrs);
    tcsetattr(&file, SetArg::TCSANOW, &attrs)?;
    Ok(file)
}

impl UwbChip {
    pub async fn new(name: String, path: String) -> Self {
        // Open the serial file and configure it as raw file
        // descriptor.
        let mut reader = fs::OpenOptions::new()
            .read(true)
            .write(true)
            .create(false)
            .open(&path)
            .await
            .and_then(makeraw)
            .expect("failed to open the serial device");
        let writer = reader
            .try_clone()
            .await
            .expect("failed to clone serial for writing");

        // Create the chip
        let service_state = Arc::new(Mutex::new(ServiceState {
            writer,
            client_state: ClientState::Closed,
        }));

        // Spawn the task that will run the polling loop.
        let handle = {
            let service_state = service_state.clone();

            tokio::task::spawn(async move {
                log::info!("UCI reader task started");

                const MESSAGE_TYPE_MASK: u8 = 0b11100000;
                const DATA_MESSAGE_TYPE: u8 = 0b000;
                const UCI_HEADER_SIZE: usize = 4;
                const UCI_BUFFER_SIZE: usize = 1024;

                let mut buffer = [0; UCI_BUFFER_SIZE];

                loop {
                    reader
                        .read_exact(&mut buffer[0..UCI_HEADER_SIZE])
                        .await
                        .expect("failed to read uci header bytes");
                    let common_header = buffer[0];
                    let mt = (common_header & MESSAGE_TYPE_MASK) >> 5;
                    let payload_length = if mt == DATA_MESSAGE_TYPE {
                        u16::from_le_bytes([buffer[2], buffer[3]]) as usize
                    } else {
                        buffer[3] as usize
                    };

                    let total_packet_length = payload_length + UCI_HEADER_SIZE;
                    reader
                        .read_exact(&mut buffer[UCI_HEADER_SIZE..total_packet_length])
                        .await
                        .expect("failed to read uci payload bytes");

                    log::debug!(" <-- {:?}", &buffer[0..total_packet_length]);

                    let mut service_state = service_state.lock().await;
                    if let ClientState::Opened { ref callbacks, ref mut initialized, .. } = service_state.client_state {
                        if !*initialized {
                            if matches!(&buffer[0..total_packet_length], [0x40, 0x00, 0x00, 0x01, 0x00]
                            ) {
                                *initialized = true;
                                callbacks.onHalEvent(UwbEvent::OPEN_CPLT, UwbStatus::OK)
                                    .expect("failed to call onHalEvent");
                            }
                        } else {
                            callbacks
                            .onUciMessage(&buffer[0..total_packet_length])
                            .unwrap();
                        }
                    }
                }
            })
        };

        Self {
            name,
            _handle: handle,
            service_state,
        }
    }
}
impl binder::Interface for UwbChip {}

#[async_trait]
impl IUwbChipAsyncServer for UwbChip {
    async fn getName(&self) -> Result<String> {
        Ok(self.name.clone())
    }

    async fn open(&self, callbacks: &Strong<dyn IUwbClientCallback>) -> Result<()> {
        log::debug!("open");

        let mut service_state = self.service_state.lock().await;

        if matches!(service_state.client_state, ClientState::Opened { .. }) {
            log::error!("the state is already opened");
            return Err(binder::ExceptionCode::ILLEGAL_STATE.into());
        }

        let mut death_recipient = {
            let service_state = self.service_state.clone();
            DeathRecipient::new(move || {
                log::info!("Uwb service has died");
                let mut service_state = service_state.blocking_lock();
                service_state.client_state = ClientState::Closed;
            })
        };

        callbacks.as_binder().link_to_death(&mut death_recipient)?;

        service_state.client_state = ClientState::Opened {
            callbacks: callbacks.clone(),
            _death_recipient: death_recipient,
            initialized: false,
        };

        // Send the command Device Reset to stop all running activities
        // on the UWBS emulator. This is necessary because the emulator
        // is otherwise not notified of the power down (the serial stays
        // open).
        //
        // The response to the command will be handled so it won't be sent
        // to uci layer.
        let uci_core_device_reset_cmd = [0x20, 0x00, 0x00, 0x01, 0x00];

        service_state
            .writer
            .write_all(&uci_core_device_reset_cmd)
            .await
            .expect("failed to write UCI Device Reset command");

        Ok(())
    }

    async fn close(&self) -> Result<()> {
        log::debug!("close");

        let mut service_state = self.service_state.lock().await;

        if matches!(service_state.client_state, ClientState::Closed) {
            log::error!("the state is already closed");
            return Err(binder::ExceptionCode::ILLEGAL_STATE.into());
        }

        if let ClientState::Opened { ref callbacks, .. } = service_state.client_state {
            callbacks.onHalEvent(UwbEvent::CLOSE_CPLT, UwbStatus::OK)?;
        }

        service_state.client_state = ClientState::Closed;
        Ok(())
    }

    async fn coreInit(&self) -> Result<()> {
        log::debug!("coreInit");

        let service_state = self.service_state.lock().await;

        if let ClientState::Opened { ref callbacks, .. } = service_state.client_state {
            callbacks.onHalEvent(UwbEvent::POST_INIT_CPLT, UwbStatus::OK)?;
            Ok(())
        } else {
            Err(binder::ExceptionCode::ILLEGAL_STATE.into())
        }
    }

    async fn sessionInit(&self, _id: i32) -> Result<()> {
        log::debug!("sessionInit");

        Ok(())
    }

    async fn getSupportedAndroidUciVersion(&self) -> Result<i32> {
        log::debug!("getSupportedAndroidUciVersion");

        Ok(1)
    }

    async fn sendUciMessage(&self, data: &[u8]) -> Result<i32> {
        log::debug!("sendUciMessage");

        let mut service_state = self.service_state.lock().await;

        if matches!(service_state.client_state, ClientState::Closed) {
            log::error!("the state is not opened");
            return Err(binder::ExceptionCode::ILLEGAL_STATE.into());
        }

        log::debug!(" --> {:?}", data);
        service_state
            .writer
            .write_all(data)
            .await
            .map(|_| data.len() as i32)
            .map_err(|_| binder::StatusCode::UNKNOWN_ERROR.into())
    }
}
