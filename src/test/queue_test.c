#include "queue.h"
#include "test.h"
#include <assert.h>

void queue_test()
{
    Queue queue, *q = &queue;
    TEST_START("ѭ�����г�ʼ������");
    queue_init(q, 2, 2);
    assert(queue.capacity == 2);
    assert(queue.increment == 2);
    assert(queue.size == 0);
    assert(queue.front == queue.rear);
    assert(queue.front == queue.base);
    TEST_END();

    int data[] = {1, 2, 3, 4};

    TEST_START("ѭ��������Ӳ���");
    queue_add(q, data);
    queue_add(q, data + 1);
    assert(queue.capacity == 2);
    assert(queue.size == 2);
    queue_add(q, data + 2);
    queue_add(q, data + 3);
    assert(queue.rear == queue.base + queue.capacity); // rear����ĩ��
    assert(queue.capacity == 4); // ������
    assert(queue.size == 4);
    TEST_END();

    TEST_START("ѭ�����г��Ӳ���");
    for (int i = 0; i < 4; i++)
        assert(queue_pull(q) == data + i);
    assert(queue.front == queue.base + queue.capacity); // front����ĩ��
    assert(NULL == queue_pull(q)); // û�ж�����
    TEST_END();

    TEST_START("ѭ��������Ӻ���ղ���");
    for (int i = 0; i < 4; i++)
        queue_add(q, data + i);
    assert(queue_pull(q) == data);
    assert(queue.capacity == 4);
    assert(queue.size == 3);
    queue_clear(q);
    assert(queue.capacity == 0);
    assert(queue.size == 0);
    assert(queue.base == NULL);
    assert(queue.front == NULL);
    assert(queue.rear == NULL);
    TEST_END();
}