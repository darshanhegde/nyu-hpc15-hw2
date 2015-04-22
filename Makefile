EXECS=ssort.o mpi_solved*.o
MPICC=mpicc

all: ${EXECS}

ssort.o: ssort.c
	${MPICC} -o ssort.o ssort.c

mpi_solved*.o: mpi_solved*.c
	${MPICC} -o mpi_solved1.o mpi_solved1.c
	${MPICC} -o mpi_solved2.o mpi_solved2.c
	${MPICC} -o mpi_solved3.o mpi_solved3.c
	${MPICC} -o mpi_solved4.o mpi_solved4.c
	${MPICC} -o mpi_solved5.o mpi_solved5.c
	${MPICC} -o mpi_solved6.o mpi_solved6.c
	${MPICC} -o mpi_solved7.o mpi_solved7.c

clean:
	rm -f ${EXECS}
