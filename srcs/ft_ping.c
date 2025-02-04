#include "../includes/ft_ping.h"

// global variables
int						exit_after_reply = 0;
char					*target_name;
int						verbose = 0;
int						verbose_bool = 0;
int						audible = 0;
int						count = -1;
float					timer = 1;
int						quiet = 0;
int						flood = 1;
pid_t					pid;
struct hostent			*host;
struct sockaddr_in		addr;

// values for final message
long long unsigned int	received_packets;
double					total_time = 0;
long long unsigned int	num_pings = 0;
double					mdev = 0;
double					max = 0;
double					min = 0;

// allows to calculate the total exec time
struct timeval			stop, start;
time_t					begin;

// special mdev
t_mean					*value_list = NULL;

// Send ICMP echo request and wait for response
int	send_ping(int sockfd, struct sockaddr_in *addr, int seq)
{
	// using icmp struct from ip_icmp.h
	char		packet[PACKET_SIZE];
	struct		timeval start, end;
	struct		icmp *icmp_hdr;
	int			bytes_received;
	int			bytes_sent;
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
		return (perror("sendto"), -1);

	// Receive ICMP reply
	addr_len = sizeof(*addr);
	bytes_received = recvfrom(sockfd, packet, PACKET_SIZE, 0, (struct sockaddr *) addr, &addr_len);
	if (bytes_received <= 0)
		return (perror("recvfrom\n"), -1);
	received_packets++;

	// Calculate RTT
	gettimeofday(&end, NULL);
	if (end.tv_usec >= start.tv_usec)
		rtt = (double) (end.tv_usec - start.tv_usec) / 1000.0;

	if (rtt > max)
		max = rtt;
	if (rtt < min || min == 0)
		min = rtt;
	num_pings++;
	total_time = total_time + rtt;
	mdev = max - min;
	pid = getpid();

	list_push(&value_list, rtt);

	// TODO changer le code pour me rapprocher du ping officiel
	// TODO utiliser getnameinfo pour pouvoir récupérer l'adresse du server de reverse proxy
	if (verbose)
	{
		if (verbose_bool++ == 0)
			printf("ping : sock4.fd: %d (socktype SOCK_RAW)\n\nai->ai_family: AF_INET, ai->ai_canonname: '%s'\n", sockfd, target_name);
		printf("%d bytes from %s: icmp_seq=%d ident=%d ttl=%d time=%.3f ms\n", bytes_received, inet_ntoa(addr->sin_addr), seq, pid, 0, rtt);
	}
	else
		printf("%d bytes from %s: icmp_seq=%d ttl=%d time=%.3f ms\n", bytes_received, inet_ntoa(addr->sin_addr), seq, 0,rtt); //TODO ttl
	return (0);
}

int arg_finder(int argc, char **argv)
{
	int	i;

	i = 0;
	while (i < argc)
	{
		if (argv[i][0] == '-')
		{
			if (strcmp(argv[i], "-v") == 0) // mandatory flag
				verbose = 1;
			else if (strcmp(argv[i], "-?") == 0)
				print_usage();
			else if (strcmp(argv[i], "-a") == 0) // makes an audible ping
				audible = 1;
			else if (strcmp(argv[i], "-c") == 0) // pings only <count> times
			{
				if (is_num(argv[i+1]))
				{
					count = atoi(argv[i+1]);
					i++;
				}
				else
				{
					if (argv[i+1] == NULL)
					{
						printf("ping: option requires an argument -- 'c'\n\n");
						print_usage();
					}
					else
						printf("ping: invalid count of packets to transmit: `%s'\n", argv[i+1]);
					return (1);
				}
			}
			else if (strcmp(argv[i], "-q") == 0) //TODO retirer les messages de ping
				quiet = 1;
			else
				return (i);
		}
		i++;
	}
	return (0);
}

void	mdev_calculation(t_mean **head)
{
	double					mean = total_time/num_pings;
	t_mean					*tmp = *head;
	long long unsigned int	i = 1;

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
	printf("rtt min/avg/max/mdev = %.3f/%.3f/%.3f/%.3f ms\n", min, total_time / num_pings, max, mdev);
}

void	signal_time(int signal)
{
	if (signal == 2)
	{
		print_stats();
		free(target_name);
		free_list(value_list);
	}
	exit(0);
}

int		main(int argc, char **argv) 
{
	int					sockfd, seq = 0;
	int 				ping_return = 0;
	int					foundTarget = 0;
	unsigned int		end = 0;
	int 				j = 0;

	(void)end;
	if (argc < 2)
		return (printf("Usage: %s [-v] <hostname or IP>\n", argv[0]), 1);
	j = arg_finder(argc, argv);
	if (j != 0)
		return (1);
	foundTarget = target_finder(argc, argv);
	if (foundTarget == -1)
		return (printf("No target found\n"), 1);

	begin =	time(NULL);
  	gettimeofday(&start, NULL);

	// Resolve hostname to IP address
	host = gethostbyname(argv[foundTarget]);
	target_name = strdup(argv[foundTarget]);
	if (host == NULL)
		return (printf("ft_ping: cannot resolve %s: Unknown host\n", argv[foundTarget]), 1);

	// Create socket
	sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (sockfd < 0)
		return (perror("socket"), 1);

	// Fill in address structure
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr = *((struct in_addr *) host->h_addr);
	value_list = malloc(sizeof(t_mean));
	if (value_list == NULL)
		return (1);
	if (value_list)
		memset(value_list, 0, sizeof(t_mean));

	// Signal
	signal(SIGINT, signal_time);

	// Send ICMP echo requests and receive replies
	while (1)
	{
		if (count >= 0)
		{
			if (count > 0)
				count--;
			else
			{
				print_stats();
				free_list(value_list);
				free(target_name);
				return (0);
			}
		}
		ping_return = send_ping(sockfd, &addr, seq++);
		if (ping_return == -1)
		{
			printf("Debug : Ping failed\n"); //TODO print the ping usage
			return 1;
		}
		if (audible == 1)
			printf("\7");
		if (count != 0)
			sleep(timer);
	}
	close(sockfd);
	free_list(value_list);
	free(target_name);
	return (0);
}

//TODO changer les flags pour le bon ping de inetutils-2.0
// https://manpages.debian.org/bullseye/inetutils-ping/ping.1.en.html

// -c -q -n
