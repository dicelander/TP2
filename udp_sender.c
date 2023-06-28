#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#define BUFSIZE 1024
#define PORT 8080
#define COUNT 100000
#define IP "127.0.0.1"

int main() {
    int sock;
    struct sockaddr_in addr;
    char buf[BUFSIZE];
    socklen_t addr_len;
    struct timespec start, end;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock < 0) {
        perror("socket");
        exit(1);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = inet_addr(IP);

    for (int msg_size = 1; msg_size <= 1000; msg_size += 100) {
        memset(buf, 'a', msg_size);

        clock_gettime(CLOCK_MONOTONIC, &start);

        for (int i = 0; i < COUNT; i++) {
            sendto(sock, buf, msg_size, 0, (struct sockaddr *)&addr, sizeof(addr));
            recvfrom(sock, buf, BUFSIZE, 0, (struct sockaddr *)&addr, &addr_len);
        }

        clock_gettime(CLOCK_MONOTONIC, &end);

        long seconds = end.tv_sec - start.tv_sec;
        long ns = end.tv_nsec - start.tv_nsec;

        if (start.tv_nsec > end.tv_nsec) { // clock underflow 
	        --seconds;
	        ns += 1000000000;
        }

        printf("Round-trip latency for %d bytes is %e seconds\n", msg_size, (double)seconds + (double)ns/(double)1000000000);
    }

    return 0;
}
