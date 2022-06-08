CFLAGS=-Wall -I$(INC_DIR)
SRC_DIR=src
BUILD_DIR=build
INC_DIR=inc/

UDPDUMP_SRC=main.c parse.c accum_stat.c
UDPDUMP_OBJ=$(addprefix $(BUILD_DIR)/, $(UDPDUMP_SRC:.c=.o))

all:
	make -C . clean
	make -C . build
	make -C . udpdump
	make -C . get_stat

build:
	mkdir -p $(BUILD_DIR)

udpdump: $(UDPDUMP_OBJ)
	gcc $(UDPDUMP_OBJ) -o $@ -lrt -pthread

get_stat: $(BUILD_DIR)/get_stat.o
	gcc $< -o $@ $(CFLAGS) -lrt

$(BUILD_DIR)/%.o : $(SRC_DIR)/%.c
	gcc -c $< -o $@ $(CFLAGS)

clean:
	rm -rf $(BUILD_DIR)/*

.PHONY: all, build, clean
