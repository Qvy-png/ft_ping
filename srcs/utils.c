#include "../includes/ft_ping.h"

int		is_num(char *str)
{
	int	i;

	i = 0;
	if (str == NULL)
		return (0);
	while (str[i] != '\0')
	{
		if (str[i] < '0' || str[i] > '9')
			return (0);
		i++;
	}
	return (1);
}

int		is_float(char *str)
{
	int	i;

	i = 0;
	if (str == NULL)
		return (0);
	while (str[i] != '\0')
	{
		if ((str[i] < '0' || str[i] > '9') && str[i] != '.')
			return (0);
		i++;
	}
	return (1);
}

void	list_push(t_mean **head, double value)
{
    t_mean * new_node;

    new_node = (t_mean *) malloc(sizeof(t_mean));
    new_node->value = value;
    new_node->next = *head;
    *head = new_node;
}

void	free_list(t_mean *head)
{
	t_mean	*tmp;

	while (head != NULL)
	{
		tmp = head;
		head = head->next;
		free(tmp);
	}
}

void	print_usage(void)
{
	printf("Usage\n");
	printf("  ./ft_ping [options] <destination>\n\n");
	printf("Options:\n");
	printf("  <destination>      DNS name or IP address\n");
	printf(" MANDATORY\n");
	printf("  -v                 verbose output\n");
	printf("  -?                 print usage\n");
	printf(" BONUS\n");
	printf("  -c <count>         stop after <count> replies\n");
	printf("  -q                 quiet output\n");
	printf("  -n                 no reverse DNS name resolution\n");
	printf("  -t                 change packet's ttl\n");
	printf("  -a                 use audible ping\n");
	
}