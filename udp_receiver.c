#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFSIZE 1024
#define PORT 55555

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

    while (1) {
        addr_len = sizeof(addr);

        // Receive COUNT and msg_size from the sender
        char info_msg[BUFSIZE];
        recvfrom(sock, info_msg, BUFSIZE, 0, (struct sockaddr *)&addr, &addr_len);

        // Parse COUNT and msg_size values
        int count, msg_size;
        sscanf(info_msg, "%d:%d", &count, &msg_size);

        printf("Received message size: %d bytes\n", msg_size);
        printf("Total messages to receive: %d\n", count);

        // Send acknowledgment to the sender
        sendto(sock, "ACK", 3, 0, (struct sockaddr *)&addr, sizeof(addr));

        // Start receiving the messages with progress bar
        int received_count = 0;
        double progress = 0.0;
        printf("Progress: ");
        fflush(stdout);

        while (received_count < count) {
            int len = recvfrom(sock, buf, BUFSIZE, 0, (struct sockaddr *)&addr, &addr_len);
            if (len > 0) {
                printf("\rProgress: ");
                fflush(stdout);

                // Update progress
                received_count++;
                progress = (double)received_count / count;
                printProgressBar(progress, received_count, count);
                
                sendto(sock, buf, len, 0, (struct sockaddr *)&addr, sizeof(addr));
            }
        }

        printf("\n");
    }

    return 0;
}
