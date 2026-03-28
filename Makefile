CC = cc
NAME = minishell
CFLAGS = -Wall -Wextra -Werror -I./includes/ -g3
LDFLAGS = -lreadline
OBJ_DIR = obj
SRC =	./src/main.c \
		./src/error.c \
		./src/dummy_check.c

OBJ = $(addprefix $(OBJ_DIR)/, $(SRC:.c=.o))
DIRS = $(sort $(dir $(OBJ)))

MAKE = make -C

all: $(NAME)

$(NAME): $(OBJ)
	@echo "$(BLUE)Compiling $(NAME)...$(RESET)"
	@$(CC) $(CFLAGS) $(OBJ) -o $(NAME) $(LDFLAGS)
	@echo "$(GREEN)Compilation successful!$(RESET)"


$(OBJ_DIR)/%.o: %.c | $(DIRS)
	@echo "$(YELLOW)Compiling $<...$(RESET)"
	@$(CC) $(CFLAGS) -c $< -o $@

$(DIRS):
	@mkdir -p $@

clean:
	@echo "$(RED)Cleaning object files...$(RESET)"
	@rm -rf $(OBJ_DIR)

fclean: clean
	@echo "$(RED)Cleaning executable $(NAME)...$(RESET)"
	@rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re $(DIRS) rc 
rc:
	@make re --no-print-directory && make clean --no-print-directory


RESET  = \033[0m
RED    = \033[31m
GREEN  = \033[32m
YELLOW = \033[33m
BLUE   = \033[34m
BOLD   = \033[1m
