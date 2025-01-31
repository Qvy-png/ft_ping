#ifndef FT_PING_H
# define FT_PING_H
# define PACKET_SIZE	64
# define PING_TIMEOUT	2

# include <stdio.h>
# include <stdlib.h>
# include <unistd.h>
# include <signal.h>
// # include <sys/socket.h>
# include <netinet/in.h>
# include <netinet/ip_icmp.h>
# include <arpa/inet.h>
# include <netdb.h>
# include <string.h>
# include <sys/time.h>
# include <time.h>

typedef struct s_mean{

    double value;
    struct s_mean *next;
} t_mean;

int		is_num(char *str);
int		is_float(char *str);
void    list_push(t_mean **head, double value);
void	print_list(t_mean **head, long long unsigned int size);
void	free_list(t_mean *head);

#endif