#include <omp.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#define DEBUG 1
int num_threads=0,num_barriers=0;
struct node {
	int count;
	int locksense;
	struct node* parent;
};

void construct_tree(struct node *nodes) {
	int i,j,k,l,m,p,q=0;
	for(i=0;i<num_threads-1;i++) {
		nodes[i].count=2;
		nodes[i].locksense=0;
	}
	l=(int)(ceil(log2(num_threads)));
	j=0;
	i=num_threads;
	p=(int)(i/2);
	for(k=1;k<=l;k++) {
		i=(int)(i/2);
		for(m=0;m<i;m++,j++) {
			if(q==2) {p++;q=0;}
			q++;
			nodes[j].parent=&nodes[p];
		}
	}
	nodes[num_threads-2].parent=NULL;
}

void tree_barrier_aux(int sense,struct node *thisnode, int thread_num,int i) {
	int flag=0;
	#pragma omp critical
		thisnode->count-=1;
	if(thisnode->count==0) {
		if(thisnode->parent!=NULL)
			tree_barrier_aux(sense,thisnode->parent,thread_num,i);
		#pragma omp critical(thisnode)
		{
			if(DEBUG == 1)	printf("%d resetting count...",thread_num);
			thisnode->count=2;
			thisnode->locksense=(thisnode->locksense==0)?1:0;
		}
		#pragma omp taskyield
	} 
	while(1) {
		#pragma omp critical(sense)
		{
			flag = (sense==thisnode->locksense)?1:0;
		}
		if(flag==1)	{ if(DEBUG == 1)	printf("\nReleasing %d...",thread_num); break; }
		#pragma omp taskyield
		#pragma omp flush 
	}
}

void tree_barrier(int *sense,int thread_num,struct node *mynode,int i) {
	int s=*sense;
	tree_barrier_aux(s,mynode,thread_num,i);
	#pragma omp critical
	{
		s=*sense;
		if(s==0)	*sense=1;
		else		*sense=0;
	}
}

int main(int argc, char **argv) {
  // Serial code
  int a=0;
  int i,j;
   
  if(argc!=3) {
	printf("Usage: tree_barrier <Number-of-OpenMP-threads> <Num-of-Barriers>\n");
	exit(-1);
  }
  num_threads=atoi( argv[1] );
  num_barriers=atoi(argv[2]);
  if (num_barriers > 1000) num_barriers = 1000;
  struct node nodes[num_threads-1];
  double start[num_barriers][num_threads],end[num_barriers][num_threads],barrier_time[num_barriers],maxstart[num_barriers],maxend[num_barriers], avg_time = 0.0f;
  
  for(i=0; i<num_barriers; i++) {
	maxstart[i]=0.0f;
	maxend[i]=0.0f;
	barrier_time[i]=0.0f;
  }
  
  if(DEBUG == 1)	printf("This is the serial section\n");
  omp_set_num_threads(num_threads);
  
  construct_tree(nodes);
  struct node mynode;
  mynode.count=2;
  mynode.locksense=0;
	
  #pragma omp parallel shared(a,nodes) private(i,j) firstprivate(mynode)
  {
    // Now we're in the parallel section
    int thread_num = omp_get_thread_num();
	int sense=1;
	mynode.parent=&nodes[(int)(thread_num/2)];
	for (i=0; i<num_barriers; i++) {
		#pragma omp critical
		{
			a++;
		}
		
		if(DEBUG == 1)	printf("a=%d in thread %d before barrier # %d.\n",a,thread_num,i+1);
		
		start[i][thread_num] = omp_get_wtime();
		tree_barrier(&sense,thread_num,&nodes[(int)(thread_num/2)],i);
		end[i][thread_num] = omp_get_wtime();
				
		if(DEBUG == 1)	printf("a=%d in thread %d after barrier # %d.\n",a,thread_num,i+1);
		
		#pragma omp taskyield
		#pragma omp flush
		usleep(500);
		if(DEBUG == 1)	printf("\nPrinting from %d=",thread_num);
		for(j=0;j<num_threads-1;j++) {
			nodes[j].locksense=(sense==0)?1:0;
			if(DEBUG == 1)	printf("%d,%d ",nodes[j].locksense,nodes[j].count);
		}
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

