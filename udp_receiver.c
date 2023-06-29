#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFSIZE 1024
#define PORT 55555

void printProgressBar(double progress, int count, int totalCount)
{
    int barWidth = 70;
    int filledWidth = progress * barWidth;

    printf("\r[");
    for (int i = 0; i < filledWidth; ++i)
    {
        printf("=");
    }
    printf(">");
    for (int i = filledWidth; i < barWidth; ++i)
    {
        printf(" ");
    }
    printf("] %.2f%% (%d/%d)", progress * 100, count, totalCount);
    fflush(stdout);
}

int main()
{
    int sock;
    struct sockaddr_in addr;
    char buf[BUFSIZE];
    socklen_t addr_len;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
    {
        perror("socket");
        exit(1);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("bind");
        exit(1);
    }

    printf("UDP Receiver running on port %d\n", PORT);

    while (1)
    {
        addr_len = sizeof(addr);

        // Receive COUNT, msg_size, and window size from the sender
        char info_msg[BUFSIZE];
        recvfrom(sock, info_msg, BUFSIZE, 0, (struct sockaddr *)&addr, &addr_len);

        // Parse COUNT, msg_size, and window size values
        int count, msg_size, window_size;
        sscanf(info_msg, "%d:%d:%d", &count, &msg_size, &window_size);

        printf("Received message size: %d bytes\n", msg_size);
        printf("Total messages to receive: %d\n", count);

        // Send acknowledgment to the sender
        sendto(sock, "ACK", 3, 0, (struct sockaddr *)&addr, sizeof(addr));

        // Start receiving the messages with progress bar
        int received_count = 0;
        int echoed_count = 0;
        double progress = 0.0;

        addr_len = sizeof(addr);

        while (1)
        {
            int len = recvfrom(sock, buf, BUFSIZE, 0, (struct sockaddr *)&addr, &addr_len);
            if (len > 0)
            {
                // Check if it's a stop message
                if (strcmp(buf, "STOP") == 0)
                {
                    printf("\nReceived stop message. Echoed messages: %d\n", echoed_count);
                    break;
                }

                // Update progress
                received_count++;
                echoed_count++;
                progress = (double)received_count / count;
                printProgressBar(progress, received_count, count);

                // Retrieve the message ID
                int messageID = 0;
                sscanf(buf, "%d", &messageID);
                printf("\rMessage ID: %d received\n", messageID);
                fflush(stdout);

                sendto(sock, buf, len, 0, (struct sockaddr *)&addr, sizeof(addr));

                // Check if all messages for the current batch have been received
                if (received_count >= count)
                {
                    printf("\n");
                    break;
                }
            }
        }
    }

    return 0;
}
