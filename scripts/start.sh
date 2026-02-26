#! /bin/bash

# Start the rabbitmq broker
systemctl --user restart rabbitmq-container.service

# Start the telemetry-driver-controller
sudo systemctl restart telemetry-driver-controller.service

# Start the humidity poll-to-ipc
systemctl --user restart poll-to-ipc@humidity.service

# Start the temperature poll-to-ipc
systemctl --user restart poll-to-ipc@temperature.service

# Wait for 10 seconds before trying to start the ipc-to-rabbit connectors
sleep 10s

# Start the humidity ipc-to-rabbit
systemctl --user restart ipc-to-rabbit@humidity.service

# Start the temperature ipc-to-rabbit
systemctl --user restart ipc-to-rabbit@temperature.service
