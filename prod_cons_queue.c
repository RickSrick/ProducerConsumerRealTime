#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

/**
 * 
 * @author Riccardo Modolo (212370)
 * @date 23/01/2025
 * 
 * @brief:
 *  Producer-(single)  consumer program  with   dynamic message  rate
 *  adjustment. The consumer shall consume messages  at a given rate,
 *  that  is, with  a   given delay simulating the   consumed message
 *  usage.  An  actor (task or   process) separate from  producer and
 *  consumer shall periodically check the message queue length and if
 *  the  length is below  a given  threshold,  it  will  increase the
 *  production rate. Otherwise (i.e.  the message length is above the
 *  given   threshold),  it  will    decrease  the   production rate.
 * 
 * @implements: 
 *  i  use  standard message queue  in linux with  its api  as shared 
 *  memory. This solution work also with  actor as separated process.
 * 
 **/


/*man msgsnd for further detail*/
/*data structure for sending message*/
struct msgbuf {
    long mtype;     /* message type, must be > 0 */
    int item;       /* message data */
};


#define PRODCONS_TYPE 1
#define MAX_PRODUCING_TIME  15000   //ms
#define MIN_PRODUCING_TIME  500     //ms
static int queue_id;


/* Rates express in ms */
unsigned int producing_time = 2500;                //time to produce message (global variable for accesing)
unsigned const int digestion_time = 5000;                //time tp digest message
unsigned const int checkqueue_time = 1000;               //time to check queue


unsigned const int min_trigger = 2;
unsigned const int max_trigger = 10;
unsigned const int delta_increase = 500;
unsigned const int delta_decrease = 500;



/**
 * Waiting for an amount of time in milliseconds
 * @param {ms} number of millisecond to wait
 */
static void wait_ms(unsigned int ms) {

    static struct timespec time_struct;
    time_struct.tv_sec = (ms / 1000);
    time_struct.tv_nsec = (ms % 1000) * 1000000;    //1 ms = 1'000'000 ns
    nanosleep(&time_struct, NULL);

}


/**
 * This function consume message from a message queue forever
 * @param {arg} object (NEVER USED)
 */
static void* consumer(void *arg) {

    struct msgbuf msg;
    int item;
    int msg_size = 0;
    while (1) {
        wait_ms(digestion_time);
        msg_size = msgrcv(queue_id, &msg, sizeof(int), PRODCONS_TYPE, 0);
        if(msg_size == -1) {
            perror("error msgrcv");
            exit(0);
        }
        item = msg.item;
        printf("consumed item:%d\n", item);
    }
}


/**
 * This function produce message from a message queue forever
 * @param {arg} object (NEVER USED)
 */
static void* producer(void *arg) {
    
    int item = 0;
    struct msgbuf msg;
    msg.mtype = PRODCONS_TYPE;
    while (1) {
        wait_ms(producing_time);  //emulate production time
        msg.item = item;
        if(msgsnd(queue_id, &msg, sizeof(int), 0) == -1) {
            perror("error in production of message");
            exit(1);
        }
        printf("producer send the message\n");
        item++;
    }
    
}


/**
 * This function check size of a message queue forever
 * @param {arg} object (NEVER USED)
 */
static void* actor(void *arg) {

    struct msqid_ds info;
    int curr_size = 0; 
    while (1)
    {
        wait_ms(checkqueue_time);   //Emulate time for production
        if(msgctl(queue_id, IPC_STAT, &info) == -1){
            perror("error reading queue information");
            exit(1);
        }
        printf("actor read size of the queue: %ld\n", info.msg_qnum);

        curr_size = info.msg_qnum;

        if((curr_size <= min_trigger) &&
          ((producing_time - delta_decrease) > 0) && 
          ((producing_time - delta_decrease) >= MIN_PRODUCING_TIME)) {
                producing_time -= delta_decrease;
                printf("under_production, adjust rate to:%d ms\n", producing_time);
        }
        else if((curr_size >= max_trigger) && ((producing_time + delta_decrease) <= MAX_PRODUCING_TIME)) {
            producing_time += delta_increase;
            printf("under_production, adjust rate to:%d ms\n", producing_time);
        }
    }
}


/********************************************/
int main(int argc, char* args[]) {
    
    /* Initialize message queue */
    queue_id = msgget(IPC_PRIVATE, 0666);
    if(queue_id == -1) {
        perror("msgget");
        exit(0);
    }
    printf("Queue id:%d\n", queue_id);


    /* threads creation */
    pthread_t threads[3];
    pthread_create(&threads[0], NULL, producer, NULL);
    pthread_create(&threads[1], NULL, consumer, NULL);
    pthread_create(&threads[2], NULL, actor, NULL);

    for(int t = 0; t < 3; t++)
        pthread_join(threads[t], NULL);

    return 0;
}