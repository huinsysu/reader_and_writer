#include<stdio.h>
#include<stdio_ext.h>
#include<stdlib.h>
#include<unistd.h>
#include<time.h>
#include<semaphore.h>
#include<pthread.h>


typedef int data_item;
#define DATA_SIZE 10
data_item share_data[DATA_SIZE];

struct params {
    int tid;
    int wait_time;
    int last_time;
};

sem_t mutex1, mutex2, mutex3, wri, rea;
int read_count = 0;
int write_count = 0;

void toread(data_item* item) {
    int index = rand()%DATA_SIZE;
    *item = share_data[index];
}

void towrite(data_item item) {
    int index = rand()%DATA_SIZE;
    share_data[index] = item;
}

void *reader(void *data) {
    data_item read_item;
    struct params *current = (struct params*)data;

    printf("Thread %d waits %d seconds to request a read operation.\n",
                                    current->tid, current->wait_time);
    sleep(current->wait_time);

    sem_wait(&mutex2);
    if (write_count != 0)
       sem_wait(&rea);
    sem_post(&mutex2);

    sem_wait(&mutex1);
    read_count++;
    if (read_count == 1)
        sem_wait(&wri);
    sem_post(&mutex1);

    printf("Thread %d read operation last %d seconds.\n",
                            current->tid, current->last_time);
    
    sleep(current->last_time);

    toread(&read_item);
    printf("Thread %d get the data %d from buffer.\n", current->tid, read_item);
    
    sem_wait(&mutex1);
    read_count--;
    if (read_count == 0)
        sem_post(&wri);
    sem_post(&mutex1);

    free(current);
    pthread_exit(0);
}

void *writer(void *data) {
    data_item write_item;
    struct params *current = (struct params*)data;

    printf("Thread %d wait %d seconds to request a write operation.\n",
                                current->tid, current->wait_time);
    sleep(current->wait_time);

    sem_wait(&mutex3);
    write_count++;
    if (write_count == 1)
        sem_init(&rea, 0, 0);
    sem_post(&mutex3);

    sem_wait(&wri);
    printf("Thread %d write operation last %d seconds.\n",
                                current->tid, current->last_time);
    sleep(current->last_time);

    write_item = rand()%100;
    towrite(write_item);
    printf("Thread %d Write %d to buffer.\n", current->tid, write_item);

    sem_post(&wri);

    sem_wait(&mutex3);
    write_count--;
    if (write_count == 0) {
        sem_post(&rea);
    }
     sem_post(&mutex3);
    
    free(current);
    pthread_exit(0);
}

int main() {
    
    char mode[100];
    int wtime[100], ltime[100];
    int i;

    FILE *fp;
    char buf[255];
    int thread_num = 0;

    fp = fopen("./text", "r");
    while (fgets(buf, 255, (FILE*)fp)) {

        int flag = 0;
        int index = 0;
        char tem[100];
        for (int i = 1; buf[i-1] != '\n'; i++) {
            if (buf[i] == ' '|| buf[i] == '\n') {
                tem[i-index] = '\0';
                if (flag == 1)
                    mode[thread_num] = tem[i-index-1];
                else if (flag == 2) {
                    wtime[thread_num] = atoi(tem);
                } else if (flag == 3) {
                    ltime[thread_num] = atoi(tem);
                }
                index = i+1;
                flag++;
            } else {
                tem[i-index] = buf[i];
            }
        }
        thread_num++;
    }
    fclose(fp);

    for (i = 0; i < DATA_SIZE; i++)
        share_data[i] = -1;

    pthread_t id[thread_num];
    pthread_attr_t attr;

    pthread_attr_init(&attr);

    srand(time(NULL));
    sem_init(&mutex1, 0, 1);
    sem_init(&mutex2, 0, 1);
    sem_init(&mutex3, 0, 1);
    sem_init(&wri, 0, 1);
    sem_init(&rea, 0, 0);

    for (i = 0; i < thread_num; i++) {
        struct params *parameter = (struct params*)malloc(sizeof(struct params));
        parameter->tid = i+1;
        parameter->wait_time = wtime[i];
        parameter->last_time = ltime[i];

        if (mode[i] == 'R')
            pthread_create(&id[i], &attr, reader, (void*)parameter);
        else
            pthread_create(&id[i], &attr, writer, (void*)parameter);
    }

    for (i = 0; i < thread_num; i++)
        pthread_join(id[i], NULL);

    for (i = 0; i < DATA_SIZE; i++)
        printf("%d ", share_data[i]);
    printf("\n");
    return 0;
}
