#include <stdio.h>
#include <pthread.h>

struct consumer_args {
    int* my_count;
    int* consumer_print;
    pthread_mutex_t* my_mutex;
    pthread_cond_t* my_cond_1;
    pthread_cond_t* my_cond_2;
};

void* consumer_thread(void* args) {
    struct consumer_args* my_args = (struct consumer_args*) args;  
    int prev_count = 0;  

    while (1) {
        pthread_mutex_lock(my_args->my_mutex);
        printf("CONSUMER: MUTEX LOCKED\n");
        while (*(my_args->consumer_print) == 0) {
            printf("CONSUMER: WAITING ON CONDITION VARIABLE 1\n");
            pthread_cond_wait(my_args->my_cond_1, my_args->my_mutex);
        }

        //printf("CONSUMER: MUTEX UNLOCKED\n");
        //pthread_mutex_unlock(my_args->my_mutex);           

        //pthread_mutex_lock(my_args->my_mutex);
        //printf("CONSUMER: MUTEX LOCKED\n");
        
        printf("Count updated: %d -> %d\n", prev_count, *(my_args->my_count));
        *(my_args->consumer_print) = 0;
        
        printf("CONSUMER: MUTEX UNLOCKED\n");
        pthread_mutex_unlock(my_args->my_mutex);        
        printf("CONSUMER: SIGNALING CONDITION VARIABLE 2\n");
        pthread_cond_signal(my_args->my_cond_2);

        if (*(my_args->my_count) >= 10) {
            break;
        }

        prev_count = *(my_args->my_count);
        
    }
    return NULL;
}

int main() {
    printf("PROGRAM START\n");
    int my_count = 0;
    int consumer_print = 0;
    pthread_mutex_t my_mutex;
    pthread_mutex_init(&my_mutex, NULL);
    pthread_cond_t my_cond_1;
    pthread_cond_init(&my_cond_1, NULL);
    pthread_cond_t my_cond_2;
    pthread_cond_init(&my_cond_2, NULL);

    struct consumer_args consumer_args;
    consumer_args.my_count = &my_count;
    consumer_args.consumer_print = &consumer_print;
    consumer_args.my_mutex = &my_mutex;
    consumer_args.my_cond_1 = &my_cond_1;
    consumer_args.my_cond_2 = &my_cond_2;

    pthread_t consumer;
    pthread_create(&consumer, NULL, consumer_thread, &consumer_args);
    printf("CONSUMER THREAD CREATED\n");

    int i;
    for (i = 0; i < 10; i++) {
        pthread_mutex_lock(&my_mutex);
        printf("PRODUCER: MUTEX LOCKED\n");
        my_count++;
        consumer_print = 1;
        printf("PRODUCER: MUTEX UNLOCKED\n");
        pthread_mutex_unlock(&my_mutex);        
        printf("PRODUCER: SIGNALING CONDITION VARIABLE 1\n");
        pthread_cond_signal(&my_cond_1);

        pthread_mutex_lock(&my_mutex);
        printf("PRODUCER: MUTEX LOCKED\n");
        while (consumer_print != 0) {
            printf("PRODUCER: WAITING ON CONDITION VARIABLE 2\n");
            pthread_cond_wait(&my_cond_2, &my_mutex);
        }
        printf("PRODUCER: MUTEX UNLOCKED\n");
        pthread_mutex_unlock(&my_mutex);        
    }

    pthread_join(consumer, NULL);
    printf("PROGRAM END\n");
    return 0;
}
