#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define BUFSIZE 65536
#define COUNT 10
#define WINDOW_SIZE 255

void printProgressBar(double progress, int count) {
    int barWidth = 70;
    int filledWidth = progress * barWidth;

    printf("\r[");
    for (int i = 0; i < filledWidth; ++i) {
        printf("=");
    }
    for (int i = filledWidth; i < barWidth; ++i) {
        printf(" ");
    }
    printf("] %.2f%% (%d/%d)", progress * 100, count, COUNT);
    fflush(stdout);
}

void writeMessageToFile(FILE *file, int size, int id_sent, int id_received) {
    fprintf(file, "%d,%d,%d\n", size, id_sent, id_received);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s <destination IP> <destinatio port>\n", argv[0]);
        return 1;
    }

    int sock;
    struct sockaddr_in addr;
    char buf[BUFSIZE];
    socklen_t addr_len;
    struct timespec start, end;
    long total_time, total_sent_bytes, total_received_bytes;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket");
        exit(1);
    }

    struct timeval timeout;
    timeout.tv_usec = 500000;
    timeout.tv_sec = 0;

    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("setsockopt");
        exit(1);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[2]));
    addr.sin_addr.s_addr = inet_addr(argv[1]);

    printf("UDP Sender sending to IP %s on port %s\n", argv[1], argv[2]);

    // for (int size = 1; size <= 1001; size += 100) {
    //     int msg_size = (size > 1) ? (size - 1) : size;
    for (int size = 1024; size <= 32768; size += 1024) {
        int msg_size = size;
        int timeout_count = 0;
        total_time = 0;
        total_sent_bytes = 0;
        total_received_bytes = 0;

        // Send COUNT, msg_size, and window size to the echoer
        char info_msg[BUFSIZE];
        snprintf(info_msg, BUFSIZE, "%d:%d:%d", COUNT, msg_size, WINDOW_SIZE);
        sendto(sock, info_msg, strlen(info_msg), 0, (struct sockaddr *)&addr, sizeof(addr));

        // Wait for acknowledgment from the echoer
        char ack_msg[BUFSIZE];
        recvfrom(sock, ack_msg, BUFSIZE, 0, (struct sockaddr *)&addr, &addr_len);

        printf("Message Size: %d bytes\n", msg_size);

        int stopAckReceived = 0;

        // Open file to write sent messages
        char sent_file_name[50];
        sprintf(sent_file_name, "sent_messages_size_%d.txt", msg_size);
        FILE *sent_file = fopen(sent_file_name, "w");
        if (sent_file == NULL) {
            perror("Error opening sent messages file");
            exit(1);
        }

        for (int i = 0; i < COUNT; i++) {
            clock_gettime(CLOCK_MONOTONIC, &start);

            char numbered_buf[BUFSIZE];

            // Create the message with ID and zero padding
            memset(numbered_buf, 0, sizeof(numbered_buf));  // Fill with zeros

            unsigned char id = i % WINDOW_SIZE;  // Create a cyclic ID based on the WINDOW_SIZE
            numbered_buf[0] = id;                // Write the ID at the start of the buffer

            int len = sendto(sock, numbered_buf, msg_size, 0, (struct sockaddr *)&addr, sizeof(addr));

            if (len < 0) {
                printf("\nTimeout occurred! Continuing...\n");
                timeout_count++;
                continue;
            }

            // Receive echo from the echoer
            memset(buf, 0, sizeof(buf));
            len = recvfrom(sock, buf, BUFSIZE, 0, (struct sockaddr *)&addr, &addr_len);
            if (len < 0) {
                printf("\nTimeout occurred while receiving echo! Continuing...\n");
                timeout_count++;
                continue;
            }

            clock_gettime(CLOCK_MONOTONIC, &end);

            long seconds = end.tv_sec - start.tv_sec;
            long ns = end.tv_nsec - start.tv_nsec;

            if (start.tv_nsec > end.tv_nsec) {  // clock underflow
                --seconds;
                ns += 1000000000;
            }

            total_time += seconds * 1000000000 + ns;
            total_sent_bytes += msg_size;
            total_received_bytes += len;

            double progress = (double)(i + 1) / COUNT;
            printProgressBar(progress, i + 1);

            // Parse the received message to extract the ID
            unsigned char received_id = buf[0];

            // Write sent message and received ID to file
            writeMessageToFile(sent_file, msg_size, id, received_id);

            // Check if the echoed message is the same as the original message
            if (memcmp(numbered_buf, buf, msg_size) != 0) {
                printf("Message mismatched: Sent message ID = %d, Received message ID = %d\n", id, received_id);
                i--;  // Resend the message by decrementing the loop counter
                continue;
            }
        }

        fclose(sent_file);  // Close the sent messages file

        double mean_time = (double)total_time / COUNT / 1000000000;

        printf("\nTotal Time for the Test: %ld nanoseconds\n", total_time);
        printf("Mean Time: %e seconds\n", mean_time);
        printf("Total Sent Bytes: %ld bytes\n", total_sent_bytes);
        printf("Total Received Bytes: %ld bytes\n", total_received_bytes);
        printf("Timeouts: %d\n", timeout_count);
        printf("---------------------------------------\n");

        // Send stop message to the echoer if STOP_ACK was not received
        if (!stopAckReceived) {
            char stop_msg[] = "STOP";
            sendto(sock, stop_msg, strlen(stop_msg), 0, (struct sockaddr *)&addr, sizeof(addr));
        }

        // Wait for a final acknowledgment from the echoer
        char final_ack_msg[BUFSIZE];
        recvfrom(sock, final_ack_msg, BUFSIZE, 0, (struct sockaddr *)&addr, &addr_len);
    }

    close(sock);

    return 0;
}
