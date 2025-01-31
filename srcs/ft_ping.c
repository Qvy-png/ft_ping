#include "../includes/ft_ping.h"

// global variables
int						verbose = 0;
int						audible = 0;
int						count = -1;
float					timer = 1;
int						exit_after_reply = 0;
int						quiet = 0;
char					*target_name;

// plusieurs valuers, le max, le min, le nombre de pings, la moyenne calculée au fur à mesure, la mdev calculée sur max - min
// values for final message
double					max = 0;
double					min = 0;
double					total_time = 0;
double					mdev = 0;
long long unsigned int	num_pings = 0;
long long unsigned int	received_packets;

time_t begin;
 struct timeval stop, start;


// special mdev
t_mean *value_list;

// Calculate checksum for ICMP packet
unsigned short	checksum(void *b, int len) {

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
int 	send_ping(int sockfd, struct sockaddr_in *addr, int seq) {


	// using icmp struct from ip_icmp.h
	struct		icmp *icmp_hdr;
	char		packet[PACKET_SIZE];
	struct		timeval start, end;
	int			bytes_sent, bytes_received;
	socklen_t	addr_len;
	double		rtt;

	// Prepare ICMP header
	icmp_hdr = (struct icmp *) packet;
	memset(icmp_hdr, 0, PACKET_SIZE);
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
	if (bytes_sent <= 0)
	{
		perror("sendto");
		return (-1);
	}

	// Receive ICMP reply
	addr_len = sizeof(*addr);
	bytes_received = recvfrom(sockfd, packet, PACKET_SIZE, 0, (struct sockaddr *) addr, &addr_len);
	if (bytes_received <= 0)
	{
		perror("recvfrom");
		return (-1);
	}
	received_packets++;
	// else if ( )

	// Calculate RTT
	gettimeofday(&end, NULL);
	rtt = (double) (end.tv_usec - start.tv_usec) / 1000.0;

	// preparations for the final message
	if (rtt > max)
		max = rtt;
	if (rtt < min || min == 0)
		min = rtt;
	num_pings++;
	total_time = total_time + rtt;
	mdev = max - min;
	//
	list_push(&value_list, rtt);
	if (verbose)
		printf("%d bytes from %s: icmp_seq=%d time=%.3f ms\n", bytes_received, inet_ntoa(addr->sin_addr), seq, rtt);
	else
		printf("Ping reply received from %s: icmp_seq=%d time=%.3f ms\n", inet_ntoa(addr->sin_addr), seq, rtt);
	
	return 0;
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
				if (is_num(argv[i+1])) {
					count = atoi(argv[i+1]);
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
				if (is_float(argv[i+1])) {
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

int target_finder(int argc, char **argv)
{
	int		i;
	int		target_count = 0;
	int		found_target = -1;

	i = 1;
	while (i < argc) {
		if (argv[i][0] == '-' || is_float(argv[i]))
			i++;
		else {
			target_count++;
			found_target = i;
			i++;
		}
	}
	if (target_count > 1)
		return -1;
	else
		return found_target;
}

void	mdev_calculation(t_mean **head)
{
	double					mean = total_time/num_pings;
	long long unsigned int	i = 1;
	t_mean					*tmp = *head;

	while(i <= num_pings)
	{
		if ((tmp->value - mean) < 0)
			mdev -= (tmp->value - mean);
		else
			mdev += (tmp->value - mean);
		tmp = tmp->next;
		i++;
	}
	mdev /= num_pings;
}

void	print_stats(void)
{
	gettimeofday(&stop, NULL);
	
	mdev_calculation(&value_list);
	printf("\n--- %s ping statistics ---\n", target_name);
	printf("%lld packets transmitted, %lld received, %LG%% packet loss, time %lu ms\n", num_pings, received_packets, 100 - (((long double)received_packets / (long double)num_pings) * 100), ((stop.tv_sec - start.tv_sec) * 1000000 + stop.tv_usec - start.tv_usec )/1000);
	printf("rtt min/avg/max/mdev = %g/%g/%g/%g ms\n", min, total_time / num_pings, max, mdev);
}

void	signal_time(int signal)
{
	if (signal == 2)
	{
		print_stats();
		free(target_name);
		// free_list(value_list);
	}
	exit(0);
}

int		main(int argc, char **argv) 
{
	struct hostent *host;
	struct sockaddr_in addr;
	int sockfd, seq = 0;
	int j = 0;
	int ping_return = 0;
	unsigned int end = 0;
	(void)end;

	if (argc < 2) {
		printf("Usage: %s [-v] <hostname or IP>\n", argv[0]);
		return 1;
	}
	j = arg_finder(argc, argv);
	if (j != 0) {
		if (j == -1)
			printf("ping: invalid option -- '%s'\n", argv[j]);
		return (1);
	}

	int foundTarget = 0;
	foundTarget = target_finder(argc, argv);
	if (foundTarget == -1) {
		printf("No target found\n");
		return 1;
	}

	//TODO
	begin =	time( NULL );
  	gettimeofday(&start, NULL);

	// Resolve hostname to IP address
	host = gethostbyname(argv[foundTarget]);
	target_name = strdup(argv[foundTarget]);
	if (host == NULL) {
		printf("ft_ping: cannot resolve %s: Unknown host\n", argv[foundTarget]);
		return (1);
	}

	// Create socket
	sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (sockfd < 0) {
		perror("socket");
		return (1);
	}

	// Fill in address structure
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr = *((struct in_addr *) host->h_addr);

	value_list = malloc(sizeof(t_mean));

	// Signal
	signal(SIGINT, signal_time);

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

//TODO get all info from the send_ping function to be able to write down the statistics
//time = adding up all the ms + number of ms from the intervals
//packets transmitted = packages sent
//received = replies from the ip
//packet loss = number of times there was no reply

//TODO les 5 flags à faire : -a -c -i -o -q