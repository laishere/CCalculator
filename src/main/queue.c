#include "queue.h"
#include <stdlib.h>

void queue_init(Queue *queue, int initCapacity, int increment)
{
    if (!queue) return;
    queue->capacity = initCapacity;
    queue->front = queue->rear = queue->base = malloc(sizeof(void *) * queue->capacity);
    queue->increment = increment;
    queue->size = 0;
}

void queue_add(Queue *queue, void *item)
{
    if (!queue || !queue->base) return;
    if (queue->size == queue->capacity)
    {
        // 需要扩容
        int front = queue->front - queue->base;
        int oldCapacity = queue->capacity;
        queue->capacity += queue->increment;
        queue->base = realloc(queue->base, sizeof(void *) * queue->capacity);
        queue->front = queue->base + front;
        // 满的时候 front = rear, 把 0 直到 front 的内容接到新分配空间的第一个位置
        void **newPos = queue->base + oldCapacity;
        for (void **p = queue->base; p < queue->front; p++, newPos++)
        {
            if (newPos == queue->base + queue->capacity)
                newPos = queue->base;
            *newPos = *p;
        }
        queue->rear = newPos;
    }
    if (queue->rear == queue->base + queue->capacity)
        queue->rear = queue->base;
    *queue->rear = item;
    queue->rear++;
    queue->size++;
}

void *queue_pull(Queue *queue)
{
    if (queue->size == 0)
        return NULL;
    if (queue->front == queue->base + queue->capacity)
        queue->front = queue->base;
    queue->size--;
    return *(queue->front++);
}

void queue_clear(Queue *queue)
{
    free(queue->base);
    queue->base = queue->front = queue->rear = NULL;
    queue->capacity = queue->increment = queue->size = 0;
}