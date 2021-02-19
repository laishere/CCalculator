#if !defined(QUEUE_H)
#define QUEUE_H

/**
 * —≠ª∑∂”¡–
 */ 
typedef struct Queue
{
    void **base, **front, **rear;
    int capacity, increment, size;
} Queue;

void queue_init(Queue *queue, int initCapacity, int increment);
void queue_add(Queue *queue, void *item);
void *queue_pull(Queue *queue);
void queue_clear(Queue *queue);

#endif // QUEUE_H
