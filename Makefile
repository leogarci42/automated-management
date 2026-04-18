CC = cc
NAME = automated-management
CFLAGS = -Wall -Wextra -I./includes/ -g3 -Wno-unused-command-line-argument
LDFLAGS = -lreadline
OBJ_DIR = obj
SRC =	./src/main.c \
		./src/helpers/error.c \
		./src/dummy_check.c \
		./src/tokenizer.c \
		./src/helpers/free_token.c \
		./src/helpers/print_AST.c \
		./src/helpers/useful.c \
		./src/fd_tracker.c \
                ./src/helpers/codegen.c

OBJ = $(addprefix $(OBJ_DIR)/, $(SRC:.c=.o))
DIRS = $(sort $(dir $(OBJ)))

MAKE = make -C

all: $(NAME)

$(NAME): $(OBJ)
	@printf "$(BLUE)Compiling $(NAME)...$(RESET)\n"
	@$(CC) $(CFLAGS) $(OBJ) -o $(NAME) $(LDFLAGS)
	@printf "$(GREEN)Compilation successful!$(RESET)\n"


$(OBJ_DIR)/%.o: %.c | $(DIRS)
	@printf "$(YELLOW)Compiling $<...$(RESET)\n"
	@$(CC) $(CFLAGS) -c $< -o $@

$(DIRS):
	@mkdir -p $@

clean:
	@printf "$(RED)Cleaning object files...$(RESET)\n"
	@rm -rf $(OBJ_DIR)

fclean: clean
	@printf "$(RED)Cleaning executable $(NAME)...$(RESET)\n"
	@rm -f $(NAME)

re: fclean all

debug: CFLAGS += -g3 -DTRACK_FD -fsanitize=address,undefined -O0 --pedantic-errors -Wpedantic -fno-omit-frame-pointer -DTRACK_FD
debug: re
	

.PHONY: all clean fclean re $(DIRS) rc 
rc:
	@make re --no-print-directory && make clean --no-print-directory


RESET  = \e[0m
RED    = \e[31m
GREEN  = \e[32m
YELLOW = \e[33m
BLUE   = \e[34m
BOLD   = \e[1m
