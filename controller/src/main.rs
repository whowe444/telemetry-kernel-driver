use axum::{
    extract::Json,
    http::StatusCode,
    routing::post,
    Router,
};
use serde::{Deserialize, Serialize};
use std::fs::OpenOptions;
use std::os::unix::io::AsRawFd;

// ── ioctl definitions ────────────────────────────────────────────────────────
// Mirrors the C macros from your driver header:
//   #define SENSOR_IOC_MAGIC        'S'
//   #define SENSOR_IOC_SET_CELSIUS    _IO(SENSOR_IOC_MAGIC, 0)
//   #define SENSOR_IOC_SET_FAHRENHEIT _IO(SENSOR_IOC_MAGIC, 1)
//   #define SENSOR_IOC_GET_UNIT       _IOR(SENSOR_IOC_MAGIC, 2, int)
//
// Linux ioctl encoding (asm-generic/ioctl.h):
//   _IO(type, nr)        = (type << 8) | nr              (no data transfer)
//   _IOR(type, nr, size) = (2 << 30) | (size << 16) | (type << 8) | nr

const SENSOR_IOC_MAGIC: u64 = b'S' as u64;

const fn _io(magic: u64, nr: u64) -> u64 {
    (magic << 8) | nr
}

const fn _ior(magic: u64, nr: u64, size: u64) -> u64 {
    (2 << 30) | (size << 16) | (magic << 8) | nr
}

const SENSOR_IOC_SET_CELSIUS: u64    = _io(SENSOR_IOC_MAGIC, 0);
const SENSOR_IOC_SET_FAHRENHEIT: u64 = _io(SENSOR_IOC_MAGIC, 1);
const SENSOR_IOC_GET_UNIT: u64       = _ior(SENSOR_IOC_MAGIC, 2, std::mem::size_of::<libc::c_int>() as u64);

const DEVICE_PATH: &str = "/dev/mock_sensor";

// ── sensor ioctl helpers ─────────────────────────────────────────────────────

/// Open the device and issue an ioctl. Runs in a blocking thread so it won't
/// stall the async runtime.
fn sensor_ioctl(request: u64) -> std::io::Result<()> {
    let file = OpenOptions::new().read(true).write(true).open(DEVICE_PATH)?;
    let fd = file.as_raw_fd();

    let ret = unsafe { libc::ioctl(fd, request) };
    if ret < 0 {
        return Err(std::io::Error::last_os_error());
    }
    Ok(())
}

fn sensor_get_unit() -> std::io::Result<libc::c_int> {
    let file = OpenOptions::new().read(true).write(true).open(DEVICE_PATH)?;
    let fd = file.as_raw_fd();

    let mut unit: libc::c_int = 0;
    let ret = unsafe { libc::ioctl(fd, SENSOR_IOC_GET_UNIT, &mut unit as *mut libc::c_int) };
    if ret < 0 {
        return Err(std::io::Error::last_os_error());
    }
    Ok(unit)
}

// ── request / response types ─────────────────────────────────────────────────

#[derive(Deserialize)]
struct SetUnitRequest {
    /// Accepted values: "celsius" | "fahrenheit"
    unit: String,
}

#[derive(Serialize)]
struct ApiResponse {
    status: String,
    #[serde(skip_serializing_if = "Option::is_none")]
    unit: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    error: Option<String>,
}

// ── handlers ─────────────────────────────────────────────────────────────────

/// POST /set_unit  { "unit": "celsius" | "fahrenheit" }
async fn set_unit(Json(payload): Json<SetUnitRequest>) -> (StatusCode, Json<ApiResponse>) {
    let ioctl_code = match payload.unit.to_lowercase().as_str() {
        "celsius"    => SENSOR_IOC_SET_CELSIUS,
        "fahrenheit" => SENSOR_IOC_SET_FAHRENHEIT,
        other => {
            return (
                StatusCode::BAD_REQUEST,
                Json(ApiResponse {
                    status: "error".into(),
                    unit: None,
                    error: Some(format!("Unknown unit '{}'. Use 'celsius' or 'fahrenheit'.", other)),
                }),
            );
        }
    };

    // Build the HTTP response first, then issue the ioctl on a blocking thread.
    let response = Json(ApiResponse {
        status: "ok".into(),
        unit: Some(payload.unit.clone()),
        error: None,
    });

    let result = tokio::task::spawn_blocking(move || sensor_ioctl(ioctl_code))
        .await
        .expect("spawn_blocking panicked");

    if let Err(e) = result {
        eprintln!("ioctl SENSOR_IOC_SET_{} failed: {}", payload.unit.to_uppercase(), e);
        return (
            StatusCode::INTERNAL_SERVER_ERROR,
            Json(ApiResponse {
                status: "error".into(),
                unit: None,
                error: Some(e.to_string()),
            }),
        );
    }

    println!("ioctl SENSOR_IOC_SET_{} succeeded", payload.unit.to_uppercase());
    (StatusCode::OK, response)
}

/// POST /get_unit  (no body required)
async fn get_unit() -> (StatusCode, Json<ApiResponse>) {
    let result = tokio::task::spawn_blocking(sensor_get_unit)
        .await
        .expect("spawn_blocking panicked");

    match result {
        Ok(unit_code) => {
            let unit_str = match unit_code {
                0 => "celsius",
                1 => "fahrenheit",
                _ => "unknown",
            };
            println!("ioctl SENSOR_IOC_GET_UNIT returned {}", unit_code);
            (
                StatusCode::OK,
                Json(ApiResponse {
                    status: "ok".into(),
                    unit: Some(unit_str.into()),
                    error: None,
                }),
            )
        }
        Err(e) => {
            eprintln!("ioctl SENSOR_IOC_GET_UNIT failed: {}", e);
            (
                StatusCode::INTERNAL_SERVER_ERROR,
                Json(ApiResponse {
                    status: "error".into(),
                    unit: None,
                    error: Some(e.to_string()),
                }),
            )
        }
    }
}

// ── main ─────────────────────────────────────────────────────────────────────

#[tokio::main]
async fn main() {
    let app = Router::new()
        .route("/set_unit", post(set_unit))
        .route("/get_unit", post(get_unit));

    let listener = tokio::net::TcpListener::bind("0.0.0.0:8080").await.unwrap();
    println!("Listening on http://0.0.0.0:8080");
    println!("Set unit to Celsius:");
    println!(r#"  curl -X POST http://localhost:8080/set_unit -H "Content-Type: application/json" -d '{{"unit":"celsius"}}'"#);

    println!("\nSet unit to Fahrenheit:");
    println!(r#"  curl -X POST http://localhost:8080/set_unit -H "Content-Type: application/json" -d '{{"unit":"fahrenheit"}}'"#);

    println!("\nGet current unit:");
    println!("  curl -X POST http://localhost:8080/get_unit");

    axum::serve(listener, app).await.unwrap();
}