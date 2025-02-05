#ifndef FT_PING_H
# define FT_PING_H
# define PACKET_SIZE	64

# include <netinet/ip_icmp.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <sys/types.h>
# include <arpa/inet.h>
# include <sys/time.h>
# include <string.h>
# include <stdlib.h>
# include <unistd.h>
# include <signal.h>
# include <stdio.h>
# include <netdb.h>
# include <time.h>

struct ping_pkt {
    struct icmphdr hdr;
    char msg[PACKET_SIZE - sizeof(struct icmphdr)];
};

typedef struct s_mean{

	double value;
	struct s_mean *next;
}   t_mean;

// utils.c
int				is_num(char *str);
int				is_float(char *str);
void			list_push(t_mean **head, double value);
void			free_list(t_mean *head);
void			print_usage(void);

// icmp.c
unsigned short	checksum(void *b, int len);
int				target_finder(int argc, char **argv);
char			*reverse_dns_lookup(char *ip_addr);

#endif