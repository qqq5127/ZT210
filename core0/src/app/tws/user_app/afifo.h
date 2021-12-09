#ifndef __AFIFO_H__
#define __AFIFO_H__

/**
 * @brief lock free fifo for application
 */

/**
 * @brief init the fifo
 *
 * @param name fifo name
 * @param type unit type
 * @param size fifo size, must be 2^n
 */
#define AFIFO_INIT(name, type, size)               \
    static const uint32_t name##_FIFO_SIZE = size; \
    static type name##_fifo[size] = {0};           \
    static uint32_t name##_fifo_in = 0;            \
    static uint32_t name##_fifo_out = 0;

/**
 * @brief check if the fifo is empty
 *
 * @param name fifo name
 *
 * @return true if fifo is empty, false if not
 */
#define AFIFO_IS_EMPTY(name) (name##_fifo_in == name##_fifo_out)

/**
 * @brief check if the fifo is full
 *
 * @param name fifo name
 *
 * @return true if fifo is empty, false if not
 */
#define AFIFO_IS_FULL(name) (((name##_fifo_in + 1) & (name##_FIFO_SIZE - 1)) == name##_fifo_out)

/**
 * @brief clear the fifo
 *
 * @param name fifo name
 */
#define AFIFO_RESET(name) (name##_fifo_out = name##_fifo_in)

/**
 * @brief put data into fifo, please check AFIFO_IS_FULL before call this macro
 *
 * @param name fifo name
 * @param x the data to put int fifo
 *
 */
#define AFIFO_IN(name, x)                                               \
    ({                                                                  \
        name##_fifo[name##_fifo_in] = x;                                \
        name##_fifo_in = (name##_fifo_in + 1) & (name##_FIFO_SIZE - 1); \
    })

/**
 * @brief take out data from fifo, please check AFIFO_IS_EMPTY before call this macro
 *
 * @param name fifo name
 * @param type data type
 *
 */
#define AFIFO_OUT(name, type)                                             \
    ({                                                                    \
        type val = name##_fifo[name##_fifo_out];                          \
        name##_fifo_out = (name##_fifo_out + 1) & (name##_FIFO_SIZE - 1); \
        val;                                                              \
    })

#endif   //__AFIFO_H__
