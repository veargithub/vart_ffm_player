//
// Created by vartc on 2023/12/13.
//

#ifndef MYNDKAPPLICATION2_MYQUEUE_H
#define MYNDKAPPLICATION2_MYQUEUE_H


#include <sys/types.h>

#define QUEUE_MAX_SIZE 50

// 节点数据类型
typedef uint NodeElement;

// 节点
typedef struct _Node {
    // 数据
    NodeElement data;
    // 下一个
    struct _Node* next;
} Node;

// 队列
typedef struct _Queue {
    // 大小
    int size;
    // 队列头
    Node* head;
    // 队列尾
    Node* tail;
} Queue;

/**
 * 初始化队列
 * @param queue
 */
void queue_init(Queue* queue);

/**
 * 销毁队列
 * @param queue
 */
void queue_destroy(Queue* queue);

/**
 * 判断是否为空
 * @param queue
 * @return
 */
bool queue_is_empty(Queue* queue);

/**
 * 判断是否已满
 * @param queue
 * @return
 */
bool queue_is_full(Queue* queue);

/**
 * 入队
 * @param queue
 * @param element
 * @param tid
 * @param cid
 */
void queue_in(Queue* queue, NodeElement element);

/**
 * 出队 (阻塞)
 * @param queue
 * @param tid
 * @param cid
 * @return
 */
NodeElement queue_out(Queue* queue);

#endif

