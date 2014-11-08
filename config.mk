# paths
PREFIX ?= /usr/local

INC ?= -I/usr/local/include
LIB ?= -L/usr/local/lib

# flags
CFLAGS += -Wall -Os ${INC}
LDFLAGS += ${LIB} -lcrypto -ltls -lssl

# compiler and linker
CC ?= cc
