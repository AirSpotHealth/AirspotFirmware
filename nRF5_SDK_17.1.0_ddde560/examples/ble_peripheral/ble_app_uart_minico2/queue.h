#ifndef __QUEUE_H__
#define __QUEUE_H__

#include <stddef.h>
#include <stdint.h>

// 环形队列,存储 data_size 对象
// head==tail时无数据，tail下标总是无数据，所以内存池长度为 ((queue_size + 1) * data_size)

typedef struct // 最多64K个数据
{
    uint8_t *pool;       // 内存池
    uint16_t queue_size; // 队列长度
    uint16_t head;       // 队首索引
    uint16_t tail;       // 队尾索引
    uint8_t  data_size;  // 数据长度
} queue_t;

// 定义队列 名称，数据大小，队列长度
#define QUEUE_DEF(_name, _data_size, _queue_size)                                          \
    static uint8_t _name##_pool[((_queue_size) + 1) * (_data_size)] = {0};                 \
    queue_t        _name                                            = {                    \
                                                          .pool       = _name##_pool,      \
                                                          .queue_size = (_queue_size) + 1, \
                                                          .head       = 0,                 \
                                                          .tail       = 0,                 \
                                                          .data_size  = (_data_size),      \
    };

/**获取缓存的数据个数 */
uint16_t queue_get_count(queue_t *queue);

/**获取缓存剩余空间 */
uint16_t queue_get_free(queue_t *queue);

/** 数据入队，队列满时直接返回0, 入队成功返回1*/
uint8_t queue_in(queue_t *queue, uint8_t *in_data);

/** 数据出队，队列为空返回0，出队成功返回1 */
uint8_t queue_out(queue_t *queue, uint8_t *out_data);

#endif
