EXECS=ssort.o
MPICC=mpicc

all: ${EXECS}

ssort.o: ssort.c
	${MPICC} -o ssort.o ssort.c

clean:
	rm -f ${EXECS}
