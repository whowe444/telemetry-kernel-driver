Title: Telemetry Device Driver
Author: Will Howe

This repo contains a personal project consisting of a simple linux device driver simulating navigation telemetry values from a sensor on-board a satellite vehicle. The data from the device is read by a user space process and publishes out over a message broker.

This project is "pedagogical" with the intention of learning as well as showcasing some interesting technologies. Namely:

- linux device driver with /sys/ interface and /dev/ char device node
- driver controllable with ioctl
- Rust / C connector programs with message queues
- HTTP endpoint in Rust
- RabbitMQ interface in Rust
- systemd security mechanisms

I did make use of LLMs during the course of this project.

Pre-requisites
=========================

1. Install the kernel headers

`sudo dnf install kernel-devel kernel-headers`

or if that doesn't work:

`sudo dnf install kernel-devel-$(uname -r)`

Instructions for installing the kernel module
=========================

1. Build the module

`make`

2. Load the module

`sudo insmod mock_sensor.ko`

3. Check that it was loaded properly

`sudo dmesg | tail`
`lsmod | grep mock_sensor`

4. Access the sensor values

`cat /sys/kernel/mock_sensor/temperature`
`cat /sys/kernel/mock_sensor/humidity`

5. You can also write new values

`echo 30 | sudo tee /sys/kernel/mock_sensor/temperature`

6. Unload when done

`sudo rmmod mock_sensor`

Instructions for running the connector program
===============================================

1. Build

`cd connectors/poll_to_ipc && make`

2. Install the executable

`sudo make install` 

3. Run the program for temperature

`./poll_to_ipc /temperature /sys/kernel/mock_sensor/temperature`

4. Run the program for humidity

`./poll_to_ipc /humidity /sys/kernel/mock_sensor/humidity`

NOTE: Messages are only added to the mq when the driver updates the sysfs file.

Instructions for running the Rust connectors
============================================

1. Build

`cargo run`

2. Run

`cargo run /temperature`

`cargo run /humidity`

3. Test

`cargo test`

4. Install in `/usr/local/bin/`

`sudo cargo install --path . --root /usr/local/`

Running a rabbit-mq broker
=================================================

# Run RabbitMQ with podman
```
podman run -d \
  --name rabbitmq \
  -p 5672:5672 \
  -p 15672:15672 \
  -e RABBITMQ_DEFAULT_USER=guest \
  -e RABBITMQ_DEFAULT_PASS=guest \
  rabbitmq:3-management
```

Inspect rabbitmq: http://localhost:15672/#/

Running the telemetry-driver-controller
==================================================

1. Build

```
cd telemetry-kernel-driver
cd controller
cargo build
```

2. Install

`cargo install --path . --root /usr/local`

Deploy (putting it all together)
========================================

Rather than running the above programs by hand, we can deploy and status them with some Bash scripts.

1. Deploy systemd services

```
cd telemetry-kernel-driver
./scripts/deploy.sh
```

2. Start the systemd services

```
cd telemetry-kernel-driver
./scripts/start.sh
```

3. Status the systemd services

```
cd telemetry-kernel-driver
./scripts/status.sh
```

TODO: More documentation of hard to understand code.
