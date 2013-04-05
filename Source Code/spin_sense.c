#include <omp.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#define DEBUG 1
int num_threads=0,num_barriers=0;
int count;
int sense=1;

void sense_barrier(int *local_sense,int thread_num) {
	int flag = 0;
	*local_sense=(*local_sense==0)?1:0;

	#pragma omp atomic
	count-=1;
	if(count==0) {
		#pragma omp critical 
		{
		count=num_threads;
		sense=*local_sense;
		}
	} 
	else {
		while(1){
			#pragma omp critical(sense)
			{
				flag = (sense==*local_sense)?1:0;
			}
			if(flag==1)	break;
		}
	}
}


int main(int argc, char **argv) {
  // Serial code
  int a=0;
  int local_sense=1;
  int i,j;
  
  if(argc!=3) {
	printf("Usage: spin_sense <Number-of-OpenMP-threads> <Num-of-Barriers>\n");
	exit(-1);
  }

  num_threads=atoi( argv[1] );
  num_barriers=atoi(argv[2]);
  if (num_barriers > 1000) num_barriers = 1000;
  count=num_threads;
  double start[num_barriers][num_threads],end[num_barriers][num_threads],barrier_time[num_barriers],maxstart[num_barriers],maxend[num_barriers], avg_time = 0.0f;
  
  for(i=0; i<num_barriers; i++) {
	maxstart[i]=0.0f;
	maxend[i]=0.0f;
	barrier_time[i]=0.0f;
  }
  
  if(DEBUG == 1)	printf("This is the serial section\n");
  omp_set_num_threads(num_threads);

  #pragma omp parallel shared(a) private(i) firstprivate(local_sense)
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
		sense_barrier(&local_sense,thread_num);
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

