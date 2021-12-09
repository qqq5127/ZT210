/****************************************************************************

Copyright(c) 2020 by WuQi Technologies. ALL RIGHTS RESERVED.

This Information is proprietary to WuQi Technologies and MAY NOT
be copied by any method or incorporated into another program without
the express written consent of WuQi. This Information or any portion
thereof remains the property of WuQi. The Information contained herein
is believed to be accurate and WuQi assumes no responsibility or
liability for its use in any way and conveys no license or title under
any patent or copyright and makes no representation or warranty that this
Information is free from patent or copyright infringement.

****************************************************************************/
/* os shim includes */
#include "types.h"
#include "string.h"
#include "generic_list.h"

#include "iot_flash.h"
#include "key_value_cache.h"
#include "key_value_port.h"

#include "os_mem.h"
#include "os_lock.h"

#include "iot_share_task.h"

/*lint -esym(754, key_value_cached_key::reverved1) */
typedef struct key_value_cached_key {
    struct list_head node;
    uint32_t length;
    uint16_t id;
    uint8_t writeable;
    uint8_t reverved1;
    uint8_t data[];
} key_value_cached_key_t;

typedef struct key_value_cache_state {
    struct list_head read_list;
    struct list_head write_list;
    os_mutex_h mutex;
    uint32_t length;
    bool_t write_flush;
} key_value_cache_state_t;

static key_value_cache_state_t key_value_cache_st;

static key_value_cached_key_t *key_value_cache_key_malloc(uint16_t id, const uint8_t *data,
                                                          uint32_t length)
{
    key_value_cached_key_t *key;

    key = os_mem_malloc(IOT_KEY_VALUE_MID, sizeof(key_value_cached_key_t) + length);
    if (key == NULL) {
        return NULL;
    }

    key->id = id;
    key->length = length;
    memcpy(key->data, data, length);

    return key;
}

static KV_ERROR key_value_cache_key_free(key_value_cached_key_t *key)
{
    os_mem_free(key);

    return KV_OK;
}

static key_value_cached_key_t *key_value_cache_find(const struct list_head *list, uint16_t id)
{
    key_value_cached_key_t *key;

    list_for_each_entry (key, list, node) {
        if (key->id == id) {
            return key;
        }
    }

    return NULL;
}

KV_ERROR key_value_cache_get_key(uint16_t id, uint8_t **data, uint32_t *length)
{
    key_value_cached_key_t *key;

    os_acquire_mutex(key_value_cache_st.mutex);
    key = key_value_cache_find(&key_value_cache_st.write_list, id);
    if (key != NULL) {
        os_release_mutex(key_value_cache_st.mutex);

        *data = key->data;
        *length = key->length;
        return KV_OK;
    }

    key = key_value_cache_find(&key_value_cache_st.read_list, id);
    if (key == NULL) {
        os_release_mutex(key_value_cache_st.mutex);
        return KV_KEY_NOT_FOUND;
    }

    /* move to tail */
    if (&key->node != &key_value_cache_st.read_list) {
        list_del(&key->node);
        list_add_tail(&key->node, &key_value_cache_st.read_list);
    }
    os_release_mutex(key_value_cache_st.mutex);

    *data = key->data;
    *length = key->length;

    return KV_OK;
}

KV_ERROR key_value_cache_add_key(uint16_t id, const uint8_t *data, uint32_t length)
{
    key_value_cached_key_t *key;
    struct list_head *saved;

    if (length >= CONFIG_KEY_VALUE_CACHE_LENGTH) {
        return KV_TOO_LONG;
    }

    /**
     * 1. key already cached, update key and move it to head
     * 2. key not cached, and have enough remaining space, add key to head
     * 3. key not cached, no enough space, free from tail until there is enough space
     */
    os_acquire_mutex(key_value_cache_st.mutex);
    key = key_value_cache_find(&key_value_cache_st.read_list, id);
    if (key != NULL) {
        if (key->length == length) {
            /* the key to be update has same length with cached key*/
            /* move to head */
            if (&key->node != &key_value_cache_st.read_list) {
                list_del(&key->node);
                list_add_tail(&key->node, &key_value_cache_st.read_list);
            }

            if (memcmp(key->data, data, length) != 0) {
                memcpy(key->data, data, length);
            }
            os_release_mutex(key_value_cache_st.mutex);
            return KV_OK;
        } else {
            key_value_cache_st.length -= key->length;
            list_del(&key->node);
            key_value_cache_key_free(key);
        }
    }

    if (key_value_cache_st.length + length > CONFIG_KEY_VALUE_CACHE_LENGTH) {
        /* no enough space, remove some key */
        list_for_each_entry_safe (key, &key_value_cache_st.read_list, node, saved) {
            key_value_cache_st.length -= key->length;
            list_del(&key->node);
            key_value_cache_key_free(key);

            if (key_value_cache_st.length + length <= CONFIG_KEY_VALUE_CACHE_LENGTH) {
                break;
            }
        }
    }
    os_release_mutex(key_value_cache_st.mutex);

    key = key_value_cache_key_malloc(id, data, length);
    if (key == NULL) {
        return KV_NO_MEM;
    }

    key_value_cache_st.length += key->length;
    list_add_tail(&key->node, &key_value_cache_st.read_list);

    return KV_OK;
}

