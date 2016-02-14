#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include "wrapper_funcs.h"
#include "globals.h"
#include <time.h>
#define max_timer_msg_len 21 //1 byte for type, 4 bytes for seq num, how many for time?

/*One way to implement the timer is as follows. The program wakes up periodically, determines
the time elapsed since the last wake up and decrements the time value in the first node.
If it reduces to 0 or less, the timer has expired for the first node. The timer process must
also receive further requests for timers while it is repeatedly waking up and reducing the
timer value in the first node. The select function call is useful for such purposes.*/

struct time_node{
	struct timeval *delta_time;
	//float delta_time;
  uint32_t seq_num;
	struct time_node *next;
  struct time_node *prev;
}time_node;
/*
The time structures involved are defined in <sys/time.h> and look
       like

           struct timeval {
               long    tv_sec;         // seconds
               long    tv_usec;        // microseconds
           };*/


struct sockaddr_in timer_sockaddr;

struct time_node *head = NULL;
struct time_node *cursor = NULL;
int tcpd_sock;

/** from: http://www.gnu.org/software/libc/manual/html_node/Elapsed-Time.html **/
/* Subtract the ‘struct timeval’ values X and Y,
   storing the result in RESULT.
   Return 1 if the difference is negative, otherwise 0. */
int
timeval_subtract (result, x, y)
     struct timeval *result, *x, *y;
{
  /* Perform the carry for the later subtraction by updating y. */
  if (x->tv_usec < y->tv_usec) {
    int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
    y->tv_usec -= 1000000 * nsec;
    y->tv_sec += nsec;
  }
  if (x->tv_usec - y->tv_usec > 1000000) {
    int nsec = (x->tv_usec - y->tv_usec) / 1000000;
    y->tv_usec += 1000000 * nsec;
    y->tv_sec -= nsec;
  }

  /* Compute the time remaining to wait.
     tv_usec is certainly positive. */
  result->tv_sec = x->tv_sec - y->tv_sec;
  result->tv_usec = x->tv_usec - y->tv_usec;

  /* Return 1 if result is negative. */
  return x->tv_sec < y->tv_sec;
}
/*** Based on the implemenation above but for adding ***/
int
timeval_add (result, x, y)
     struct timeval *result, *x, *y;
{
  /* Perform the carry for the later subtraction by updating y. */
  if (x->tv_usec < y->tv_usec) {
    int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
    y->tv_usec -= 1000000 * nsec;
    y->tv_sec += nsec;
  }
  if (x->tv_usec - y->tv_usec > 1000000) {
    int nsec = (x->tv_usec - y->tv_usec) / 1000000;
    y->tv_usec += 1000000 * nsec;
    y->tv_sec -= nsec;
  }

  /* Compute the time remaining to wait.
     tv_usec is certainly positive. */
  result->tv_sec = x->tv_sec + y->tv_sec;
  result->tv_usec = x->tv_usec + y->tv_usec;

  /* Return 1 if result is negative. */
  return x->tv_sec < y->tv_sec;
}

//add time node to list
int add_to_list(struct timeval *d_time, uint32_t s_num);
//remove time node from list
int remove_from_list(uint32_t s_num);

int print_list();
//update head node in timeing list
void update_timer(struct timeval *elapsed_time);

//recieve info from timer_sockaddr
int recv_from_tcpd();

//let timer_sockaddr know that something timed out
int alert_tcpd();




