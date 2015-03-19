include config.mk

SRC = mjson.c tickerd.c
OBJ = ${SRC:.c=.o}

all: tickerd

.c.o:
	${CC} -c -o $@ ${CFLAGS} $<

${OBJ}: config.mk

tickerd: ${OBJ}
	${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	rm -f tickerd ${OBJ}

install: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f tickerd ${DESTDIR}${PREFIX}/bin/

.PHONY: all clean install
