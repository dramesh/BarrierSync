#include <stdio.h>
#include <stdlib.h>
#include<mpi.h>
#include<unistd.h>
#include<math.h>
#define DEBUG 1
int num_processes;
int num_rounds;
int num_barriers;
typedef enum {	winner=0,loser=1, champion=2, dropout=3,bye=4} roles;
roles *rounds;

void init_tree(int id) {
	int k;
	rounds=(roles*)malloc((num_rounds+1)*sizeof(roles));
	for(k=0;k<=num_rounds;k++){
		if(k==0) 
			rounds[k]=dropout;
		else if(k>0){
			if(id==0 && pow(2,k)>=num_processes){
				rounds[k]=champion;
				continue;
			}
			if(id%((int)pow(2,k))==pow(2,k-1)){
				rounds[k]=loser;
				continue;
			}
			if(id%((int)pow(2,k))==0){
				if(id+pow(2,k-1) >= num_processes)
					rounds[k]=bye;
				else if(id+pow(2,k-1)  < num_processes && pow(2,k) < num_processes)
					rounds[k]=winner;
			} /*end if id%(pow(2,k))==0 */
			else
				rounds[k]=bye;
		}	
	}
}

void tournament_barrier(int id) {
	const int MSG_TAG = 1000;
	int round,len=1,dest,src;
	int *msgbuf;
	MPI_Status stat;
	msgbuf = (int *)malloc (sizeof (int));
	
	for(round=0;round<=num_rounds;round++) {
		switch(rounds[round]){
			case loser:
				dest=id-pow(2,round-1);
				MPI_Send (msgbuf, len, MPI_INT, dest, MSG_TAG, MPI_COMM_WORLD);
				MPI_Recv (msgbuf, len, MPI_INT, dest, MSG_TAG, MPI_COMM_WORLD, &stat);
				break;
			case winner:
				src=id+pow(2,round-1);
				MPI_Recv (msgbuf, len, MPI_INT, src, MSG_TAG, MPI_COMM_WORLD, &stat);
				break;
			case bye:
				break;
			case champion:
				dest=id+pow(2,round-1);
				MPI_Recv (msgbuf, len, MPI_INT, dest, MSG_TAG, MPI_COMM_WORLD, &stat);
				MPI_Send (msgbuf, len, MPI_INT, dest, MSG_TAG, MPI_COMM_WORLD);
				break;
			default:
				break;
		}	
	} /*end for*/
	round--;
	for(;round>=0;round--){
		switch(rounds[round]){
			case loser:
				break;
			case winner:
				dest=id+pow(2,round-1);
				MPI_Send (msgbuf, len, MPI_INT, dest, MSG_TAG, MPI_COMM_WORLD);
				break;
			case bye:
				break;
			case champion:
				break;
			case dropout:
				return;
			default:
				break;
		}	
	} /*end for*/
}

int main(int argc, char **argv) {
  int my_id;
  char hname[100];
  int i,k;
  const int MSG_TAG = 1000;
  MPI_Status stat;
  
  if(argc!=2) {
	printf("Usage: tmt_barrier <Num-of-Barriers>\n");
	exit(-1);
  }
  num_barriers=atoi(argv[1]);
  if (num_barriers > 1000) num_barriers = 1000;
  
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &num_processes);
  MPI_Comm_rank(MPI_COMM_WORLD, &my_id);
  double start_time[num_barriers],end_time[num_barriers],start[num_processes][num_barriers],end[num_processes][num_barriers],barrier_time[num_barriers],maxstart[num_barriers],maxend[num_barriers],avg_time=0.0f;
  
  num_rounds=((int)ceil(log2(num_processes)));
  init_tree(my_id);
  gethostname(hname,100);
  
  for(i=0; i<num_barriers; i++) {
	maxstart[i]=0.0f;
	maxend[i]=0.0f;
	barrier_time[i]=0.0f;
  }
  
  for(k=0;k<num_barriers;k++) {
	  if(DEBUG == 1)	printf("%d. Before barrier process %d of %d in node %s\n", k+1, my_id, num_processes,hname);
	  if(my_id==0)	start[0][k]=MPI_Wtime ();
	  else 		  start_time[k] = MPI_Wtime ();
	  
	  tournament_barrier(my_id);
	  
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

