#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <termios.h>

#include <sched.h> // sched_setaffinity
/**
 * 
 * @author Riccardo Modolo (212370)
 * @date 05/02/2025
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
 *  using  a  local  array  managed  with  mutex  and  condition  for 
 *  multithreading context.  
 * 
 **/


#define BUFFER_SIZE 128
#define MAX_PRODUCING_TIME  15000
#define MIN_PRODUCING_TIME  500

pthread_mutex_t mutex;
pthread_cond_t can_produce, can_digest;

unsigned const int min_trigger = 2;
unsigned const int max_trigger = 10;
unsigned const int delta_increase = 500;
unsigned const int delta_decrease = 500;

/* Shared data */
int buffer[BUFFER_SIZE];
int read_id = 0;
int write_id = 0;
int num_elem = 0;                                   //number of messages inside buff

unsigned int producing_time = 2500;                 //time to produce message (global variable for accesing)
unsigned int digestion_time = 5000;                 //time tp digest message (not for global accessing but for easy edit)
unsigned int checkqueue_time = 1000;                //time to check queue (non for global accessing but for easy edit)

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

int set_realtime_attribute(pthread_attr_t *attr, int priority, cpu_set_t *cpuset) {

    int status;
    struct sched_param param;
    pthread_attr_init(attr);


    status = pthread_attr_getschedparam(attr, &param);
    if(status) {
        perror("pthread_attr_getschedparam");
        return status;
    }

    param.sched_priority = priority;
    status = pthread_attr_setschedparam(attr, &param);
    if(status) {
        perror("pthread_attr_setschedparam");
        return status;
    }

    if(cpuset != NULL) {
        status = pthread_attr_setaffinity_np(attr, sizeof(cpu_set_t), cpuset);
        if(status) {
            perror("pthread_attr_setaffinity_np");
            return status;
        }
    }
    
    return status;
}


/**
 * Implementation of the consumer routine
 * @param {arg} NEVER USED
 */
static void* consumer(void* arg) {

    int item;
    printf("Consumer in CPU %d\n",sched_getcpu());
    while (1) {

        pthread_mutex_lock(&mutex);
        while(read_id == write_id) {
            pthread_cond_wait(&can_digest, &mutex);
        }
        
        item = buffer[read_id];
        read_id = (read_id + 1)%BUFFER_SIZE;
        num_elem--;
        printf("[digest]: %d\n", item);

        pthread_cond_signal(&can_produce);
        pthread_mutex_unlock(&mutex);

        wait_ms(digestion_time);    // Emulate consumption time 
    }
    
}


/**
 * Implementation of the producer routine
 * @param {arg} NEVER USED
 */
static void* producer(void* arg) {

    int item = 0;
    printf("Producer in CPU %d\n",sched_getcpu());
    while (1) {

        wait_ms(producing_time);    // Emulate producing time

        pthread_mutex_lock(&mutex);
        while((write_id + 1)%BUFFER_SIZE == read_id) {
            pthread_cond_wait(&can_produce, &mutex);
        }
    
        buffer[write_id] = item;
        write_id = (write_id + 1)%BUFFER_SIZE;
        num_elem++;
        printf("[produce]: %d\n", item);
        pthread_cond_signal(&can_digest);
        pthread_mutex_unlock(&mutex);
        item++;
    }
    
}

/**
 * Implementation of the actor routine
 * @param {arg} NEVER USED
 */
static void* actor(void* arg) {

    printf("Actor in CPU %d\n",sched_getcpu());
    while (1) {
        wait_ms(checkqueue_time);
        printf("[size]: %d\n", num_elem);

        if((num_elem <= min_trigger) &&
           ((producing_time - delta_decrease) > 0) && 
           ((producing_time - delta_decrease) >= MIN_PRODUCING_TIME)) {
            producing_time -= delta_decrease;
            printf("under_production, adjust rate to:%d ms\n", producing_time);
        }
        else if((num_elem >= max_trigger) && 
                ((producing_time + delta_decrease) <= MAX_PRODUCING_TIME)) {
            producing_time += delta_increase;
            printf("under_production, adjust rate to:%d ms\n", producing_time);
        }
    }
    
}


void interrupt_handling(int signum) {
    printf(": code interrupted\n");
    exit(0);
}


/**
 * function to change via input the digestion rate
 * @param {arg} NEVER USED
 */
static void* input_handling(void* arg) {
    char c; 
    static struct termios oldtio, newtio;
    tcgetattr(0, &oldtio);
    newtio = oldtio;
    newtio.c_lflag &= ~ICANON;
    newtio.c_lflag &= ~ECHO;
    tcsetattr(0, TCSANOW, &newtio);

    while (1) {
        fflush(stdout);
        read(0, &c, 1);
        if (c == 'm') {
            digestion_time+= delta_increase;
            printf("Increase DIGESTION RATE: %d\n", digestion_time);
        }
        if( c == 'n') {
            digestion_time-= delta_decrease;
            printf("Decrease DIGESTION RATE: %d\n", digestion_time);
        }
    }
}

/********************************************/
int main(int argc, char* args[]) {
    
    signal(SIGINT, interrupt_handling);

    long number_of_processors = sysconf(_SC_NPROCESSORS_ONLN);
    printf("number of processors: %ld\n", number_of_processors);


    // cpu_set_t: This data set is a bitset where each bit represents a CPU.
    cpu_set_t producer_set, consumer_set, actor_set, input_set;

    // CPU_ZERO: This macro initializes the CPU set set to be the empty set.
    CPU_ZERO(&producer_set);
    CPU_ZERO(&consumer_set);
    CPU_ZERO(&actor_set);
    CPU_ZERO(&input_set);
    CPU_SET(0, &producer_set);
    CPU_SET(1, &consumer_set);
    CPU_SET(2, &actor_set);
    CPU_SET(3, &input_set);


    pthread_attr_t producer_attr, consumer_attr, actor_attr, input_attr;
    set_realtime_attribute(&producer_attr, 0, &producer_set);   
    set_realtime_attribute(&consumer_attr, 0, &consumer_set);   
    set_realtime_attribute(&actor_attr,    0, &actor_set);         
    set_realtime_attribute(&input_attr,    0, &input_set);         

    /* Initialize mutex and condition variables */
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&can_produce, NULL);
    pthread_cond_init(&can_digest, NULL);

    /* thread creation */
    pthread_t threads[4];
    pthread_create(&threads[0], &producer_attr, producer, NULL);
    pthread_create(&threads[1], &consumer_attr, consumer, NULL);
    pthread_create(&threads[2], &actor_attr, actor, NULL);
    pthread_create(&threads[3], &input_attr, input_handling, NULL);

    for(size_t t = 0; t < 4; t++)
        pthread_join(threads[t], NULL);

    return 0;
}