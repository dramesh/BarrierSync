#include <omp.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#define DEBUG 1
int num_threads=0,num_barriers=0;

int main(int argc, char **argv) {
  // Serial code
  int a=0;
  int i,j;
  
  if(argc!=3) {
	printf("Usage: omp_barrier <Number-of-OpenMP-threads> <Num-of-Barriers>\n");
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
  
  if(DEBUG == 1)	printf("This is the serial section\n");
  omp_set_num_threads(num_threads);

  #pragma omp parallel shared(a) private(i) 
  {
    // Now we're in the parallel section
    int thread_num = omp_get_thread_num();
	
	for (i=0; i<num_barriers; i++) {
		#pragma omp critical
		{
		a++;
		}
			
		if(DEBUG == 1)	printf("1. a=%d in thread %d before first barrier.\n",a,thread_num);
		
		start[i][thread_num] = omp_get_wtime();
		#pragma omp barrier
		end[i][thread_num] = omp_get_wtime();
		
		if(DEBUG == 1)	printf("%d. a=%d in thread %d after first barrier.\n",i+1,a,thread_num);
	}
  }
	
  for(i=0;i<num_barriers;i++) {
	  for(j=0;j<num_threads;j++) {
		if(start[i][j]>maxstart[i])	maxstart[i]=start[i][j];
		if(end[i][j]>maxend[i])	maxend[i]=end[i][j];
	  }
	  barrier_time[i]=maxend[i]-maxstart[i];
	  if(DEBUG == 1)	printf("Barrier Time of barrier #%d = %lf\n",i+1,barrier_time[i]);
  }
  for(i=0;i<num_barriers;i++) {
	avg_time += barrier_time[i];
  }
  avg_time = avg_time/num_barriers;
  printf("Barrier_Time=%lf\n",avg_time);
  
  // Resume serial code
  if(DEBUG == 1)	printf("Back in the serial section again\n");
  return 0;
}

