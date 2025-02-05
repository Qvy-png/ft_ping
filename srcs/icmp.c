#include "../includes/ft_ping.h"

// Calculate checksum for ICMP packet
unsigned short	checksum(void *b, int len)
{
	unsigned short	*buf = b;
	unsigned int	sum = 0;
	unsigned short	result;

	for (sum = 0; len > 1; len -= 2)
		sum += *buf++;
	if (len == 1)
		sum += *(unsigned char *)buf;
	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += (sum >> 16);
	result = ~sum;
	return (result);
}


// TODO modify to allow for specific IP pings
int target_finder(int argc, char **argv)
{
	int		found_target = -1;
	int		target_count = 0;
	int		i;

	i = 1;
	while (i < argc)
	{
		if (argv[i][0] == '-' || is_float(argv[i]))
			i++;
		else
		{
			target_count++;
			found_target = i;
			i++;
		}
	}
	if (target_count > 1)
		return (-1);
	else
		return (found_target);
}

// Resolve the reverse lookup of the hostname
char *reverse_dns_lookup(char *ip_addr) {
    struct sockaddr_in temp_addr;
    socklen_t len;
    char buf[NI_MAXHOST], *ret_buf;

    temp_addr.sin_family = AF_INET;
    temp_addr.sin_addr.s_addr = inet_addr(ip_addr);
    len = sizeof(struct sockaddr_in);

    if (getnameinfo((struct sockaddr *)&temp_addr, len, buf, sizeof(buf), NULL, 0, NI_NAMEREQD)) {
        printf("Could not resolve reverse lookup of hostname\n");
        return NULL;
    }

    ret_buf = (char *)malloc((strlen(buf) + 1) * sizeof(char));
    strcpy(ret_buf, buf);
    return ret_buf;
}
