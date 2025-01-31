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

void	print_list(t_mean **head, long long unsigned int size)
{
	t_mean					*tmp = *head;
	long long unsigned int	i = 1;

	while (i <= size)
	{
		printf("%g\n", tmp->value);
		tmp = tmp->next;
		i++;
	}
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