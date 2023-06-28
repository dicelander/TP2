#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFSIZE 1024
#define PORT 555555

int main() {
    int sock;
    struct sockaddr_in addr;
    char buf[BUFSIZE];
    socklen_t addr_len;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock < 0) {
        perror("socket");
        exit(1);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if(bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        exit(1);
    }

    while(1) {
        addr_len = sizeof(addr);
        int len = recvfrom(sock, buf, BUFSIZE, 0, (struct sockaddr *)&addr, &addr_len);
        if(len > 0) {
            sendto(sock, buf, len, 0, (struct sockaddr *)&addr, sizeof(addr));
        }
    }

    return 0;
}
