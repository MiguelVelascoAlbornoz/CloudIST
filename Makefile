CC = gcc

CFLAGS = -g -std=c17 -D_POSIX_C_SOURCE=200809L \
        -Wall -Werror -Wextra \
        -Wcast-align -Wconversion -Wfloat-equal -Wformat=2 -Wnull-dereference -Wshadow -Wsign-conversion -Wswitch-enum -Wundef -Wunreachable-code -Wunused \
        -fsanitize=undefined -pthread

ifneq ($(shell uname -s),Darwin) # if not MacOS
    CFLAGS += -fmax-errors=5
endif

SRC_DIR = src
OBJS = $(SRC_DIR)/parser.o $(SRC_DIR)/datacenter.o $(SRC_DIR)/datacenter_utils.o \
       $(SRC_DIR)/resources.o $(SRC_DIR)/filesystem.o $(SRC_DIR)/utils/vector.o
HEADERS = $(wildcard $(SRC_DIR)/*.h $(SRC_DIR)/utils/*.h)

all: cloudIST

cloudIST: $(SRC_DIR)/main.c $(OBJS) $(HEADERS)
	$(CC) $(CFLAGS) -o cloudIST $(SRC_DIR)/main.c $(OBJS) -lm

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

run: cloudIST
	@./cloudIST

clean:
	rm -f $(SRC_DIR)/*.o $(SRC_DIR)/utils/*.o cloudIST

format:
	@which clang-format >/dev/null 2>&1 || echo "Please install clang-format to run this command"
	clang-format -i $(SRC_DIR)/*.c $(SRC_DIR)/*.h $(SRC_DIR)/utils/*.c $(SRC_DIR)/utils/*.h