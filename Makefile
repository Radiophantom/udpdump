CFLAGS=-Wall -I$(INC_DIR)
SRC_DIR=src
BUILD_DIR=build
INC_DIR=inc/

TARGET=udpdump

SRC=main.c parse.c accum_stat.c
OBJ=$(SRC:.c=.o)

all:
	make -C . clean
	make -C . build
	make -C . $(TARGET)

build:
	mkdir -p $(BUILD_DIR)

$(TARGET): $(BUILD_DIR)/$(OBJ)
	gcc $(OBJ) -o $(TARGET) -lrt -pthread

$(BUILD_DIR)/%.o : $(SRC_DIR)/%.c
	gcc -c $< -o $@ $(CFLAGS)

clean:
	rm -rf $(BUILD_DIR)/*

run:
	./$(TARGET) -dstip 192.168.1.1 -srcip 192.168.3.1 -dstport 55000 -srcport 56000 -if lo

.PHONY: run, build, all
