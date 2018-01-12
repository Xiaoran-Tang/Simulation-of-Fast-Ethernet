include ../Make.defines

PROGS =	csp sp

all:	${PROGS}

sp:	sp.o
		${CC} ${CFLAGS} -o $@ sp.o ${LIBS}

csp:	csp.o
		${CC} ${CFLAGS} -o $@ csp.o ${LIBS}

clean:
		rm -f ${PROGS} ${CLEANFILES}
