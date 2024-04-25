#include <stdio.h>
// #include <stdlib.h>
#include <unistd.h>
// #include <sys/socket.h>
// #include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <sys/time.h>

#define PACKET_SIZE     64
#define PING_TIMEOUT    2

int verbose = 0;

// Calculate checksum for ICMP packet
unsigned short checksum(void *b, int len) {

    unsigned short *buf = b;
    unsigned int sum = 0;
    unsigned short result;

    for (sum = 0; len > 1; len -= 2)
        sum += *buf++;

    if (len == 1)
        sum += *(unsigned char *)buf;

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;

    return result;
}

// Send ICMP echo request and wait for response
void send_ping(int sockfd, struct sockaddr_in *addr, int seq) {
    char packet[PACKET_SIZE];

    //using icmp struct from ip_icmp.h
    struct icmp *icmp_hdr;
    struct timeval start, end;
    int bytes_sent, bytes_received;
    socklen_t addr_len;
    double rtt;

    // Prepare ICMP header
    icmp_hdr = (struct icmp *) packet;
    icmp_hdr->icmp_type = ICMP_ECHO;
    icmp_hdr->icmp_code = 0;
    icmp_hdr->icmp_id = getpid();
    icmp_hdr->icmp_seq = seq;
    icmp_hdr->icmp_cksum = 0;
    gettimeofday(&start, NULL);

    // Calculate checksum
    icmp_hdr->icmp_cksum = checksum(icmp_hdr, PACKET_SIZE);

    // Send ICMP packet
    bytes_sent = sendto(sockfd, packet, PACKET_SIZE, 0, (struct sockaddr *) addr, sizeof(*addr));
    if (bytes_sent <= 0) {

        perror("sendto");
        return;
    }

    // Receive ICMP reply
    addr_len = sizeof(*addr);
    bytes_received = recvfrom(sockfd, packet, PACKET_SIZE, 0, (struct sockaddr *) addr, &addr_len);
    if (bytes_received <= 0) {
        perror("recvfrom");
        return;
    }

    // Calculate RTT
    gettimeofday(&end, NULL);
    rtt = (double) (end.tv_usec - start.tv_usec) / 1000.0;
    if (verbose)
        printf("%d bytes from %s: icmp_seq=%d time=%.3f ms\n", bytes_received, inet_ntoa(addr->sin_addr), seq, rtt);
    else
        printf("Ping reply received from %s: icmp_seq=%d time=%.3f ms\n", inet_ntoa(addr->sin_addr), seq, rtt);
}

int arg_finder(int argc, char **argv) {

    int i;

    i = 0;
    while (i < argc) {
        if (strcmp(argv[i], "-v") == 0) {
            return 1;
        }
        i++;
    }
    return 0;
}

int target_finder(int argc, char **argv) {

    int i;

    i = 1;
    while (i < argc) {
        printf("Looking at target %s\n", argv[i]);
        if (argv[i][0] == '-')
            i++;
        else {
            printf("Target: %s\n", argv[i]);
            return i;
        }
        // i++;
    }
    return -1;
}

int main(int argc, char **argv) {

    struct hostent *host;
    struct sockaddr_in addr;
    int sockfd, seq = 0;

    printf("%d\n", argc);

    if (argc < 2) {
        printf("Usage: %s [-v] <hostname or IP>\n", argv[0]);
        return 1;
    }
    if (arg_finder(argc, argv) == 1) {
        verbose = 1;
        printf("Verbose mode enabled\n");
    }

    int foundTarget = 0;
    foundTarget = target_finder(argc, argv);
    printf("Found target at index: %d\n", foundTarget);
    if (foundTarget == -1) {
        printf("No target found\n");
        return 1;
    }

    // Resolve hostname to IP address
    host = gethostbyname(argv[foundTarget]);
    if (host == NULL) {
        printf("ft_ping: cannot resolve %s: Unknown host\n", argv[foundTarget]);
        return 1;
    }

    // Create socket
    sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    // Fill in address structure
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr = *((struct in_addr *) host->h_addr);

    // Send ICMP echo requests and receive replies
    while (1) {
        send_ping(sockfd, &addr, seq++);
        sleep(1);
    }

    // Close socket
    close(sockfd);

    return 0;
}
