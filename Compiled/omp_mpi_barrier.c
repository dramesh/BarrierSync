#include <omp.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include<mpi.h>
#include<unistd.h>
#include<math.h>
#define DEBUG 1

int num_threads;
int num_barriers;
char hname[100];

int main(int argc, char **argv) {
  // Serial code
  int a=0;
  int my_id, num_processes;
  int i,j,k;
  
  const int MSG_TAG = 1000;
  MPI_Status stat;
  
  if(argc!=3) {
	printf("Usage: mpi_barrier <no-of-OpenMP-threads> <Num-of-Barriers>\n");
	exit(-1);
  }
  num_threads=atoi(argv[1]);
  num_barriers=atoi(argv[2]);
  if (num_barriers > 1000) num_barriers = 1000;
  double start[num_barriers][num_threads],end[num_barriers][num_threads],barrier_time[num_barriers],maxstart[num_barriers],maxend[num_barriers], avg_time = 0.0f;
  
  for(i=0; i<num_barriers; i++) {
	maxstart[i]=0.0f;
	maxend[i]=0.0f;
	barrier_time[i]=0.0f;
  }
  
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &num_processes);
  MPI_Comm_rank(MPI_COMM_WORLD, &my_id);
  double start_mpi[num_processes][num_barriers],end_mpi[num_processes][num_barriers];
  
  gethostname(hname,100);
  
  if(DEBUG == 1)	printf("This is the serial section\n");
  omp_set_num_threads(num_threads);
  
  #pragma omp parallel shared(a,start_mpi,start,end_mpi,end) private(i)
  {
    // Now we're in the parallel section
    int thread_num = omp_get_thread_num();
	
	for (i=0; i<num_barriers; i++) {
	
		#pragma omp critical
		{
		a++;
		}
		
		if(DEBUG == 1)	printf("a=%d in thread %d before barrier # %d.\n",a,thread_num,i+1);
		start[i][thread_num] = omp_get_wtime();	
		
		if(thread_num==0)
			MPI_Barrier(MPI_COMM_WORLD);
		#pragma omp barrier
		
		end[i][thread_num] = omp_get_wtime();
		if(DEBUG == 1)	printf("a=%d in thread %d after barrier # %d.\n",a,thread_num,i+1);
	}	
  }

  for(i=0;i<num_barriers;i++) {
	  for(j=0;j<num_threads;j++) {
		if(start[i][j]>maxstart[i])	maxstart[i]=start[i][j];
		if(end[i][j]>maxend[i])	maxend[i]=end[i][j];
	  }
	  barrier_time[i]=maxend[i]-maxstart[i];
	  if(DEBUG == 1)	printf("BT=%lf\n",barrier_time[i]);
  }
  MPI_Send (maxstart, num_barriers, MPI_DOUBLE, 0, MSG_TAG, MPI_COMM_WORLD);
  MPI_Send (maxend, num_barriers, MPI_DOUBLE, 0, MSG_TAG, MPI_COMM_WORLD);
  if(my_id==0) {
	for(i=0;i<num_processes;i++) {
		MPI_Recv (start_mpi[i], num_barriers, MPI_DOUBLE, i, MSG_TAG, MPI_COMM_WORLD, &stat);
		MPI_Recv (end_mpi[i], num_barriers, MPI_DOUBLE, i, MSG_TAG, MPI_COMM_WORLD, &stat);
	}
	for(i=0; i<num_barriers; i++) {
		maxstart[i]=0.0f;
		maxend[i]=0.0f;
		barrier_time[i]=0.0f;
    }
	for(i=0;i<num_barriers;i++) {
		for(k=0;k<num_processes;k++) {
			if(start_mpi[k][i]>maxstart[i])	maxstart[i]=start_mpi[k][i];
			if(end_mpi[k][i]>maxend[i])	maxend[i]=end_mpi[k][i];
		}
		barrier_time[i]=maxend[i]-maxstart[i];
		avg_time += barrier_time[i];
	    if(DEBUG == 1)	printf("BT=%lf\n",barrier_time[i]);
	}
	avg_time = avg_time/num_barriers;
	printf("Barrier_Time=%lf\n",avg_time);
  }
  
  // Resume serial code
  if(DEBUG == 1)	printf("Back in the serial section again\n");
  MPI_Finalize();
  return 0;
}