int main(){


	//for sleeping the program
	struct timeval timer;
	//for determining time at which sleep started
	struct timeval start_time;
	//when sleep ends, the time at which this occurs is stored here
	struct timeval curr_time;
	//for storing the result of curr_time - start_time
	struct timeval result_time;
/*Set up for timer_sockaddr Socket*/

 tcpd_sock = socket(AF_INET,SOCK_DGRAM,0);
 if(tcpd_sock < 0) {
	perror("error: opening datagram socket");
	exit(1);
 }

 timer_sockaddr.sin_family = AF_INET;
 timer_sockaddr.sin_port = htons(TIMERPORT);
 timer_sockaddr.sin_addr.s_addr = INADDR_ANY;

 if(bind(tcpd_sock, (struct sockaddr *)&timer_sockaddr, sizeof(timer_sockaddr)) < 0) {
	perror("getting socket name");
	exit(2);
 }

 printf("Timer waiting on port # %d\n", ntohs(timer_sockaddr.sin_port));


int tv_sub_ret_val;
int select_ret_val;
/*
//for testing only, watch stdin (fd 0) to see when it has input
fd_set rfds;
FD_ZERO(&rfds);
FD_SET(0,&rfds);

timer.tv_sec = 5;
timer.tv_usec = 0;
gettimeofday(&start_time,NULL);
select_ret_val = select(1,&rfds,NULL,NULL,&timer);
gettimeofday(&curr_time,NULL);

if(select_ret_val == -1){
	perror("select()");
}else if(select_ret_val){
	printf("Data is now available.\n");
}else{
	printf("No data in 5 seconds\n");
}
printf("Elasped time in microseconds: %ld ms\n",
	((curr_time.tv_sec - start_time.tv_sec)*1000000L +
		curr_time.tv_usec - start_time.tv_usec));

*/
recv_from_tcpd();

/**** Unit testing section ****/
/*
 struct timeval *inc_dtime;
 inc_dtime = malloc(sizeof(struct timeval));

 struct timeval *inc_dtime1;
 inc_dtime1 = malloc(sizeof(struct timeval));

 struct timeval *inc_dtime2;
 inc_dtime2 = malloc(sizeof(struct timeval));

 struct timeval *inc_dtime3;
 inc_dtime3 = malloc(sizeof(struct timeval));

 struct timeval *inc_dtime4;
 inc_dtime4 = malloc(sizeof(struct timeval));

 inc_dtime -> tv_sec = 30;
 inc_dtime -> tv_usec = 0;
 add_to_list(inc_dtime,1);

 inc_dtime1 -> tv_sec = 34;
 inc_dtime1 -> tv_usec = 0;
 add_to_list(inc_dtime1,2);

 inc_dtime2 -> tv_sec = 35;
 inc_dtime2 -> tv_usec = 0;
 add_to_list(inc_dtime2,3);

 inc_dtime3 -> tv_sec = 36;
 inc_dtime3 -> tv_usec = 0;
 add_to_list(inc_dtime3,4);

 inc_dtime4 -> tv_sec = 38;
 inc_dtime4 -> tv_usec = 0;
 add_to_list(inc_dtime4,5);

 print_list();
*/
//printf("\n\n/**** Remove testing ****/\n\n");
/*
int removed = remove_from_list(3);
printf("Seq_num removed is  : %d\n",removed);
print_list();
removed = remove_from_list(5);
printf("\nSeq_num removed is  : %d\n",removed);
print_list();
removed = remove_from_list(1);
printf("\nSeq_num removed is  : %d\n",removed);
print_list();

struct timeval *test_el_time;
test_el_time = malloc(sizeof(struct timeval));
test_el_time -> tv_sec = 5;
test_el_time -> tv_usec = 0;
update_timer(test_el_time);
printf("5 seconds elapsed\n");
print_list();
//update_timer(26); //should make head be less than zero
//printf("26 seconds elapsed\n");
//print_list();*/
/**** End unit testing section ****/
}


int recv_from_tcpd(){

int timer_sockaddr_len = sizeof(timer_sockaddr);
int buflen = max_timer_msg_len;
char recv_buff[buflen];


         printf("waiting to receive\n");
int res = recvfrom(tcpd_sock, recv_buff, buflen, 0, (struct sockaddr *)&timer_sockaddr, &buflen);
printf("res is %d\n",res);
    	if(res < 0) {

		perror("error receiving in timer");
		exit(4);

        }
        printf( "buff0 is %d\n",(uint8_t)ntohs(recv_buff[0]));
        if(ntohs(recv_buff[0]) == 6){

        	uint32_t seq_num = ntohs(recv_buff[1]);
        	add_to_list(&recv_buff[5],seq_num);

        }else if(ntohs(recv_buff[0]) == 7){

        	uint32_t seq_num = ntohs(recv_buff[1]);
        	remove_from_list(seq_num);

        }
 return 1;
}
int print_list(){
    if(head == NULL){
        printf("head is null\n");
        return -1;
    }
    cursor = head;
    while(cursor != NULL){
        printf("Node seq_num: %d   time: %ld\n",cursor->seq_num,
				(cursor -> delta_time -> tv_sec + cursor -> delta_time -> tv_usec));
        cursor = cursor -> next;
    }
    return 0;
}

