//
//  main.cpp
//  ping
//
//  Created by Chidume Nnamdi on 10/07/2025.
//

#include <iostream>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <netinet/ip_icmp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netdb.h>

struct icmphdr {
    uint8_t type;       // message type
    uint8_t code;       // type sub-code
    uint16_t checksum;
    union {
        struct {
            uint16_t id;
            uint16_t sequence;
        } echo;
        uint32_t gateway;
        struct {
            uint16_t __unused;
            uint16_t mtu;
        } frag;
    } un;
};

// ICMP header checksum calculation
unsigned short checksum(void* b, int len) {
    unsigned short* buf = (unsigned short*)b;
    unsigned int sum = 0;
    unsigned short result;

    for (sum = 0; len > 1; len -= 2)
        sum += *buf++;
    
    if (len == 1)
        sum += *(unsigned char*)buf;
    
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;
    return result;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <hostname or IP>\n";
        return 1;
    }

    const char* hostname = argv[1];

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;

    if (inet_pton(AF_INET, hostname, &addr.sin_addr) <= 0) {
        struct hostent* host = gethostbyname(hostname);
        if (!host) {
            std::cerr << "Host not found.\n";
            return 1;
        }
        addr.sin_addr = *(struct in_addr*)host->h_addr;
    }

    int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    char sendbuf[64];
    struct icmphdr* icmp = (struct icmphdr*)sendbuf;
    icmp->type = ICMP_ECHO;
    icmp->code = 0;
    icmp->un.echo.id = getpid();
    icmp->un.echo.sequence = 1;
    icmp->checksum = 0;
    icmp->checksum = checksum(icmp, sizeof(sendbuf));

    struct timeval start, end;
    gettimeofday(&start, NULL);

    if (sendto(sockfd, sendbuf, sizeof(sendbuf), 0, (struct sockaddr*)&addr, sizeof(addr)) <= 0) {
        perror("sendto");
        return 1;
    }

    char recvbuf[1024];
    socklen_t len = sizeof(addr);
    if (recvfrom(sockfd, recvbuf, sizeof(recvbuf), 0, (struct sockaddr*)&addr, &len) <= 0) {
        perror("recvfrom");
        return 1;
    }

    gettimeofday(&end, NULL);
    double rtt = (end.tv_sec - start.tv_sec) * 1000.0 +
                 (end.tv_usec - start.tv_usec) / 1000.0;

    std::cout << "Reply from " << inet_ntoa(addr.sin_addr)
              << ": time=" << rtt << " ms\n";

    close(sockfd);
    return 0;
}
