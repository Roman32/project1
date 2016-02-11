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
#define max_timer_msg_len 24 //1 byte for type, 4 bytes for seq num, how many for time?

/*One way to implement the timer is as follows. The program wakes up periodically, determines
the time elapsed since the last wake up and decrements the time value in the first node.
If it reduces to 0 or less, the timer has expired for the first node. The timer process must
also receive further requests for timers while it is repeatedly waking up and reducing the
timer value in the first node. The select function call is useful for such purposes.*/

struct time_node{
	float delta_time;
	int seq_num;
	struct time_node *next;
    struct time_node *prev;
}time_node;

struct sockaddr_in timer_sockaddr;

struct time_node *head = NULL;
struct time_node *cursor = NULL;
int tcpd_sock;


//add time node to list
int add_to_list(float d_time, int s_num);
//remove time node from list
void remove_from_list(int s_num);

int print_list();
//iterate through list checking times???????
void check_time_outs();

//recieve info from timer_sockaddr
int recv_from_tcpd();

//let timer_sockaddr know that something timed out
int alert_tcpd();


int main(){
/*Set up for timer_sockaddr Socket*/
    /*
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
*/
 add_to_list(5.00,1);
 add_to_list(12.00,2);
 add_to_list(14.00,3);
 add_to_list(3.00,4);
 add_to_list(19.00,5);
 print_list();
}


int recv_from_tcpd(){

	int timer_sockaddr_len = sizeof(timer_sockaddr);
    int buflen = max_timer_msg_len;
    char recv_buff[buflen];

    while(1){

    	if(recvfrom(tcpd_sock, recv_buff, buflen, 0, (struct sockaddr *)&timer_sockaddr, &buflen) < 0) {

			perror("error receiving in timer");
			exit(4);

        }
        if(ntohs(recv_buff[0]) == 6){

        	int seq_num = ntohs(recv_buff[5]);
        	add_to_list(recv_buff[5],seq_num);

        }else if(ntohs(recv_buff[0]) == 7){

        	int seq_num = ntohs(recv_buff[5]);
        	//remove_from_list(seq_num);

        }

    }

}
int print_list(){
    if(head == NULL){
        printf("head is null\n");
        return -1;
    }
    cursor = head;
    while(cursor != NULL){
        printf("Node seq_num: %d   time: %f\n",cursor->seq_num,cursor -> delta_time);
        cursor = cursor -> next;
    }
    return 0;
}
int add_to_list(float d_time, int s_num){
    struct time_node *new_t_node_ptr = (struct time_node*)malloc(sizeof(struct time_node));
    struct time_node *prev_p  = head;
    new_t_node_ptr -> seq_num = s_num;
    cursor = head;
    if(head != NULL){
        printf("here\n");
        if(d_time <= head -> delta_time) {
                printf("here  0\n");
                new_t_node_ptr -> next = head;
                new_t_node_ptr -> prev = NULL;

                head -> delta_time = (head -> delta_time) - d_time;
                head -> prev = new_t_node_ptr;

                new_t_node_ptr -> delta_time = d_time;
                head = new_t_node_ptr;
                printf("here  1\n");

        }else{
                printf("here  x\n");

              d_time = d_time -(cursor -> delta_time);



                cursor = cursor -> next;
                if(cursor != NULL){

                    while(cursor != NULL && d_time > (cursor -> delta_time)){
                            d_time = d_time -(cursor -> delta_time);
                            prev_p = cursor;
                            cursor = cursor -> next;


                    }


                    if(cursor == NULL){
                        prev_p -> next = new_t_node_ptr;
                        new_t_node_ptr -> next = NULL;
                        new_t_node_ptr -> prev = prev_p;
                        new_t_node_ptr -> delta_time = d_time;

                        }else{
                            cursor -> prev -> next = new_t_node_ptr;
                            new_t_node_ptr -> prev  = cursor -> prev;
                            cursor -> prev = new_t_node_ptr;
                            new_t_node_ptr -> next = cursor;
                            new_t_node_ptr -> delta_time = d_time;
                        }

                }else{
                    head -> next = new_t_node_ptr;
                    new_t_node_ptr -> next = NULL;
                    new_t_node_ptr -> delta_time = d_time;
                    new_t_node_ptr -> prev = head;
                }



    }
}else{
        printf("yes\n");
        new_t_node_ptr -> delta_time = d_time;
        new_t_node_ptr -> next = NULL;
        new_t_node_ptr -> prev = NULL;
        head = new_t_node_ptr;
        printf("the damn node seq_num is %d\n",head -> seq_num);
    }
    return -1;
}
