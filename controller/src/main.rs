use clap::Parser;
use clap::ValueEnum;
use std::os::fd::AsRawFd;
use std::fs::OpenOptions;

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

/// Open the device and issue an ioctl.
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

#[derive(ValueEnum, Clone, Debug)]
enum TemperatureUnit {
    Celsius,
    Fahrenheit
}

#[derive(Parser, Debug)]
#[command(version, about, long_about=None)]
struct Args {
    // Set
    #[arg(short, long, requires="value")]
    set: Option<String>,

    // Get
    #[arg(short, long)]
    get: Option<String>,

    // Value
    #[arg(long, value_enum)]
    value: Option<TemperatureUnit>,
}

fn main() {
    let args = Args::parse();
    if let Some(sensor) = args.get {
        if sensor == "temperature" {
            let units_string = match sensor_get_unit() {
                Ok(0) => "Celsius",
                Ok(1) => "Fahrenheit",
                Err(_) => todo!(),
                Ok(i32::MIN..=-1_i32) | Ok(2_i32..=i32::MAX) => todo!()
            };
            print!("Current units of the temperature sensor are: {}\n", units_string);        
        }
    } else if let Some(sensor) = args.set {
        if sensor == "temperature" {
            let ioctl_request = match args.value {
                Some(TemperatureUnit::Celsius) => sensor_ioctl(SENSOR_IOC_SET_CELSIUS),
                Some(TemperatureUnit::Fahrenheit) => sensor_ioctl(SENSOR_IOC_SET_FAHRENHEIT),
                None => todo!()
            };
            print!("Setting temperature to: {:?}\n", args.value.unwrap());
        }
    }

}