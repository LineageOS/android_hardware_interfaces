use android_hardware_uwb::aidl::android::hardware::uwb::{
    IUwbChip::IUwbChipAsyncServer, IUwbClientCallback::IUwbClientCallback, UwbEvent::UwbEvent,
    UwbStatus::UwbStatus,
};
use android_hardware_uwb::binder;
use async_trait::async_trait;
use binder::{Result, Strong};

use tokio::fs::{File, OpenOptions};
use tokio::io::{AsyncReadExt, AsyncWriteExt};
use tokio::sync::Mutex;

use std::os::fd::AsRawFd;

use std::io;

use nix::sys::termios;

enum State {
    Closed,
    Opened {
        callbacks: Strong<dyn IUwbClientCallback>,
        #[allow(dead_code)]
        tasks: tokio::task::JoinSet<()>,
        serial: File,
    },
}

pub struct UwbChip {
    name: String,
    path: String,
    state: Mutex<State>,
}

impl UwbChip {
    pub fn new(name: String, path: String) -> Self {
        Self {
            name,
            path,
            state: Mutex::new(State::Closed),
        }
    }
}

pub fn makeraw(file: File) -> io::Result<File> {
    let fd = file.as_raw_fd();

    let mut attrs = termios::tcgetattr(fd)?;

    termios::cfmakeraw(&mut attrs);

    termios::tcsetattr(fd, termios::SetArg::TCSANOW, &attrs)?;

    Ok(file)
}

impl binder::Interface for UwbChip {}

#[async_trait]
impl IUwbChipAsyncServer for UwbChip {
    async fn getName(&self) -> Result<String> {
        Ok(self.name.clone())
    }

    async fn open(&self, callbacks: &Strong<dyn IUwbClientCallback>) -> Result<()> {
        log::debug!("open: {:?}", &self.path);

        let serial = OpenOptions::new()
            .read(true)
            .write(true)
            .create(false)
            .open(&self.path)
            .await
            .and_then(makeraw)
            .map_err(|_| binder::StatusCode::UNKNOWN_ERROR)?;

        let mut state = self.state.lock().await;

        if let State::Closed = *state {
            let client_callbacks = callbacks.clone();

            let mut tasks = tokio::task::JoinSet::new();
            let mut reader = serial
                .try_clone()
                .await
                .map_err(|_| binder::StatusCode::UNKNOWN_ERROR)?;

            tasks.spawn(async move {
                loop {
                    const UWB_HEADER_SIZE: usize = 4;

                    let mut buffer = vec![0; UWB_HEADER_SIZE];
                    reader
                        .read_exact(&mut buffer[0..UWB_HEADER_SIZE])
                        .await
                        .unwrap();

                    let length = buffer[3] as usize + UWB_HEADER_SIZE;

                    buffer.resize(length, 0);
                    reader
                        .read_exact(&mut buffer[UWB_HEADER_SIZE..length])
                        .await
                        .unwrap();

                    client_callbacks.onUciMessage(&buffer[..]).unwrap();
                }
            });

            callbacks.onHalEvent(UwbEvent::OPEN_CPLT, UwbStatus::OK)?;

            *state = State::Opened {
                callbacks: callbacks.clone(),
                tasks,
                serial,
            };

            Ok(())
        } else {
            Err(binder::ExceptionCode::ILLEGAL_STATE.into())
        }
    }

    async fn close(&self) -> Result<()> {
        log::debug!("close");

        let mut state = self.state.lock().await;

        if let State::Opened { ref callbacks, .. } = *state {
            callbacks.onHalEvent(UwbEvent::CLOSE_CPLT, UwbStatus::OK)?;
            *state = State::Closed;
            Ok(())
        } else {
            Err(binder::ExceptionCode::ILLEGAL_STATE.into())
        }
    }

    async fn coreInit(&self) -> Result<()> {
        log::debug!("coreInit");

        if let State::Opened { ref callbacks, .. } = *self.state.lock().await {
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
        Ok(1)
    }

    async fn sendUciMessage(&self, data: &[u8]) -> Result<i32> {
        log::debug!("sendUciMessage");

        if let State::Opened { ref mut serial, .. } = &mut *self.state.lock().await {
            serial
                .write(data)
                .await
                .map(|written| written as i32)
                .map_err(|_| binder::StatusCode::UNKNOWN_ERROR.into())
        } else {
            Err(binder::ExceptionCode::ILLEGAL_STATE.into())
        }
    }
}
