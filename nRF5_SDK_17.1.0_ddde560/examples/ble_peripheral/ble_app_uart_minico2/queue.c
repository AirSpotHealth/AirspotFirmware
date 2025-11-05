#include "queue.h"
// #include "log.h"

#define __next_idx(queue, idx) ((idx) + 1 == queue->queue_size ? 0 : (idx) + 1)

/**获取缓存的数据个数 */
uint16_t queue_get_count(queue_t *queue)
{
    return queue->tail >= queue->head ? queue->tail - queue->head : queue->queue_size - queue->head + queue->tail;
}

/**获取缓存剩余空间 */
uint16_t queue_get_free(queue_t *queue)
{
    return queue->head <= queue->tail ? queue->head + queue->queue_size - (queue->tail + 1) : queue->head - (queue->tail + 1);
}

/** 数据入队，队列满时直接返回0, 入队成功返回1*/
uint8_t queue_in(queue_t *queue, uint8_t *in_data)
{
    uint16_t tail_next = __next_idx(queue, queue->tail);
    if (tail_next == queue->head) return 0;

    uint8_t *ptr = queue->pool + queue->data_size * queue->tail;
    for (uint8_t i = 0; i < queue->data_size; i++)
    {
        *(ptr + i) = *(in_data + i);
    }
    queue->tail = tail_next;
    // print("queue tail %d\n", tail_next);
    return 1;
}

/** 数据出队，队列为空返回0，出队成功返回1 */
uint8_t queue_out(queue_t *queue, uint8_t *out_data)
{
    if (queue->head != queue->tail)
    {
        uint8_t *ptr = queue->pool + queue->data_size * queue->head;
        for (uint8_t i = 0; i < queue->data_size; i++)
        {
            *(out_data + i) = *(ptr + i);
            *(ptr + i)      = 0;
        }
        // print("queue head %d data %02X\n", queue->head, *out_data);
        queue->head = __next_idx(queue, queue->head);
        return 1;
    }
    return 0;
}
