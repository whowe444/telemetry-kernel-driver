#! /bin/bash

WORKSPACE="${HOME}/Documents/telemetry-kernel-driver/"

# Deploy rabbit-mq container in systemd
echo "Copying rabbitmq-container.service to ~/.config/systemd/user/"
cp "${WORKSPACE}/connectors/ipc_to_rabbit/rabbitmq-container.service" "${HOME}/.config/systemd/user/"
systemctl --user daemon-reload
systemctl --user enable rabbitmq-container.service


