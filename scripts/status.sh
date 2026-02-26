#! /bin/bash

# Status the rabbitmq broker
systemctl --user status rabbitmq-container.service

# Status the telemetry-driver-controller
sudo systemctl status telemetry-driver-controller.service

# Status the humidity ipc-to-rabbit
systemctl --user status ipc-to-rabbit@humidity.service

# Status the temperature ipc-to-rabbit
systemctl --user status ipc-to-rabbit@temperature.service

# Status the humidity poll-to-ipc
systemctl --user status poll-to-ipc@humidity.service

# Status the temperature poll-to-ipc
systemctl --user status poll-to-ipc@temperature.service

