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
