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
int						numerical = 0;
char					*ip, *reverse_hostname;
int						ttl = 64;
pid_t					pid;
struct hostent			*host;
struct sockaddr_in		addr;
struct sockaddr_in		recv_ret;
int						got_it = 1;
int						intro = 1;

// values for final message
long long unsigned int	received_packets = 0;
long long unsigned int	dead_packets = 0;
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
	char				rbuffer[128];
	struct				timeval start, end;
    struct ping_pkt 	pckt;
	struct icmphdr		*recvhdr_tmp;
	int					bytes_received;
	int					bytes_sent;
	double				rtt;
	int					msg_count = 0;
	long unsigned int	i;
	unsigned int		recv_ret_len;

	got_it = 1;
	
	bzero(&pckt, sizeof(pckt));
	pckt.hdr.type = ICMP_ECHO;
	pckt.hdr.un.echo.id = getpid();
	for ( i = 0; i < sizeof(pckt.msg) - 1; i++)
        pckt.msg[i] = i + '0';
	pckt.msg[i] = 0;
	pckt.hdr.un.echo.sequence = msg_count++;
	// Calculate checksum (RFC 1071)
    pckt.hdr.checksum = checksum(&pckt, sizeof(pckt));

	gettimeofday(&start, NULL);

	// Send ICMP packet
	bytes_sent = sendto(sockfd, &pckt, PACKET_SIZE, 0, (struct sockaddr *)addr, sizeof(*addr));
	if (bytes_sent <= 0)
		return (perror("sendto"), got_it = 0, -1);

	// Receive ICMP reply
	recv_ret_len = sizeof(recv_ret);
	bytes_received = recvfrom(sockfd, rbuffer, PACKET_SIZE, 0, (struct sockaddr *)&recv_ret, &recv_ret_len);
	if (bytes_received <= 0)
	{
		printf("From %s (%s): icmp_seq=%d Time to live exceeded\n", reverse_hostname, inet_ntoa(addr->sin_addr), seq);
		dead_packets++;
		got_it = 0;
	}
	
	// ip_header = (struct iphdr *)packet;
	// printf("JE SUIS LE IP_HEADER->TTL %d\n", ip_header->ttl);
	recvhdr_tmp = (struct icmphdr *)rbuffer;
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

	if (got_it && !quiet)
	{
		if (verbose && verbose_bool++ == 0)
			printf("ping : sock4.fd: %d (socktype SOCK_RAW)\n\nai->ai_family: AF_INET, ai->ai_canonname: '%s'\n", sockfd, target_name);
		if (intro == 1 )
		{
			printf("PING %s (%s) 56(84) bytes of data.\n", target_name, inet_ntoa(addr->sin_addr));
			intro = 0;
		}
		if (!(recvhdr_tmp->type == 69 && recvhdr_tmp->code == 0 ))
		{
			if (numerical)
				printf("From %s: icmp_seq=%d Time to live exceeded\n", inet_ntoa(addr->sin_addr), seq);
			else
				printf("From %s (%s): icmp_seq=%d Time to live exceeded\n", reverse_hostname, inet_ntoa(addr->sin_addr), seq);
			dead_packets++;
		}
		else
		{
			if (verbose)
			{
				if (numerical)
					printf("%d bytes from %s: icmp_seq=%d ident=%d ttl=%d time=%.2f ms\n", bytes_received, inet_ntoa(addr->sin_addr), seq, pid, ttl - 1, rtt);
				else
					printf("%d bytes from %s (%s): icmp_seq=%d ident=%d ttl=%d time=%.2f ms\n", bytes_received, reverse_hostname, inet_ntoa(addr->sin_addr), seq, pid, ttl - 1, rtt);
			}
			else
			{
				if (numerical)
					printf("%d bytes from %s: icmp_seq=%d ttl=%d time=%.2f ms\n", bytes_received, inet_ntoa(addr->sin_addr), seq, ttl - 1,rtt);
				else
					printf("%d bytes from %s (%s): icmp_seq=%d ttl=%d time=%.2f ms\n", bytes_received, reverse_hostname, inet_ntoa(addr->sin_addr), seq, ttl - 1,rtt);
			}
		}
	}
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
			else if (strcmp(argv[i], "-?") == 0) // mandatory flag
				return (print_usage(), 1);
			else if (strcmp(argv[i], "-a") == 0) // makes an audible ping
				audible = 1;
			else if (strcmp(argv[i], "-n") == 0) // makes it numerical only
				numerical = 1;
			else if (strcmp(argv[i], "-t") == 0) // changes the ttl
			{
				if (argv[i+1])
					ttl = atoi(argv[i + 1]);
				else
					return (printf("ping: option requires an argument -- 't'\n\n"), print_usage(), 1);
			}
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
			else if (strcmp(argv[i], "-q") == 0) //silences the output
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
	printf("%lld packets transmitted, %lld received, %LG%% packet loss, time %lu ms\n", num_pings, received_packets - dead_packets, 100 - ((((long double)received_packets - (long double)dead_packets) / (long double)num_pings) * 100), ((stop.tv_sec - start.tv_sec) * 1000000 + stop.tv_usec - start.tv_usec )/1000);
	printf("rtt min/avg/max/mdev = %.3f/%.3f/%.3f/%.3f ms\n", min, total_time / num_pings, max, mdev);
}

void	signal_time(int signal)
{
	if (signal == 2)
	{
		print_stats();
		free(target_name);
		free_list(value_list);
		free(ip);
		free(reverse_hostname);
	}
	exit(0);
}

int		main(int argc, char **argv) 
{
	int					setsockopt_ret;
	int					sockfd;
	int					seq = 1;
	int 				ping_return = 0;
	int					foundTarget = 0;
	int 				j = 0;
    struct timeval		tv_out;


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

	target_name = strdup(argv[foundTarget]);
	// Resolve hostname to IP address
	host = gethostbyname(target_name);
	if (host == NULL)
		return (printf("ft_ping: cannot resolve %s: Unknown host\n", argv[foundTarget]), 1);

	// Fill in address structure
	ip = strdup(inet_ntoa(*(struct in_addr *)host->h_addr));
	addr.sin_family = host->h_addrtype;
	addr.sin_port = htons(0);
	addr.sin_addr.s_addr = *(long *)host->h_addr;

	// Getnameinfo from addr to get the true server name through reverse proxy
	reverse_hostname = reverse_dns_lookup(ip);
	if (reverse_hostname == NULL)
		return(printf("Could not resolve hostname\n"), 1);

	// Create socket
	sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (sockfd < 0)
		return (perror("socket"), 1);

	// Allows to set the ttl of the packet
	setsockopt_ret = setsockopt(sockfd, SOL_IP, IP_TTL, &ttl, sizeof(ttl));
	if (setsockopt_ret < 0)
		return (perror("setsockopt"), -1);

	// Setting timout for packets
	memset(&tv_out, 0, sizeof(tv_out)); // initialize memory of tv_out
	tv_out.tv_usec = 10000; // 10ms
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv_out, sizeof tv_out);

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
				free(ip);
				free(reverse_hostname);
				return (0);
			}
		}
		ping_return = send_ping(sockfd, &addr, seq++);
		if (ping_return == -1)
		{
			print_usage();
			free_list(value_list);
			free(target_name);
			free(ip);
			free(reverse_hostname);
			return (1);
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
