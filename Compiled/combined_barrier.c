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

struct node {
	int count;
	int locksense;
	struct node* parent;
} *nodes;

struct treenode {
	int have_child[4];
	int child_ptrs[2];
} *mcsnodes;

void init_tree(int id,int num_processes) {
	 int i,j;
	 mcsnodes=(struct treenode*)malloc(num_processes*sizeof(struct treenode));
	 for(i=0;i<num_processes;i++) {
		 for (j=0; j<4; j++) {
			if(4*i+(j+1) < num_processes) mcsnodes[i].have_child[j] = 1;
			else mcsnodes[i].have_child[j] = 0;
		 }
		 mcsnodes[i].child_ptrs[0]=(2*i+1>=num_processes)?-1:2*i+1;
		 mcsnodes[i].child_ptrs[1]=(2*i+2>=num_processes)?-1:2*i+2;
	 }
}

void MCS_tree_barrier(int id,int num_processes) {
	
	int i,len=1;
	int *msgbuf;
	const int MSG_TAG = 1000;
	MPI_Status stat;
	msgbuf = (int *)malloc (sizeof (int));
	for(i=0;i<4;i++) {
		if(mcsnodes[id].have_child[i]==1)
			MPI_Recv (msgbuf, len, MPI_INT, 4*id+i+1, MSG_TAG, MPI_COMM_WORLD, &stat);
	}
	if(id!=0) {
		MPI_Send (msgbuf, len, MPI_INT, floor((id-1)/4), MSG_TAG, MPI_COMM_WORLD);
		//wakeup
		MPI_Recv (msgbuf, len, MPI_INT, floor((id-1)/2), MSG_TAG, MPI_COMM_WORLD, &stat);
	}
	
	if(mcsnodes[id].child_ptrs[0]!=-1) 
		MPI_Send (msgbuf, len, MPI_INT, mcsnodes[id].child_ptrs[0], MSG_TAG, MPI_COMM_WORLD);
	if(mcsnodes[id].child_ptrs[1]!=-1) 
		MPI_Send (msgbuf, len, MPI_INT, mcsnodes[id].child_ptrs[1], MSG_TAG, MPI_COMM_WORLD);
}

void construct_tree() {
	int i,j,k,l,m,p,q=0;
	nodes=(struct node*)malloc((num_threads-1)*sizeof(struct node));
	for(i=0;i<num_threads-1;i++) {
		nodes[i].count=2;
		nodes[i].locksense=0;
	}
	l=(int)(ceil(log2(num_threads)));
	j=0;
	i=num_threads;
	p=(int)(i/2);
	for(k=1;k<=l;k++){
		i=(int)(i/2);
		for(m=0;m<i;m++,j++){
			if(q==2) {p++;q=0;}
			q++;
			nodes[j].parent=&nodes[p];
			}
	}
	nodes[num_threads-2].parent=NULL;
}

void tree_barrier_aux(int *sense,struct node *thisnode, int thread_num,int my_id,int num_processes)
{
	int flag=0;
	#pragma omp critical
	thisnode->count-=1;
	
	if(thisnode->count==0) {
		if(thisnode->parent!=NULL) 
			tree_barrier_aux(sense,thisnode->parent,thread_num,my_id,num_processes);
		else {
			//Call MPI barrier
			if(DEBUG == 1)	printf("Before barrier process %d of %d in node %s\n", my_id, num_processes,hname);
			MCS_tree_barrier(my_id,num_processes);
			if(DEBUG == 1)	printf("After barrier from process %d of %d in node %s\n", my_id, num_processes,hname);
		}
		
		#pragma omp critical
		{
			thisnode->count=2;
			thisnode->locksense=(thisnode->locksense==0)?1:0;
		}
	} 
	
	while(1) {
		#pragma omp critical(sense)
		{
			flag = (*sense==thisnode->locksense)?1:0;
		}
		if(flag==1)	break;
	}
}

void tree_barrier(int *sense,int thread_num,int my_id,int num_processes) {
	struct node mynode;
	mynode.count=2;
	mynode.locksense=0;
	mynode.parent=&nodes[(int)(thread_num/2)];
	
	tree_barrier_aux(sense,mynode.parent,thread_num,my_id,num_processes);
	
	#pragma omp critical
	{
		*sense=(*sense==0)?1:0;
	}
}

int main(int argc, char **argv) {
  // Serial code
  int a=0;
  int sense=1;
  int my_id, num_processes;
  int i,j,k;
  
  const int MSG_TAG = 1000;
  MPI_Status stat;
  
  if(argc!=3) {
	printf("Usage: combined_barrier <no-of-OpenMP-threads> <Num-of-Barriers>\n");
	exit(-1);
  }
  num_threads=atoi( argv[1] );
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
  
  init_tree(my_id,num_processes);
  gethostname(hname,100);
  
  if(DEBUG == 1)	printf("This is the serial section\n");
  omp_set_num_threads(num_threads);
  
  construct_tree();
  
  #pragma omp parallel shared(a,start_mpi,start,end_mpi,end) private(i) firstprivate(sense)
  {
    // Now we're in the parallel section
    int thread_num = omp_get_thread_num();
	
	for (i=0; i<num_barriers; i++) {
	
		#pragma omp critical
		{
			a++;
		}
		
		if(DEBUG == 1)	printf("a=%d in thread %d before barrier # %d.\n",a,thread_num,i+1);
		start[i][thread_num] = MPI_Wtime();	
		
		tree_barrier(&sense,thread_num,my_id,num_processes);
		
		end[i][thread_num] = MPI_Wtime();
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
	    if(DEBUG == 1)	printf("i=%d,BT=%lf\n",i,barrier_time[i]);
	}
	avg_time = avg_time/num_barriers;
	printf("Barrier_Time=%lf\n",avg_time);
  }
  
  // Resume serial code
  if(DEBUG == 1)	printf("Back in the serial section again\n");
  MPI_Finalize();
  free(nodes);
  return 0;
}

