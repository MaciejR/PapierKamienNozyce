CC = g++
SRC_DIR_KLIENT = kody/klient
OBJ_DIR_KLIENT = kody/klient/obj/Debug
SRC_DIR_SERWER = kody/serwer
OBJ_DIR_SERWER = kody/serwer/obj/Debug

CFLAGS = -Wall -g -o -std=c++11
PACKAGE = `pkg-config --cflags --libs gtk+-3.0`
LIBS = `pkg-config --libs gtk+-3.0` -lm -lX11 -lpthread

EXEKLIENT = klient
EXESERWER = serwer

SRCS_KLIENT = $(SRC_DIR_KLIENT)/main.cpp
OBJS_KLIENT = $(OBJ_DIR_KLIENT)/main.o 

SRCS_SERWER = $(SRC_DIR_SERWER)/main.cpp
OBJS_SERWER = $(OBJ_DIR_SERWER)/main.o 

main : buildKlient buildSerwer      

buildKlient: $(OBJS_KLIENT)
	$(CC) $(CFLAGS) $(OBJS_KLIENT) -o $(EXEKLIENT) $(LIBS)
	
buildSerwer: $(OBJS_SERWER)
	$(CC) $(CFLAGS) $(OBJS_SERWER) -o $(EXESERWER) $(LIBS)
	
$(OBJ_DIR)/%.o: $(SRC_DIR_KLIENT)/%.c
	$(CC) $(CFLAGS) ***-c***  $< -o $@ $(PACKAGE)
$(OBJ_DIR)/%.o: $(SRC_DIR_SERWER)/%.c
	$(CC) $(CFLAGS) ***-c***  $< -o $@ $(PACKAGE)
