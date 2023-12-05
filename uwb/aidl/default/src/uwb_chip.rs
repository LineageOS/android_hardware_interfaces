use android_hardware_uwb::aidl::android::hardware::uwb::{
    IUwbChip::IUwbChipAsyncServer, IUwbClientCallback::IUwbClientCallback, UwbEvent::UwbEvent,
    UwbStatus::UwbStatus,
};
use android_hardware_uwb::binder;
use async_trait::async_trait;
use binder::{DeathRecipient, IBinder, Result, Strong};

use std::sync::Arc;
use tokio::io::unix::AsyncFd;
use tokio::select;
use tokio::sync::Mutex;
use tokio_util::sync::CancellationToken;

use std::fs::{File, OpenOptions};
use std::io::{self, Read, Write};
use std::os::unix::fs::OpenOptionsExt;

enum State {
    Closed,
    Opened {
        callbacks: Strong<dyn IUwbClientCallback>,
        handle: tokio::task::JoinHandle<()>,
        serial: File,
        death_recipient: DeathRecipient,
        token: CancellationToken,
    },
}

pub struct UwbChip {
    name: String,
    path: String,
    state: Arc<Mutex<State>>,
}

impl UwbChip {
    pub fn new(name: String, path: String) -> Self {
        Self {
            name,
            path,
            state: Arc::new(Mutex::new(State::Closed)),
        }
    }
}

impl State {
    /// Terminate the reader task.
    async fn close(&mut self) -> Result<()> {
        if let State::Opened { ref mut token, ref callbacks, ref mut death_recipient, ref mut handle, .. } = *self {
            log::info!("waiting for task cancellation");
            callbacks.as_binder().unlink_to_death(death_recipient)?;
            token.cancel();
            handle.await.unwrap();
            log::info!("task successfully cancelled");
            callbacks.onHalEvent(UwbEvent::CLOSE_CPLT, UwbStatus::OK)?;
            *self = State::Closed;
        }
        Ok(())
    }
}

pub fn makeraw(file: File) -> io::Result<File> {
    // Configure the file descriptor as raw fd.
    use nix::sys::termios::*;
    let mut attrs = tcgetattr(&file)?;
    cfmakeraw(&mut attrs);
    tcsetattr(&file, SetArg::TCSANOW, &attrs)?;

    Ok(file)
}

/// Wrapper around Read::read to handle EWOULDBLOCK.
/// /!\ will actively wait for more data, make sure to call
/// this method only when data is immediately expected.
fn read_exact(file: &mut File, mut buf: &mut [u8]) -> io::Result<()> {
    while buf.len() > 0 {
        match file.read(buf) {
            Ok(0) => panic!("unexpectedly reached end of file"),
            Ok(read_len) => buf = &mut buf[read_len..],
            Err(err) if err.kind() == io::ErrorKind::WouldBlock => continue,
            Err(err) => return Err(err),
        }
    }
    Ok(())
}

impl binder::Interface for UwbChip {}

#[async_trait]
impl IUwbChipAsyncServer for UwbChip {
    async fn getName(&self) -> Result<String> {
        Ok(self.name.clone())
    }

    async fn open(&self, callbacks: &Strong<dyn IUwbClientCallback>) -> Result<()> {
        log::debug!("open: {:?}", &self.path);

        let mut state = self.state.lock().await;

        if matches!(*state, State::Opened { .. }) {
            log::error!("the state is already opened");
            return Err(binder::ExceptionCode::ILLEGAL_STATE.into());
        }

        let serial = OpenOptions::new()
            .read(true)
            .write(true)
            .create(false)
            .custom_flags(libc::O_NONBLOCK)
            .open(&self.path)
            .and_then(makeraw)
            .map_err(|_| binder::StatusCode::UNKNOWN_ERROR)?;

        let state_death_recipient = self.state.clone();
        let mut death_recipient = DeathRecipient::new(move || {
            let mut state = state_death_recipient.blocking_lock();
            log::info!("Uwb service has died");
            if let State::Opened { ref mut token, .. } = *state {
                token.cancel();
                *state = State::Closed;
            }
        });

        callbacks.as_binder().link_to_death(&mut death_recipient)?;

        let token = CancellationToken::new();
        let cloned_token = token.clone();

        let client_callbacks = callbacks.clone();

        let reader = serial
            .try_clone()
            .map_err(|_| binder::StatusCode::UNKNOWN_ERROR)?;

        let join_handle = tokio::task::spawn(async move {
            log::info!("UCI reader task started");
            let mut reader = AsyncFd::new(reader).unwrap();

            loop {
                const UWB_HEADER_SIZE: usize = 4;
                let mut buffer = vec![0; UWB_HEADER_SIZE];

                // The only time where the task can be safely
                // cancelled is when no packet bytes have been read.
                //
                // - read_exact() cannot be used here since it is not
                //   cancellation safe.
                // - read() cannot be used because it cannot be cancelled:
                //   the syscall is executed blocking on the threadpool
                //   and completes after termination of the task when
                //   the pipe receives more data.
                let read_len = loop {
                    // On some platforms, the readiness detecting mechanism
                    // relies on edge-triggered notifications. This means that
                    // the OS will only notify Tokio when the file descriptor
                    // transitions from not-ready to ready. For this to work
                    // you should first try to read or write and only poll for
                    // readiness if that fails with an error of
                    // std::io::ErrorKind::WouldBlock.
                    match reader.get_mut().read(&mut buffer) {
                        Ok(0) => {
                            log::error!("file unexpectedly closed");
                            return;
                        }
                        Ok(read_len) => break read_len,
                        Err(err) if err.kind() == io::ErrorKind::WouldBlock => (),
                        Err(_) => panic!("unexpected read failure"),
                    }

                    let mut guard = select! {
                        _ = cloned_token.cancelled() => {
                            log::info!("task is cancelled!");
                            return;
                        },
                        result = reader.readable() => result.unwrap()
                    };

                    guard.clear_ready();
                };

                // Read the remaining header bytes, if truncated.
                read_exact(reader.get_mut(), &mut buffer[read_len..]).unwrap();

                let length = buffer[3] as usize + UWB_HEADER_SIZE;
                buffer.resize(length, 0);

                // Read the payload bytes.
                read_exact(reader.get_mut(), &mut buffer[UWB_HEADER_SIZE..]).unwrap();

                client_callbacks.onUciMessage(&buffer).unwrap();
            }
        });

        callbacks.onHalEvent(UwbEvent::OPEN_CPLT, UwbStatus::OK)?;

        *state = State::Opened {
            callbacks: callbacks.clone(),
            handle: join_handle,
            serial,
            death_recipient,
            token,
        };

        Ok(())
    }

    async fn close(&self) -> Result<()> {
        log::debug!("close");

        let mut state = self.state.lock().await;

        if matches!(*state, State::Opened { .. }) {
            state.close().await
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
                .map(|written| written as i32)
                .map_err(|_| binder::StatusCode::UNKNOWN_ERROR.into())
        } else {
            Err(binder::ExceptionCode::ILLEGAL_STATE.into())
        }
    }
}
