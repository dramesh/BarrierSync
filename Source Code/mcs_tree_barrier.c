#include <stdio.h>
#include <stdlib.h>
#include<mpi.h>
#include<unistd.h>
#include<math.h>
#define DEBUG 1

int num_barriers;

struct treenode {
	int have_child[4];
	int child_ptrs[2];
} *nodes;

void init_tree(int id, int num_processes) {
	 int i,j;
	 nodes=(struct treenode*)malloc(num_processes*sizeof(struct treenode));
	 for(i=0;i<num_processes;i++) {
		 for (j=0; j<4; j++) {
			if(4*i+(j+1) < num_processes) nodes[i].have_child[j] = 1;
			else nodes[i].have_child[j] = 0;
		 }
		 nodes[i].child_ptrs[0]=(2*i+1>=num_processes)?-1:2*i+1;
		 nodes[i].child_ptrs[1]=(2*i+2>=num_processes)?-1:2*i+2;
	 }
}

void MCS_tree_barrier(int id,int num_processes) {
	
	int i,len=1;
	int *msgbuf;
	const int MSG_TAG = 1000;
	MPI_Status stat;
	msgbuf = (int *)malloc (sizeof (int));
	for(i=0;i<4;i++){
		if(nodes[id].have_child[i]==1)
			MPI_Recv (msgbuf, len, MPI_INT, 4*id+i+1, MSG_TAG, MPI_COMM_WORLD, &stat);
	}
	if(id!=0) {
		MPI_Send (msgbuf, len, MPI_INT, floor((id-1)/4), MSG_TAG, MPI_COMM_WORLD);
		//wakeup
		MPI_Recv (msgbuf, len, MPI_INT, floor((id-1)/2), MSG_TAG, MPI_COMM_WORLD, &stat);
	}
	
	if(nodes[id].child_ptrs[0]!=-1) 
		MPI_Send (msgbuf, len, MPI_INT, nodes[id].child_ptrs[0], MSG_TAG, MPI_COMM_WORLD);
	if(nodes[id].child_ptrs[1]!=-1) 
		MPI_Send (msgbuf, len, MPI_INT, nodes[id].child_ptrs[1], MSG_TAG, MPI_COMM_WORLD);
}

int main(int argc, char **argv)
{
  int my_id, num_processes;
  int i,k;
  
  const int MSG_TAG = 1000;
  MPI_Status stat;
  char hname[100];
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &num_processes);
  MPI_Comm_rank(MPI_COMM_WORLD, &my_id);
  if(argc!=2) {
	printf("Usage: mcs_tree_barrier <Num-of-Barriers>\n");
	exit(-1);
  }
  num_barriers=atoi(argv[1]);
  if (num_barriers > 1000) num_barriers = 1000;
  double start_time[num_barriers],end_time[num_barriers],start[num_processes][num_barriers],end[num_processes][num_barriers],barrier_time[num_barriers],maxstart[num_barriers],maxend[num_barriers],avg_time=0.0f;
    
  for(i=0; i<num_barriers; i++) {
	maxstart[i]=0.0f;
	maxend[i]=0.0f;
	barrier_time[i]=0.0f;
  }
  
  init_tree(my_id,num_processes);
  gethostname(hname,100);
  
  for(k=0;k<num_barriers;k++) {
	  if(DEBUG == 1)	printf("%d. Before barrier process %d of %d in node %s\n", k+1, my_id, num_processes,hname);
	  
	  if(my_id==0)	start[0][k]=MPI_Wtime ();
	  else 		  start_time[k] = MPI_Wtime ();
	  
	  MCS_tree_barrier(my_id,num_processes);
	  
	  if(my_id==0)	end[0][k]=MPI_Wtime ();
	  else 		  end_time[k] = MPI_Wtime ();
	  
	  if(DEBUG == 1)	printf("%d. After barrier from process %d of %d in node %s\n", k+1, my_id, num_processes,hname);
  }
  
  if(my_id!=0) {
	MPI_Send (start_time, num_barriers, MPI_DOUBLE, 0, MSG_TAG, MPI_COMM_WORLD);
	MPI_Send (end_time, num_barriers, MPI_DOUBLE, 0, MSG_TAG, MPI_COMM_WORLD);
  } else {
	for(i=1;i<num_processes;i++) {
		MPI_Recv (start[i], num_barriers, MPI_DOUBLE, i, MSG_TAG, MPI_COMM_WORLD, &stat);
		MPI_Recv (end[i], num_barriers, MPI_DOUBLE, i, MSG_TAG, MPI_COMM_WORLD, &stat);
	}
	for(i=0;i<num_barriers;i++) {
		for(k=0;k<num_processes;k++) {
			if(start[k][i]>maxstart[i])	maxstart[i]=start[k][i];
			if(end[k][i]>maxend[i])	maxend[i]=end[k][i];
		}
		barrier_time[i]=maxend[i]-maxstart[i];
		avg_time += barrier_time[i];
	    if(DEBUG == 1)	printf("Barrier Time of barrier #%d = %lf\n",i+1,barrier_time[i]);
	}
	avg_time = avg_time/num_barriers;
	printf("Barrier_Time=%lf\n",avg_time);
  }
  MPI_Finalize();
  return 0;
}