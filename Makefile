################################################################################
##                                                                            ##
##                          Makefile for the MSP430                           ##
##                                                                            ##
################################################################################
#                                                                              #
#                                   USAGE                                      #
#                                                                              #
# 'make' buids assembly (.s), and executable (.elf)                            # 
# 'make flash' installs the executable on the msp430                           #
# 'make clean' deletes files created by make                                   #
# 'make reset' resets the msp430                                               #
#                                                                              #
#  edit CFLAGS to change compiler settings                                     #
#  default no optimization, for optimization append -O2                        #
#                                                                              #
        CFLAGS  = -Wall -g -fomit-frame-pointer -mmcu=msp430g2553 
#                                                                              #
#                                                                              #
#  edit to specify which file to compile, do not include .c extension          #
#  'SRC = *' should work anywhere if there is only one .c file in directory    #
#  'SRC = prog0' if source file prog0.c                                        #
#                                                                              #
        SRC   = main
#                                                                              #
#                                                                              #
#   you probably don't need to change anything below this                      #
################################################################################

OBJS  = $(SRC).o
TARGET  = $(SRC).elf

CC		= msp430-gcc
#msp430-elf-gcc
DEVICE  = mspdebug rf2500

all: $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)
#to produce assembly:
#	$(CC) $(CFLAGS) -S -o $(SRC).s $(SRC).c

.PHONY:flash
flash: all
	echo prog $(TARGET) | $(DEVICE)

.PHONY:erase
erase:
	echo erase | $(DEVICE)

.PHONY: reset
reset:
	echo reset | $(DEVICE)

#this is an implicit rule that says how to compile a .c to get a .o
#$< is the name of the first prereq (the .c file)
%.o: %.c
	$(CC) $(CFLAGS) -c $<


clean:
	rm -fr $(SRC).elf $(SRC).s $(OBJS)
