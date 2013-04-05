COMPILING & RUNNING:
====================

1. Run make to generate the object files for all the barrier implementations.
$ make

2. The OpenMP barrier implementations can be run by specifying number of threads and the number of barriers for which the threads need to be synchronized as command-line arguments.
$ ./<barrier_name> <number_of_threads> <number_of_barriers>
	eg: $ ./spin_sense 6 100

3. The MPI barrier implementations can be run by specifying number of MPI processors and the number of barriers as command-line arguments.
$ mpirun --hostfile $PBS_NODEFILE -np <number_of_processors> <barrier_name> <number_of_barriers>
	eg: $ mpirun --hostfile $PBS_NODEFILE -np 8 tmt_barrier 50

4. The OpenMPI-MP combination barrier implementations can be run by specifying number of MPI processors, the number of threads per process and the number of barriers as command-line arguments.
$ mpirun --hostfile $PBS_NODEFILE -np <number_of_processors> <barrier_name> <number_of_threads> <number_of_barriers>
	eg: $ mpirun --hostfile $PBS_NODEFILE -np 8 combined_barrier 8 10
