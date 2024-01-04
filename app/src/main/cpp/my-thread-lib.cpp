//
// Created by vartc on 2023/12/12.
//
#include <jni.h>
#include <string>
#include <android/log.h>
#include <unistd.h>
#include "mymutex.h"
#include "myqueue.h"

#define LOGE(FORMAT,...) __android_log_print(ANDROID_LOG_ERROR, "player", FORMAT, ##__VA_ARGS__);

void* run(void* arg) {
    char *name = (char*) arg;
    for (int i = 0; i < 10; i++) {
        LOGE("Test C Thread : name = %s, i = %d", name, i);
        sleep(1);
    }
    return NULL;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_myndkapplication2_MyCThread_testCThread(JNIEnv *env, jobject clazz) {
    pthread_t tid1, tid2;
    pthread_create(&tid1, NULL, run, (void*) "Thread1");
    pthread_create(&tid2, NULL, run, (void*) "Thread2");
}



extern "C"
JNIEXPORT void JNICALL
Java_com_example_myndkapplication2_MyCThread_testCThread2(JNIEnv *env, jobject thiz) {
    Queue queue;
    pthread_mutex_t mutex_id;
    pthread_cond_t produce_condition_id, consume_condition_id;
    pthread_t tid1, tid2, tid3;

    queue_init(&queue);


    pthread_mutex_init(&mutex_id, nullptr);
    pthread_cond_init(&produce_condition_id, nullptr);
    pthread_cond_init(&consume_condition_id, NULL);
    LOGE("init --- ");

    pthread_create(&tid1, NULL, produce, (void*) "producer1");
    pthread_create(&tid2, NULL, consume, (void*) "consumer1");
    pthread_create(&tid3, NULL, consume, (void*) "consumer2");

    // 阻塞线程
    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);
    pthread_join(tid3, NULL);
    // 销毁条件变量
    pthread_cond_destroy(&produce_condition_id);
    pthread_cond_destroy(&consume_condition_id);
    // 销毁线程锁
    pthread_mutex_destroy(&mutex_id);
    // 销毁队列
    queue_destroy(&queue);
    LOGE("destroy --- ");
}