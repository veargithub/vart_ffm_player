//
// Created by vartc on 2023/12/14.
//

#include <unistd.h>
#include <__threading_support>
#include "myqueue.h"
#include <jni.h>
#include <android/log.h>
#include "mymutex.h"

#define LOGE(FORMAT,...) __android_log_print(ANDROID_LOG_ERROR, "player", FORMAT, ##__VA_ARGS__);

//线程锁
pthread_mutex_t mutex_id;
//条件变量
pthread_cond_t produce_condition_id, consume_condition_id;

Queue queue;

#define PRODUCE_COUNT 10

int consume_number = 0;

void* produce(void* arg) {
    char* name = (char *) arg;
    for (int i = 0; i < PRODUCE_COUNT; ++i) {
        pthread_mutex_lock(&mutex_id);
        while (queue_is_full(&queue)) {
            pthread_cond_wait(&produce_condition_id, &mutex_id);
        }
        LOGE("%s produce element : %d", name, i);

        queue_in(&queue, (NodeElement)i);

        pthread_cond_signal(&consume_condition_id);
        pthread_mutex_unlock(&mutex_id);
        sleep(1);
    }
    LOGE("%s produce finish", name);
    return NULL;
}

void* consume(void* arg) {
    char * name = (char*) arg;
    while (1) {
        pthread_mutex_lock(&mutex_id);
        while (queue_is_empty(&queue)) {
            if (consume_number == PRODUCE_COUNT) {
                break;
            }
            pthread_cond_wait(&consume_condition_id, &mutex_id);
        }
        if (consume_number == PRODUCE_COUNT) {
            pthread_cond_signal(&consume_condition_id);
            pthread_mutex_unlock(&mutex_id);
            break;
        }

        NodeElement  element = queue_out(&queue);
        consume_number += 1;
        LOGE("%s consume element : %d", name, element);
        pthread_cond_signal(&produce_condition_id);
        pthread_mutex_unlock(&mutex_id);
        sleep(1);
    }
    LOGE("%s consume finish", name);
    return  NULL;
}