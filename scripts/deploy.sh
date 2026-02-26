#! /bin/bash

WORKSPACE="${HOME}/Documents/telemetry-kernel-driver/"

# Deploy rabbit-mq container in systemd
echo "Copying rabbitmq-container.service to ~/.config/systemd/user/"
cp "${WORKSPACE}/connectors/ipc_to_rabbit/rabbitmq-container.service" "${HOME}/.config/systemd/user/"
systemctl --user daemon-reload
systemctl --user enable rabbitmq-container.service

# Deploy telemetry-driver-controller in systemd
echo "Copying telemetry-driver-controller.service to /etc/systemd/system/"
sudo cp "${WORKSPACE}/controller/telemetry-driver-controller.service" "/etc/systemd/system/"
sudo systemctl --user daemon-reload
sudo systemctl enable telemetry-driver-controller.service

# Deploy the poll-to-ipc connector in systemd
echo "Copying poll-to-ipc@.service to ~/.config/systemd/user/"
cp "${WORKSPACE}/connectors/poll_to_ipc/poll-to-ipc@.service" "${HOME}/.config/systemd/user/"
systemctl --user daemon-reload
systemctl --user enable --now poll-to-ipc@temperature.service
systemctl --user enable --now poll-to-ipc@humidity.service

# Deploy the ipc-to-rabbit connector in systemd
echo "Copying ipc-to-rabbit@.service to ~/.config/systemd/user/"
cp "${WORKSPACE}/connectors/ipc_to_rabbit/ipc-to-rabbit@.service" "${HOME}/.config/systemd/user/"
systemctl --user daemon-reload
systemctl --user enable --now ipc-to-rabbit@temperature.service
systemctl --user enable --now ipc-to-rabbit@humidity.service
