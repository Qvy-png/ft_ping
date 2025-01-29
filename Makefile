# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: rdel-agu <rdel-agu@student.42.fr>          +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2021/11/25 09:36:05 by rdel-agu          #+#    #+#              #
#    Updated: 2022/05/16 22:20:44 by rdel-agu         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME = ft_ping

CC = cc

CFLAGS = -Wall -Werror -Wextra

RM = rm

OBJ = $(SRC:.c=.o)

SRC =	srcs/ft_ping.c\
		srcs/utils.c
		
INCL =	includes/ft_ping.h

all: $(NAME)

bonus: $(NAME)_bonus

%.o: %.c 
	@$(CC) $(CFLAGS) -c $< -o $@
		
$(NAME): $(OBJ)
	@echo " \033[0;31mCompiling $(NAME)...\033[0m"
	@$(CC) $(CFLAGS) $(OBJ) -o $(NAME)
	@echo " \033[0;32mSuccess\033[0m"

clean:
	@$(RM) -f $(OBJ)
	@echo " \033[0;32mCleaning done!\033[0m"
	
fclean: clean
	@$(RM) -f $(NAME) 
	@$(RM) -f $(NAME)_bonus

re: fclean all

	
.PHONY: clean fclean
