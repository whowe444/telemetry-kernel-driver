#! /bin/bash

# Start the rabbitmq broker
systemctl --user restart rabbitmq-container.service
systemctl --user status rabbitmq-container.service
