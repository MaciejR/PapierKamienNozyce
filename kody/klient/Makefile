CC = gcc
SRC_DIR = .
INC_DIR = 
OBJ_DIR = obj/Debug
CFLAGS = -c -Wall 
EXE = project
PACKAGE = `pkg-config --cflags --libs gtk+-3.0`
LIBS = `pkg-config --libs gtk+-3.0`
SRCS = $(SRC_DIR)/main.cpp 
OBJS = $(OBJ_DIR)/main.o       

main : build

build: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(EXE) $(LIBS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) ***-c*** -I$(INC_DIR) $< -o $@ $(PACKAGE)