# Make file for client overseer project 
# Author: Benjamin Nasraoui 
# Student No: n9437207

CC = gcc
CFLAGS = -Wall -pedantic

all: controller overseer
		echo "Done."

controller: controller.o 

overseer: overseer.o 

clean:
	rm -f controller
	rm -f overseer 
	rm -f *.o

.PHONY: all clean