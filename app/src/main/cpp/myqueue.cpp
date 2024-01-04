//
// Created by vartc on 2023/12/14.
//

#include "myqueue.h"
#include <stdlib.h>

void queue_init(Queue* queue) {
    queue->size = 0;
    queue->head = NULL;
    queue->tail = NULL;
}

void queue_destroy(Queue* queue) {
    Node* node = queue->head;
    while (node != NULL) {
        queue->head = queue->head->next;
        free(node);
        node = queue->head;
    }
    queue->head = NULL;
    queue->tail = NULL;
}

bool queue_is_empty(Queue* queue) {
    return queue->size == 0;
}

bool queue_is_full(Queue* queue) {
    return queue->size == QUEUE_MAX_SIZE;
}

void queue_in(Queue* queue, NodeElement element) {
    if (queue_is_full(queue)) {
        return;
    }
    Node *newNode = (Node*) malloc(sizeof(Node));
    newNode->data = element;
    newNode->next = NULL;
    if (queue->head == NULL) {
        queue->head = newNode;
        queue->tail = newNode;
    } else {
        queue->tail->next = newNode;
        queue->tail = newNode;
    }
    queue->size += 1;
}

NodeElement queue_out(Queue* queue) {
    if (queue->size == 0 || queue->head == NULL) {
        return NULL;
    }
    Node* node = queue->head;
    NodeElement  element = node->data;
    queue->head = queue->head->next;
    free(node);
    queue->size -= 1;
    return element;
}


