OMPFLAGS = -fopenmp
OMPLIBS = -lgomp

CC = gcc
CPPFLAGS = -g -Wall $(OMPFLAGS)
LDFLAGS = -g -Wall -lm
LDLIBS = $(OMPLIBS)

MPICC = mpicc
MPICH = $(MPIROOT)
CFLAGS = -I$(MPICH)/include

all: spin_sense tree_barrier omp_barrier tmt_barrier mcs_tree_barrier combined_barrier mpi_barrier omp_mpi_barrier

spin_sense: spin_sense.o 
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)
tree_barrier: tree_barrier.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

omp_barrier: omp_barrier.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)
	
tmt_barrier: tmt_barrier.o
	$(MPICC) -o $@ $(LDFLAGS) $^ $(LDLIBS)
	
mcs_tree_barrier: mcs_tree_barrier.o
	$(MPICC) -o $@ $(LDFLAGS) $^ $(LDLIBS)

mpi_barrier: mpi_barrier.o
	$(MPICC) -o $@ $(LDFLAGS) $^ $(LDLIBS)
	
combined_barrier: combined_barrier.o
	$(MPICC) -o $@ $(LDFLAGS) $^ $(LDLIBS)

omp_mpi_barrier: omp_mpi_barrier.o
	$(MPICC) -o $@ $(LDFLAGS) $^ $(LDLIBS)
	
clean:
	rm -f *.o spin_sense tree_barrier omp_barrier tmt_barrier mcs_tree_barrier combined_barrier mpi_barrier omp_mpi_barrier
