#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <poll.h>

#define BUFSIZE 65536
#define PORT 55555
#define TIMEOUT_MS 5000

void printProgressBar(double progress, int count, int totalCount) {
    int barWidth = 70;
    int filledWidth = progress * barWidth;

    printf("\r[");
    for (int i = 0; i < filledWidth; ++i) {
        printf("=");
    }
    printf(">");
    for (int i = filledWidth; i < barWidth; ++i) {
        printf(" ");
    }
    printf("] %.2f%% (%d/%d)", progress * 100, count, totalCount);
    fflush(stdout);
}

void writeMessageToFile(FILE *file, int size, int id_sent) {
    fprintf(file, "%d,%d\n", size, id_sent);
}

int main() {
    int sock;
    struct sockaddr_in addr;
    char buf[BUFSIZE];
    socklen_t addr_len;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket");
        exit(1);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        exit(1);
    }

    printf("UDP Receiver running on port %d\n", PORT);

    struct pollfd fds[1];
    fds[0].fd = sock;
    fds[0].events = POLLIN;

    int count = 0, msg_size = 0, window_size = 0;
    int received_count = 0, echoed_count = 0;
    double progress = 0.0;
    int new_batch = 1;

    // Open file to write received messages
    FILE *received_file = fopen("received_messages.txt", "w");
    if (received_file == NULL) {
        perror("Error opening received messages file");
        exit(1);
    }

    while (1) {
        int pollResult = poll(fds, 1, TIMEOUT_MS);

        if (pollResult < 0) {
            perror("poll");
            break;
        } else if (pollResult == 0) {
            printf("Timeout occurred. No data received.\n");
            break;
        }

        addr_len = sizeof(addr);
        int len = recvfrom(sock, buf, BUFSIZE, 0, (struct sockaddr *)&addr, &addr_len);
        if (len > 0) {
            // Check if it's a stop message
            if (len == 4 && strncmp(buf, "STOP", 4) == 0)  // Check length to avoid matching padded messages
            {
                printf("\nReceived stop message. Echoed messages: %d\n", echoed_count);
                new_batch = 1;  // Set flag for new batch

                // Send a final acknowledgment to the sender
                sendto(sock, "STOP_ACK", strlen("STOP_ACK") + 1, 0, (struct sockaddr *)&addr, sizeof(addr));

                // Continue receiving the remaining batches
                continue;
            }

            // If we're at the start of a new batch, expect the initial message with count, msg_size, and window_size
            if (new_batch) {
                sscanf(buf, "%d:%d:%d", &count, &msg_size, &window_size);

                printf("Received message size: %d bytes\n", msg_size);
                printf("Total messages to receive: %d\n", count);

                // Send acknowledgment to the sender
                sendto(sock, "ACK", 3, 0, (struct sockaddr *)&addr, sizeof(addr));

                received_count = 0;
                echoed_count = 0;
                new_batch = 0;  // Unset flag for new batch
                continue;       // Skip to next iteration
            }

            unsigned char id_sent = buf[0];  // Extract the ID directly from the message

            // Write received message to file
            writeMessageToFile(received_file, len, id_sent);

            // Update progress
            received_count++;
            echoed_count++;
            progress = (double)received_count / count;
            printProgressBar(progress, received_count, count);

            // Check if all messages for the current batch have been received
            if (received_count >= count) {
                printf("\n");
                new_batch = 1;  // Set flag for new batch
            }
        }
    }

    fclose(received_file);  // Close the received messages file

    return 0;
}