KV_ERROR key_value_cache_write_key(uint16_t id, const uint8_t *data, uint32_t length, bool_t writeable)
{
    key_value_cached_key_t *key;

    /* If the key already cached in read list, update and move to write list */
    os_acquire_mutex(key_value_cache_st.mutex);
    key = key_value_cache_find(&key_value_cache_st.read_list, id);
    if (key != NULL) {
        key_value_cache_st.length -= key->length;
        if (key->length == length) {
            list_del(&key->node);
            key->writeable = writeable ? 1 : 0;
            list_add_tail(&key->node, &key_value_cache_st.write_list);

            if (memcmp(key->data, data, length) != 0) {
                memcpy(key->data, data, length);
            }

            os_release_mutex(key_value_cache_st.mutex);
            key_value_cache_flush();

            return KV_OK;
        } else {
            list_del(&key->node);
            key_value_cache_key_free(key);
        }
    } else {
        /* if the key already in write list, just update the data */
        key = key_value_cache_find(&key_value_cache_st.write_list, id);
        if (key != NULL) {
            // same length and different data
            if ((key->length == length) && (memcmp(key->data, data, length) != 0)) {
                memcpy(key->data, data, length);
                os_release_mutex(key_value_cache_st.mutex);

                return KV_OK;
            }
            list_del(&key->node);
            key_value_cache_key_free(key);
        }
    }

    key = key_value_cache_key_malloc(id, data, length);
    key->writeable = writeable ? 1 : 0;
    if (key == NULL) {
        os_release_mutex(key_value_cache_st.mutex);
        return KV_NO_MEM;
    }

    list_add_tail(&key->node, &key_value_cache_st.write_list);
    os_release_mutex(key_value_cache_st.mutex);

    key_value_cache_flush();

    return KV_OK;
}

KV_ERROR key_value_cache_invalid(uint16_t id)
{
    key_value_cached_key_t *key;

    key = key_value_cache_find(&key_value_cache_st.read_list, id);
    if (key != NULL) {
        os_acquire_mutex(key_value_cache_st.mutex);
        list_del(&key->node);
        os_release_mutex(key_value_cache_st.mutex);
    }

    return KV_OK;
}

static void key_value_cache_flush_write(void *arg)
{
    UNUSED(arg);
    key_value_cached_key_t *key;
    struct list_head *saved;

    os_acquire_mutex(key_value_cache_st.mutex);
    list_for_each_entry_safe (key, &key_value_cache_st.write_list, node, saved) {
        key_value_write_key(key->id, key->data, key->length, key->writeable != 0);
        list_del(&key->node);

        /* add to read list tail */
        list_add_tail(&key->node, &key_value_cache_st.read_list);
    }
    key_value_cache_st.write_flush = false;
    os_release_mutex(key_value_cache_st.mutex);
}

KV_ERROR key_value_cache_flush(void)
{
    os_acquire_mutex(key_value_cache_st.mutex);
    if (!key_value_cache_st.write_flush) {
        key_value_cache_st.write_flush = true;
        iot_share_task_post_event(IOT_SHARE_TASK_QUEUE_LP, IOT_SHARE_EVENT_KV_CACHE_EVENT);
    }
    os_release_mutex(key_value_cache_st.mutex);
    return KV_OK;
}

KV_ERROR key_value_cache_init(void)
{
    list_init(&key_value_cache_st.read_list);
    list_init(&key_value_cache_st.write_list);

    key_value_cache_st.mutex = os_create_mutex(IOT_KEY_VALUE_MID);
    key_value_cache_st.length = 0;
    key_value_cache_st.write_flush = false;

    uint32_t ret = iot_share_task_event_register(
        IOT_SHARE_TASK_QUEUE_LP, IOT_SHARE_EVENT_KV_CACHE_EVENT, key_value_cache_flush_write, NULL);

    if (ret != RET_OK) {
        return KV_UNKONWN_ERROR;
    }

    return KV_OK;
}