int add_to_list(struct timeval *d_time, uint32_t s_num){
    struct time_node *new_t_node_ptr = (struct time_node*)malloc(sizeof(struct time_node));
    struct time_node *prev_p  = head;

	  struct timeval *cur_node_time;
		struct timeval *upd_node_time;

		cur_node_time = malloc(sizeof(struct timeval));
		upd_node_time = malloc(sizeof(struct timeval));

    new_t_node_ptr -> seq_num = s_num;
    cursor = head;

    if(head != NULL){
        printf("here\n");
        if( (d_time -> tv_sec + d_time -> tv_usec) <=
						(head -> delta_time -> tv_sec + head -> delta_time -> tv_usec) )
				{
                printf("here  0\n");
                new_t_node_ptr -> next = head;
                new_t_node_ptr -> prev = NULL;

                //get the updated timval struct using function (x - y)
								timeval_subtract (upd_node_time, head -> delta_time, d_time);
								head -> delta_time = upd_node_time;
                //head -> delta_time = (head -> delta_time) - d_time;
                head -> prev = new_t_node_ptr;

                new_t_node_ptr -> delta_time = d_time;
                head = new_t_node_ptr;
                printf("here  1\n");

        }else{
                printf("here  x\n");

               //d_time = d_time -(cursor -> delta_time);
               timeval_subtract (d_time, d_time, cursor -> delta_time);


                cursor = cursor -> next;
                if(cursor != NULL){

                    while(cursor != NULL && d_time > (cursor -> delta_time)){
                            //d_time = d_time -(cursor -> delta_time);
														timeval_subtract (d_time, d_time, cursor -> delta_time);
                            prev_p = cursor;
                            cursor = cursor -> next;


                    }


                    if(cursor == NULL){
                        prev_p -> next = new_t_node_ptr;
                        new_t_node_ptr -> next = NULL;
                        new_t_node_ptr -> prev = prev_p;
                        new_t_node_ptr -> delta_time = d_time;
												//here we are at the end of the list, no need
												//to update the remaining list (there is nothing remaining)

                        }else{
                            cursor -> prev -> next = new_t_node_ptr;
                            new_t_node_ptr -> prev  = cursor -> prev;
                            cursor -> prev = new_t_node_ptr;
                            new_t_node_ptr -> next = cursor;
                            new_t_node_ptr -> delta_time = d_time;
														//now we need to update the next item
														//since we insterted into the middle of the list
														timeval_subtract (upd_node_time,
														new_t_node_ptr -> next -> delta_time,
														new_t_node_ptr -> delta_time);
														new_t_node_ptr -> next -> delta_time = upd_node_time;

																/*
														new_t_node_ptr -> next -> delta_time =
															new_t_node_ptr -> next -> delta_time -
															new_t_node_ptr -> delta_time;*/
                        }

                }else{
                    head -> next = new_t_node_ptr;
                    new_t_node_ptr -> next = NULL;
                    new_t_node_ptr -> delta_time = d_time;
                    new_t_node_ptr -> prev = head;
										//new node is second in the list and nothing comes after
										//so no need to update the remaining list since there is
										//none
                }



    }
}else{
        printf("yes\n");
        new_t_node_ptr -> delta_time = d_time;
        new_t_node_ptr -> next = NULL;
        new_t_node_ptr -> prev = NULL;
        head = new_t_node_ptr;
        //no need to update times since only one element in list
    }
    return -1;
}



int remove_from_list(uint32_t s_num){
	struct timeval *cur_node_time;
	struct timeval *upd_node_time;

	cur_node_time = malloc(sizeof(struct timeval));
	upd_node_time = malloc(sizeof(struct timeval));


	int removed = -1;
	if(head == NULL){
		printf("Timer List is empty!");
		return -1;
	}

	cursor = head;
	if(head -> seq_num == s_num){
		removed = head -> seq_num;
		cur_node_time = head -> delta_time;
		cursor = head -> next;
		timeval_add(upd_node_time,cur_node_time,cursor -> delta_time);
		cursor -> delta_time = upd_node_time;
		head = cursor;
		head -> prev = NULL;

		return removed;
	}else{
		cursor = cursor -> next;
		while(cursor != NULL  && cursor -> seq_num != s_num){
			cursor = cursor->next;
		}

		if(cursor == NULL){

			//this would seg fault if cursor is NULL
			//removed = cursor ->prev ->seq_num;
			//cursor -> prev -> next = NULL;
		}else{
			removed = cursor -> seq_num;
			cur_node_time = cursor -> delta_time;
			if(cursor -> next != NULL){
				timeval_add(upd_node_time,cursor -> next -> delta_time, cur_node_time);
				cursor -> next -> delta_time = upd_node_time;
				cursor -> next -> prev = cursor ->prev;
			}
			cursor -> prev -> next = cursor -> next;
		}

	}
	return removed;
}

void update_timer(struct timeval *elapsed_time){
		 struct timeval *head_time;
		 struct timeval *upd_head_time;
		 upd_head_time = malloc(sizeof(struct timeval));
		 head_time = malloc(sizeof(struct timeval));

		 head_time = head -> delta_time;
		 int res = timeval_subtract(upd_head_time,head_time,elapsed_time);


		if(res < 0){
			head = head -> next;
			head -> prev = NULL;
		}else{
			head -> delta_time = upd_head_time;
		}
}
