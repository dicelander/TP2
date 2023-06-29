#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#define BUFSIZE 1024
#define PORT 55555
#define COUNT 100000

void printProgressBar(double progress, int count) {
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
    printf("] %.2f%% (%d/%d)", progress * 100, count, COUNT);
    fflush(stdout);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <destination IP>\n", argv[0]);
        return 1;
    }

    int sock;
    struct sockaddr_in addr;
    char buf[BUFSIZE];
    socklen_t addr_len;
    struct timespec start, end;
    long total_time, total_sent_bytes;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket");
        exit(1);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = inet_addr(argv[1]);

    printf("UDP Sender sending to IP %s on port %d\n", argv[1], PORT);

    for (int size = 1; size <= 1001; size += 100) {
        int msg_size = (size > 1) ? (size - 1) : size;
        memset(buf, 'a', msg_size);
        total_time = 0;
        total_sent_bytes = 0;

        // Send COUNT and msg_size to the receiver
        char info_msg[BUFSIZE];
        snprintf(info_msg, BUFSIZE, "%d:%d", COUNT, msg_size);
        sendto(sock, info_msg, strlen(info_msg), 0, (struct sockaddr *)&addr, sizeof(addr));

        // Wait for acknowledgment from the receiver
        char ack_msg[BUFSIZE];
        recvfrom(sock, ack_msg, BUFSIZE, 0, (struct sockaddr *)&addr, &addr_len);

        printf("Message Size: %d bytes\n", msg_size);
        printf("Progress: ");
        fflush(stdout);

        for (int i = 0; i < COUNT; i++) {
            clock_gettime(CLOCK_MONOTONIC, &start);
            sendto(sock, buf, msg_size, 0, (struct sockaddr *)&addr, sizeof(addr));
            int len = recvfrom(sock, buf, BUFSIZE, 0, (struct sockaddr *)&addr, &addr_len);
            clock_gettime(CLOCK_MONOTONIC, &end);

            long seconds = end.tv_sec - start.tv_sec;
            long ns = end.tv_nsec - start.tv_nsec;

            if (start.tv_nsec > end.tv_nsec) { // clock underflow
                --seconds;
                ns += 1000000000;
            }

            total_time += seconds * 1000000000 + ns;
            total_sent_bytes += len;

            double progress = (double)(i + 1) / COUNT;
            printProgressBar(progress, i + 1);
        }

        double mean_time = (double)total_time / COUNT / 1000000000;

        printf("\nTotal Time for the Test: %ld nanoseconds\n", total_time);
        printf("Mean Time: %e seconds\n", mean_time);
        printf("Total Sent Bytes: %ld bytes\n", total_sent_bytes);
        printf("---------------------------------------\n");
    }

    return 0;
}
