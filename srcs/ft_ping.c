#include "../includes/ft_ping.h"

// global variables
int						exit_after_reply = 0;
char					*target_name;
int						verbose = 0;
int						sockfd;
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
int						ping_received = 1;
int						intro = 1;

// flags for FQDN, reverse lookup and a bunch of other stuff

int						flag_hostname = 0;

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

int is_valid_ip(const char *str)
{
	struct sockaddr_in sa;
	return (inet_pton(AF_INET, str, &(sa.sin_addr)) != 0);
}

int target_finder(int argc, char **argv)
{
	int found_target = -1;
	int target_count = 0;
	int i = 1;

	while (i < argc)
	{
		if (argv[i][0] == '-') // Ignore flags
			i++;
		else if (is_valid_ip(argv[i]) || isalpha(argv[i][0])) // Accept IP or FQDN
		{
			target_count++;
			found_target = i;
			if (is_valid_ip(argv[i]))
				numerical = 1;
			i++;
		}
		else
			i++;
	}

	if (target_count > 1)
		return -1;
	else
		return found_target;
}


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

	ping_received = 1;
	
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
		return (perror("sendto"), ping_received = 0, -1);

	// Receive ICMP reply
	recv_ret_len = sizeof(recv_ret);
	bytes_received = recvfrom(sockfd, rbuffer, PACKET_SIZE, 0, (struct sockaddr *)&recv_ret, &recv_ret_len);
	if (bytes_received <= 0)
	{
		printf("From %s (%s): icmp_seq=%d Time to live exceeded\n", reverse_hostname, inet_ntoa(addr->sin_addr), seq);
		dead_packets++;
		ping_received = 0;
	}
	
	// ip_header = (struct iphdr *)packet;
	recvhdr_tmp = (struct icmphdr *)rbuffer;
	received_packets++;

	// Calculate RTT
	gettimeofday(&end, NULL);
	// printf("end.tv_sec: %ld and end.tv_usec: %ld\n", end.tv_sec * 1000000 + end.tv_usec,  end.tv_usec);
	if (end.tv_usec >= start.tv_usec)
		rtt = (double) ((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec)) / 1000.0;

	if (rtt > max)
		max = rtt;
	if (rtt < min || min == 0)
		min = rtt;
	num_pings++;
	total_time = total_time + rtt;
	mdev = max - min;
	pid = getpid();

	list_push(&value_list, rtt);

	if (ping_received && !quiet)
	{
		// printing the second half of the verbose message if ping is received
		if (verbose && verbose_bool++ == 0)
			printf("ai->ai_family: AF_INET, ai->ai_canonname: '%s'\n", target_name);
		
		// printing the introduction message
		if (intro == 1 )
		{
			printf("PING %s (%s) 56(84) bytes of data.\n", target_name, inet_ntoa(addr->sin_addr));
			intro = 0;
		}

		// handles ttl too short
		if (!(recvhdr_tmp->type == 69 && recvhdr_tmp->code == 0 ))
		{
			// printf("That thang can't live\n");
			printf("From %s: icmp_seq=%d Time to live exceeded\n", inet_ntoa(addr->sin_addr), seq);
			dead_packets++;
		}
		else
		{
			// handling ping message
			
			/// handling the bytes to print
			printf("%d bytes ", bytes_received);
	
			/// handling numerical (to only show numerical), or when not finding reverse lookup address
			if (numerical || flag_hostname)
				printf("from %s:", inet_ntoa(addr->sin_addr));
			else
				printf("from %s (%s): ", reverse_hostname, inet_ntoa(addr->sin_addr));
	
			printf("icmp_seq=%d ", seq);
	
			/// handling verbose with the additional pid to display
			if (verbose)
				printf("ident=%d ", pid);
			// TODO avant de rendre le TTL, checker avec le vrai ping de la fonction ping sur la VM s'il faut vraiment récup le TTL du retour
			// voir s'il y a besoin de display le ttl - 1
			printf("ttl=%d time=%.2f ms\n", ttl - 1, rtt);
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

// Resolve the reverse lookup of the hostname
char *reverse_dns_lookup(char *ip_addr) {
    struct sockaddr_in temp_addr;
    socklen_t len;
    char buf[NI_MAXHOST], *ret_buf;

    temp_addr.sin_family = AF_INET;
    temp_addr.sin_addr.s_addr = inet_addr(ip_addr);
    len = sizeof(struct sockaddr_in);

    if (getnameinfo((struct sockaddr *)&temp_addr, len, buf, sizeof(buf), NULL, 0, NI_NAMEREQD))
        return NULL;

    ret_buf = (char *)malloc((strlen(buf) + 1) * sizeof(char));
    strcpy(ret_buf, buf);
    return ret_buf;
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
		close(sockfd);
	}
	exit(0);
}

int		main(int argc, char **argv) 
{
	int					setsockopt_ret;
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
	
	// Create socket
	sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (sockfd < 0)
		return (perror("socket"), 1);
	
	if (verbose)
		printf("ping : sock4.fd: %d (socktype SOCK_RAW)\n\n", sockfd);

	foundTarget = target_finder(argc, argv);
	if (foundTarget == -1)
		return (printf("ping: usage error: Destination address required\n"), close(sockfd), 1);

	begin =	time(NULL);
	gettimeofday(&start, NULL);

	target_name = strdup(argv[foundTarget]);

	// // Resolve hostname to IP address
	// host = gethostbyname(target_name);
	// if (host == NULL)
	// 	return (printf("ft_ping: cannot resolve %s: Unknown host\n", argv[foundTarget]), close(sockfd), 1);

	// // Fill in address structure
	// ip = strdup(inet_ntoa(*(struct in_addr *)host->h_addr));
	// addr.sin_family = host->h_addrtype;
	// addr.sin_port = htons(0);
	// addr.sin_addr.s_addr = *(long *)host->h_addr;
	
	//FIX_GETHOSTBYNAME_SHIT

		struct addrinfo hints, *res;
		int status;
		
		// Prepare hints
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_INET;  // Keep it IPv4 (change to AF_UNSPEC for IPv6 support)
		hints.ai_socktype = SOCK_RAW;
		hints.ai_flags = AI_CANONNAME;
		
		// Resolve hostname
		status = getaddrinfo(target_name, NULL, &hints, &res);
		if (status != 0) {
			return (printf("ft_ping: %s: %s\n", target_name, gai_strerror(status)), free(target_name), close(sockfd),  1);
		}
		
		// Extract first result (like gethostbyname would return)
		struct sockaddr_in *ipv4 = (struct sockaddr_in *)res->ai_addr;
		
		// Convert IP to string
		ip = strdup(inet_ntoa(ipv4->sin_addr));
		
		// Fill in address structure
		addr.sin_family = res->ai_family;
		addr.sin_port = htons(0);
		addr.sin_addr = ipv4->sin_addr;
		
		// Cleanup
		freeaddrinfo(res);

	//

	// Getnameinfo from addr to get the true server name through reverse proxy
	reverse_hostname = reverse_dns_lookup(ip);
	if (reverse_hostname == NULL)
		flag_hostname = 1;


	// Allows to set the ttl of the packet
	setsockopt_ret = setsockopt(sockfd, SOL_IP, IP_TTL, &ttl, sizeof(ttl));
	if (setsockopt_ret < 0)
		return (perror("setsockopt"), -1);
	// Setting timout for packets
	memset(&tv_out, 0, sizeof(tv_out)); // initialize memory of tv_out
	tv_out.tv_sec = 1; // 1s timout
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
