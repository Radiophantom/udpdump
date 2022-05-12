SRC=accum_stat.c
BUILD_SRC=$(addprefix src/, $(SRC))
OBJ=$(addprefix $(BUILD_DIR)/,$(SRC:.c=.o))
BUILD_DIR=build
TARGET=accum_stat

all : build, $(TARGET)

build:
	mkdir -p $(BUILD_DIR)

$(TARGET) : $(OBJ)
	gcc $(OBJ) -o $(TARGET)

$(OBJ) : $(BUILD_SRC)
	gcc -c $(BUILD_SRC) -o $(OBJ)

run:
	./$(TARGET) -dstip 192.168.1.1 -srcip 192.168.3.1 -dstport 55000 -srcport 56000

.PHONY: run, build, all
