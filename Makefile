CC = cc
NAME = automated-management
CFLAGS = -Wall -Wextra -I./includes/ -g3
LDFLAGS = -lreadline
OBJ_DIR = obj
SRC =	./src/main.c \
		./src/error.c \
		./src/dummy_check.c \
		./src/tokenizer.c

OBJ = $(addprefix $(OBJ_DIR)/, $(SRC:.c=.o))
DIRS = $(sort $(dir $(OBJ)))

MAKE = make -C

all: $(NAME)

$(NAME): $(OBJ)
	@echo -e "$(BLUE)Compiling $(NAME)...$(RESET)"
	@$(CC) $(CFLAGS) $(OBJ) -o $(NAME) $(LDFLAGS)
	@echo -e "$(GREEN)Compilation successful!$(RESET)"


$(OBJ_DIR)/%.o: %.c | $(DIRS)
	@echo -e "$(YELLOW)Compiling $<...$(RESET)"
	@$(CC) $(CFLAGS) -c $< -o $@

$(DIRS):
	@mkdir -p $@

clean:
	@echo -e "$(RED)Cleaning object files...$(RESET)"
	@rm -rf $(OBJ_DIR)

fclean: clean
	@echo -e "$(RED)Cleaning executable $(NAME)...$(RESET)"
	@rm -f $(NAME)

re: fclean all

debug: CFLAGS += -g3 -fsanitize=address,undefined -O0 --pedantic-errors -Wpedantic -fno-omit-frame-pointer
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
