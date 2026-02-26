#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/poll.h>
#include <mqueue.h>
#include <errno.h>

#define MAX_MSG_SIZE 1024

// Function to print usage information
void print_usage() {
    printf("Usage: file_poll_posix_ipc <queue_name> <file_to_poll>\n");
    printf("  <queue_name>   - Name of the POSIX message queue (e.g., /my_message_queue)\n");
    printf("  <file_to_poll> - Path to the sysfs or other file to poll for changes\n");
}

int main(int argc, char *argv[]) {
    // Check if enough arguments were passed
    if (argc != 3) {
        print_usage();
        return 1;
    }

    // Parse command-line arguments
    const char *queue_name = argv[1];  // Message queue name
    const char *filename = argv[2];    // File to poll

    // Step 1: Open POSIX message queue
    mqd_t mq;
    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;  // Maximum number of messages in the queue
    attr.mq_msgsize = MAX_MSG_SIZE;  // Maximum message size
    attr.mq_curmsgs = 0;  // Number of messages currently in the queue

    mq_unlink(queue_name);  // Cleanup the message queue
    mq = mq_open(queue_name, O_CREAT | O_WRONLY, 0644, &attr);
    if (mq == (mqd_t) -1) {
        perror("mq_open failed");
        return 1;
    }

    // Step 2: Open the file to be read (the sysfs file path)
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("Failed to open file");
        mq_close(mq);
        mq_unlink(queue_name);  // Cleanup the message queue
        return 1;
    }

    // Step 3: Setup poll structure to watch the file descriptor
    struct pollfd fds[1];
    fds[0].fd = fd;            // File descriptor of the sysfs file
    fds[0].events = POLLPRI | POLLERR;    // We want to poll for readability (any change in file)

    // Step 4: Polling loop
    while (1) {
        int ret = poll(fds, 1, 5000);  // Poll with 5 seconds timeout
        if (ret == -1) {
            perror("poll failed");
            close(fd);
            mq_close(mq);
            mq_unlink(queue_name);  // Cleanup the message queue
            return 1;
        }

        if (ret == 0) {
            printf("Polling timeout\n");
            continue;  // Timeout, continue polling
        }

        // If POLLPRI event occurs, file is ready to be read
        if (fds[0].revents & POLLPRI) {
            // Reset the file pointer to the beginning of the file
            if (lseek(fd, 0, SEEK_SET) == -1) {
                perror("lseek failed");
                close(fd);
                mq_close(mq);
                mq_unlink(queue_name);  // Cleanup the message queue
                return 1;
            }

            char buffer[MAX_MSG_SIZE];
            ssize_t bytes_read = read(fd, buffer, sizeof(buffer) - 1);
            if (bytes_read == -1) {
                perror("Error reading file");
                close(fd);
                mq_close(mq);
                mq_unlink(queue_name);  // Cleanup the message queue
                return 1;
            }

            // Null terminate the buffer
            buffer[bytes_read] = '\0';

            // Step 5: Send the read data to the message queue
            if (mq_send(mq, buffer, bytes_read + 1, 0) == -1) {
                perror("mq_send failed");
                close(fd);
                mq_close(mq);
                mq_unlink(queue_name);  // Cleanup the message queue
                return 1;
            }

            printf("Sent data to message queue: %s\n", buffer);
        }
    }

    // Cleanup resources before exiting
    close(fd);
    mq_close(mq);
    mq_unlink(queue_name);  // Delete the message queue
    return 0;
}
