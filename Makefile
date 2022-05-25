SRC=accum_stat.c parse.c
SRC_DIR=../src/
BUILD_SRC=$(addprefix $(SRC_DIR), $(SRC))
OBJ=$(SRC:.c=.o)
TARGET=accum_stat
INC_DIR=$(SRC_DIR)
CFLAGS=-Wall -I$(INC_DIR)

all : build, $(TARGET)

build:
	mkdir -p $(BUILD_DIR)

$(TARGET) : $(OBJ)
	echo SRC $(SRC)
	echo SRC_DIR $(SRC_DIR)
	echo BUILD_SRCi $(BUILD_SRC)
	echo OBJ $(OBJ)
	gcc $(OBJ) -o $(TARGET) -lrt

%.o : %.c
	gcc -c $< -o $@ $(CFLAGS)

run:
	./$(TARGET) -dstip 192.168.1.1 -srcip 192.168.3.1 -dstport 55000 -srcport 56000

.PHONY: run, build, all
