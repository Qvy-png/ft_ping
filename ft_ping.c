#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
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
int audible = 0;
int count = -1;
float timer = 1;
int exit_after_reply = 0;
int quiet = 0;

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
int send_ping(int sockfd, struct sockaddr_in *addr, int seq) {
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
        return -1;
    }

    // Receive ICMP reply
    addr_len = sizeof(*addr);
    bytes_received = recvfrom(sockfd, packet, PACKET_SIZE, 0, (struct sockaddr *) addr, &addr_len);
    if (bytes_received <= 0) {
        perror("recvfrom");
        return -1;
    }
    // else if ( )

    // Calculate RTT
    gettimeofday(&end, NULL);
    rtt = (double) (end.tv_usec - start.tv_usec) / 1000.0;
    if (verbose)
        printf("%d bytes from %s: icmp_seq=%d time=%.3f ms\n", bytes_received, inet_ntoa(addr->sin_addr), seq, rtt);
    else
        printf("Ping reply received from %s: icmp_seq=%d time=%.3f ms\n", inet_ntoa(addr->sin_addr), seq, rtt);
    return 0;
}

int isNum(char *str) {
    int i = 0;

    if (str == NULL)
        return 0;
    while (str[i] != '\0') {
        if (str[i] < '0' || str[i] > '9')
            return 0;
        i++;
    }
    return 1;
}

int isFloat(char *str) {

    int i = 0;

    if (str == NULL)
        return 0;
    while (str[i] != '\0') {
        if ((str[i] < '0' || str[i] > '9') && str[i] != '.')
            return 0;
        i++;
    }

    return 1;
}

int arg_finder(int argc, char **argv) {

    int i;

    i = 0;
    while (i < argc) {
        if (argv[i][0] == '-')
        {
            if (strcmp(argv[i], "-v") == 0) { // mandatory flag
                verbose = 1;
            }
            else if (strcmp(argv[i], "-a") == 0) { //makes an audible ping
                audible = 1;
            }
            else if (strcmp(argv[i], "-c") == 0) {
                // count = 1; // nbr de count
                if (isNum(argv[i+1])) {
                    count = atoi(argv[i+1]);
                    printf("Debug : Count: %d\n", count);
                    i++;
                }
                else {
                    if (argv[i+1] == NULL){
                        //TODO print usage, replacing the current message
                        printf("ping: option requires an argument -- 'c'\n");
                    }
                    else
                        printf("ping: invalid count of packets to transmit: `%s'\n", argv[i+1]);
                    return 1;
                }
            }
            else if (strcmp(argv[i], "-i") == 0) {
                printf("well hello there ! ");
                if (isFloat(argv[i+1])) {
                    timer = atof(argv[i+1]);
                    printf("Debug : Timer: %f\n", timer);
                    
                    i++;
                }
                else {
                    if (argv[i+1] == NULL){
                        //TODO print usage, replacing the current message
                        printf("ping: option requires an argument -- 'i'\n");
                    }
                    else
                        printf("ping: invalid interval time: `%s'\n", argv[i+1]);
                    return 1;
                }
            }
            else if (strcmp(argv[i], "-f") == 0) { //TODO flood mode, no waiting time and only displays one dot while flooding
                
            }
            else if (strcmp(argv[i], "-q") == 0) { //TODO retirer les messages de ping
                quiet = 1;
            }
            else
                return -1;
        }
        i++;
    }
    return 0;
}

int target_finder(int argc, char **argv) {

    int i;
    int target_count = 0;
    int found_target = -1;

    i = 1;
    while (i < argc) {
        printf("Debug : Looking at target %s\n", argv[i]);
        if (argv[i][0] == '-' || isFloat(argv[i]))
            i++;
        else {
            printf("Debug : Target: %s\n", argv[i]);
            target_count++;
            found_target = i;
            i++;
        }
    }

    printf("Debug : Target count: %d\n", target_count);
    if (target_count > 1)
        return -1;
    else
        return found_target;
}

// void signal_time( void ) {

//     struct sigaction act;

//     bzero(&act, sizeof(act));
	
// 	act.sa_handler = &sa_handler;

// 	sigaction(SIGINT, &act, NULL);
// }

int main(int argc, char **argv) { //TODO signal handler pour ctrl+c

    struct hostent *host;
    struct sockaddr_in addr;
    int sockfd, seq = 0;
    int j = 0;
    int ping_return = 0;
    unsigned int end = 0;

    printf("Debug : argc: %d\n", argc);

    if (argc < 2) {
        printf("Usage: %s [-v] <hostname or IP>\n", argv[0]);
        return 1;
    }
    j = arg_finder(argc, argv);
    if (j != 0) {
        if (j == -1)
            printf("ping: invalid option -- '%s'\n", argv[j]);
        return 1;
    }

    int foundTarget = 0;
    foundTarget = target_finder(argc, argv);
    printf("Debug : Found target at index: %d\n", foundTarget);
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

    // signal_time(); //TODO

    // Send ICMP echo requests and receive replies
    while (1) {

        if (count >= 0) {
            printf("Debug : Count: %d\n", count);
            if (count > 0)
                count--;
            else
                return 0; // TODO Call the summary function
        }
        ping_return = send_ping(sockfd, &addr, seq++);
        if (ping_return == -1) {
            printf("Debug : Ping failed\n"); //TODO print the ping usage
            return 1;
        }
        if (audible == 1)
            printf("\7");
        if (count != 0) //permet de ne pas faire de sleep sur le dernier du count pour avoir un exit plus rapide
            sleep(timer);
    }

    // Close socket
    close(sockfd);

    return 0;
}

//TODO changer les flags pour le bon ping de inetutils-2.0
// https://manpages.debian.org/bullseye/inetutils-ping/ping.1.en.html

// -c -i -q -f

//TODO intercepter les signaux

//TODO get all info from the send_ping function to be able to write down the statistics
//time = adding up all the ms + number of ms from the intervals
//packets transmitted = packages sent
//received = replies from the ip
//packet loss = number of times there was no reply

//TODO les 5 flags à faire : -a -c -i -o -q