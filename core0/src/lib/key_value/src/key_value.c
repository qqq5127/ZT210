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

#include "iot_flash.h"
#include "key_value_port.h"
#include "key_value.h"

#if CONFIG_KEY_VALUE_CACHE_ENABLE
#include "key_value_cache.h"
#endif

#if defined(BUILD_OS_NON_OS)
#include "critical_sec.h"
#else
#include "os_mem.h"
#include "os_lock.h"
#include "riscv_cpu.h"
#endif

#ifdef LIB_DBGLOG_ENABLE
#include "dbglog.h"
#define DBGLOG_KV_INFO(fmt, arg...)     DBGLOG_STREAM_INFO(IOT_KEY_VALUE_MID, fmt, ##arg)
#define DBGLOG_KV_WARNING(fmt, arg...)  DBGLOG_STREAM_WARNING(IOT_KEY_VALUE_MID,  fmt, ##arg)
#define DBGLOG_KV_ERROR(fmt, arg...)    DBGLOG_STREAM_ERROR(IOT_KEY_VALUE_MID, fmt, ##arg)
#else
#include "stdio.h"
#define DBGLOG_KV_INFO(fmt, arg...)     printf(fmt, ##arg)
#define DBGLOG_KV_WARNING(fmt, arg...)  printf(fmt, ##arg)
#define DBGLOG_KV_ERROR(fmt, arg...)    printf(fmt, ##arg)
#endif

typedef struct kv_alloc_info {
    uint32_t length;
    uint32_t empty_pages;
    key_value_t *kv;
} kv_alloc_info_t;

typedef struct kv_defrag_info {
    uint32_t remain_length;
    uint32_t empty_page_index;
    key_value_t *dst_kv;
} kv_defrag_info_t;

typedef struct kv_find_info {
    uint32_t id;
    uint32_t state;
} kv_find_info_t;

typedef bool_t (*page_iterator_callback)(kv_page_header_t *p_header, void *argv1, void *argv2,
                                         KV_ERROR *ret);

typedef bool_t (*key_iterator_callback)(const key_value_t *kv, void *argv1, void *argv2, KV_ERROR *ret);

static const uint32_t ket_value_state_array[] = {
    KV_KEY_STATE_UNUSED,   KV_KEY_STATE_WRITING, KV_KEY_STATE_VALID,
    KV_KEY_STATE_DELETING, KV_KEY_STATE_INVALID,
};

static const uint32_t ket_page_state_array[] = {
    KV_PAGE_STATE_EMPTY,
    KV_PAGE_STATE_USING,
    KV_PAGE_STATE_DEFRAGGING,
};

static KV_ERROR key_value_find_key(kv_find_info_t *find_key, key_value_t **kv);
static KV_ERROR key_value_recovery_deleting_key(const key_value_t *kv);
static KV_ERROR key_value_set_page_state(const kv_page_header_t *p_header, KV_PAGE_STATE state);
static KV_ERROR key_value_set_key_state(const key_value_t *kv, KV_VALUE_STATE state);
static KV_ERROR key_value_key_write(key_value_t *kv, kv_header_t *k_header, const uint8_t *data);

#if defined(BUILD_OS_NON_OS)
static uint8_t key_value_buffer[KEY_VALUE_PAGE_SIZE];
#else
static os_mutex_h key_value_mutex;
#endif

/*lint -sem(key_value_acquire_mutex, thread_lock) */
static void key_value_acquire_mutex(void)
{
#if defined(BUILD_OS_NON_OS)
    cpu_critical_enter();
#else
    os_acquire_mutex(key_value_mutex);
#endif
}

/*lint -sem(key_value_release_mutex, thread_unlock) */
static void key_value_release_mutex(void)
{
#if defined(BUILD_OS_NON_OS)
    cpu_critical_exit();
#else
    os_release_mutex(key_value_mutex);
#endif
}

static kv_page_header_t *key_value_get_next_page(kv_page_header_t *p_header)
{
    if (p_header == NULL) {
        return (kv_page_header_t *)KEY_VALUE_READ_ADDR;
    }

    p_header = (kv_page_header_t *)((uint32_t)p_header + KEY_VALUE_PAGE_SIZE);
    if ((uint32_t)p_header < KEY_VALUE_READ_ADDR + KEY_VALUE_PAGE_SIZE * KEY_VALUE_PAGE_NUM) {
        return p_header;
    }

    return NULL;
}

static key_value_t *key_value_get_next_key(key_value_t *kv)
{
    key_value_t *next;
    uint32_t key_length;

    if (kv->header.state == KV_KEY_STATE_UNUSED) {
        return NULL;
    } else if (kv->header.state == KV_KEY_STATE_WRITING) {
        // key length not set, the key just alloced but not write data.
        // So the lenght should be zero
        key_length = kv->header.length > KEY_VALUE_KEY_MAX_VALUE_LENGTH ? 0 : kv->header.length;
    } else {
        // k_header->state == KV_KEY_STATE_VALID
        key_length = kv->header.length;
    }

    if (key_length > KEY_VALUE_KEY_MAX_VALUE_LENGTH) {
        DBGLOG_KV_ERROR("[KV]Key length error in %p, length:0x%08x\n", kv, key_length);
        return NULL;
    }

    next = (key_value_t *)(((uint32_t)kv) + sizeof(kv_header_t)
                           + KEY_VALUE_ALIGN(key_length, KEY_VALUE_LENGTH_ALIGN));

    if ((uint32_t)next >= KEY_VALUE_ALIGN((uint32_t)kv, KEY_VALUE_PAGE_SIZE)) {
        return NULL;
    } else {
        return next;
    }
}

static uint16_t key_value_get_page_id(kv_page_header_t *p_header)
{
    return (uint16_t)(((uint32_t)p_header - KEY_VALUE_READ_ADDR) / KEY_VALUE_PAGE_SIZE);
}

static KV_ERROR key_value_page_format(kv_page_header_t *p_header)
{
    kv_page_header_t header;

    header.type = KV_PAGE_TYPE_NORMAL;
    header.state = KV_PAGE_STATE_EMPTY;
    header.magic = KEY_VALUE_PAGE_MAGIC;
    header.id = key_value_get_page_id(p_header);
    header.version = KV_VERSION;
    header.reserved = 0xFF;

    key_value_page_erase((uint32_t)p_header);

    int32_t ret_code =
        key_value_flash_write((uint32_t)p_header, (uint8_t *)&header, sizeof(kv_page_header_t));
    if (ret_code) {
        DBGLOG_KV_ERROR("[KV]Write flash error code:%d\n", ret_code);
        return KV_WRITE_ERROR;
    }
    return KV_OK;
}

static KV_ERROR key_value_page_iterator(void *argv1, void *argv2, page_iterator_callback callback)
{
    kv_page_header_t *p_header = NULL;
    KV_ERROR ret = KV_OK;

    assert(callback != NULL);

    p_header = key_value_get_next_page(p_header);
    while (p_header) {
        if (!callback(p_header, argv1, argv2, &ret)) {
            return ret;
        }

        p_header = key_value_get_next_page(p_header);
    }
    return ret;
}

static KV_ERROR key_value_key_iterator(kv_page_header_t *p_header, void *argv1, void *argv2,
                                       key_iterator_callback callback)
{
    key_value_t *kv;
    KV_ERROR ret = KV_OK;

    kv = (key_value_t *)(((uint32_t)p_header) + sizeof(kv_page_header_t));
    while (kv) {
        if (!callback(kv, argv1, argv2, &ret)) {
            return ret;
        }

        kv = key_value_get_next_key(kv);
    }
    return ret;
}

static bool_t key_value_page_magic_check_cb(kv_page_header_t *p_header, void *argv1, void *argv2,
                                            KV_ERROR *ret)
{
    UNUSED(argv1);
    UNUSED(argv2);
    if (p_header->magic != KEY_VALUE_PAGE_MAGIC) {
        DBGLOG_KV_ERROR("[KV]Page %p magic error, format it\n", p_header);
        *ret = key_value_page_format(p_header);

        if (*ret != KV_OK) {
            DBGLOG_KV_ERROR("[KV]Page %p format ERROR\n", p_header);
            return false;
        }
    }

    return true;
}

static KV_ERROR key_value_page_magic_check(void)
{
    return key_value_page_iterator(NULL, NULL, key_value_page_magic_check_cb);
}

static kv_page_header_t *key_value_get_page(uint32_t addr)
{
    return (kv_page_header_t *)(addr & ~(KEY_VALUE_PAGE_SIZE - 1));
}

static bool_t key_value_get_page_remaining_cb(const key_value_t *kv, void *argv1, void *argv2,
                                              KV_ERROR *ret)
{
    uint32_t *empty_offset = (uint32_t *)argv1;

    UNUSED(argv2);
    *ret = KV_OK;
    if (kv->header.state == KV_KEY_STATE_UNUSED) {
        return false;
    }

    *empty_offset +=
        sizeof(kv_header_t) + KEY_VALUE_ALIGN(kv->header.length, KEY_VALUE_LENGTH_ALIGN);

    return true;
}

static uint32_t key_value_get_page_remaining(kv_page_header_t *p_header, uint32_t *empty_offset)
{
    KV_ERROR ret;

    // Check page header
    *empty_offset = sizeof(kv_page_header_t);

    if (p_header->magic != KEY_VALUE_PAGE_MAGIC) {
        *empty_offset = KEY_VALUE_PAGE_SIZE;
        return 0;
    } else if (p_header->state == KV_PAGE_STATE_EMPTY) {
        return KEY_VALUE_PAGE_SIZE - *empty_offset;
    } else {
    }

    ret = key_value_key_iterator(p_header, empty_offset, NULL, key_value_get_page_remaining_cb);
    if (ret == KV_OK) {
        return KEY_VALUE_PAGE_SIZE - *empty_offset;
    }

    *empty_offset = KEY_VALUE_PAGE_SIZE;
    return 0;
}

static bool_t key_value_find_key_cb(const key_value_t *kv, void *argv1, void *argv2, KV_ERROR *ret)
{
    const kv_header_t *k_header = &(kv->header);
    kv_find_info_t *find_key = (kv_find_info_t *)argv1;
    key_value_t **found_kv = (key_value_t **)argv2;

    *ret = KV_KEY_NOT_FOUND;
    if ((k_header->id == find_key->id) && (k_header->state == find_key->state)) {
        // Find the key
        *found_kv = (key_value_t *)k_header;
        *ret = KV_OK;
        return false;
    } else if (k_header->state == KV_KEY_STATE_UNUSED) {
        return false;
    } else {
        return true;
    }
}

static bool_t key_value_find_cb(kv_page_header_t *p_header, void *argv1, void *argv2, KV_ERROR *ret)
{
    // Check page header
    if (p_header->magic != KEY_VALUE_PAGE_MAGIC) {
        DBGLOG_KV_ERROR("[KV]page %p, magic:0x%08x\n", p_header, p_header->magic);
        *ret = KV_READ_ERROR;
        return true;
    } else if (p_header->state == KV_PAGE_STATE_EMPTY) {
        *ret = KV_KEY_NOT_FOUND;
        return true;
    } else {
    }

    *ret = key_value_key_iterator(p_header, argv1, argv2, key_value_find_key_cb);
    if (*ret == KV_OK) {
        // Found key, stop the iterator
        return false;
    }

    return true;
}

static KV_ERROR key_value_find_key(kv_find_info_t *find_key, key_value_t **kv)
{
    return key_value_page_iterator(find_key, kv, key_value_find_cb);
}

static bool_t key_value_alloc_space_cb(kv_page_header_t *p_header, void *argv1, void *argv2,
                                       KV_ERROR *ret)
{
    uint32_t offset;
    uint32_t remain;
    kv_alloc_info_t *empty_info = (kv_alloc_info_t *)argv1;
    uint32_t state = *(uint32_t *)argv2;

    if (p_header->state == state) {
        remain = key_value_get_page_remaining(p_header, &offset);
        if (remain >= (empty_info->length + sizeof(key_value_t))) {
            empty_info->kv = (key_value_t *)((uint32_t)p_header + offset);
            *ret = KV_OK;
            return false;
        }
    } else {
        // Only if state == KV_KEY_STATE_UNUSED
        if (p_header->state == KV_PAGE_STATE_EMPTY) {
            empty_info->empty_pages++;
        }
    }

    *ret = KV_NO_ENOUGH_SPACE;
    return true;
} /*lint !e818 argv2 couldn't be declared as const */

static KV_ERROR key_value_alloc_space(key_value_t **kv, uint32_t length)
{
    uint32_t state;
    kv_alloc_info_t empty_info;
    KV_ERROR ret;

    if (length > KEY_VALUE_KEY_MAX_VALUE_LENGTH) {
        return KV_TOO_LONG;
    }

    *kv = NULL;
    empty_info.length = length;
    empty_info.empty_pages = 0;
    empty_info.kv = NULL;

    // First try to alloc in none empty pages
    state = KV_PAGE_STATE_USING;
    ret = key_value_page_iterator(&empty_info, &state, key_value_alloc_space_cb);
    if (ret == KV_OK) {
        *kv = empty_info.kv;
        return ret;
    }

    assert(empty_info.empty_pages);

    // At least 1 more page than KEY_VALUE_DEFRAG_PAGE_NUM
    if (empty_info.empty_pages > KEY_VALUE_DEFRAG_PAGE_NUM) {
        // Then try to alloc in empty pages
        state = KV_PAGE_STATE_EMPTY;
        ret = key_value_page_iterator(&empty_info, &state, key_value_alloc_space_cb);
        if (ret == KV_OK) {
            *kv = empty_info.kv;
            return ret;
        }
    } else {
        return KV_NO_ENOUGH_SPACE;      //lint !e527 assert could be empty
    }

    // Should never be here;
    return KV_UNKONWN_ERROR;
}

/* Only used at key_value_write_key, no need to take mutex */
static KV_ERROR key_value_try_alloc(key_value_t **kv, uint32_t length)
{
    KV_ERROR ret;

    // Alloc space
    ret = key_value_alloc_space(kv, length);
    if (ret == KV_NO_ENOUGH_SPACE) {
        ret = key_value_defrag();
        if (ret != KV_OK) {
            DBGLOG_KV_ERROR("[KV]defrag error in alloc\n");
            return KV_NO_ENOUGH_SPACE;
        } else {
            return key_value_alloc_space(kv, length);
        }
    }

    return ret;
}

static KV_ERROR key_value_move_key(key_value_t *dst, const key_value_t *src)
{
    kv_header_t k_header;
    uint8_t *data;
    KV_ERROR ret;

    assert(src->header.length < KEY_VALUE_KEY_MAX_VALUE_LENGTH);

    /** old data may read from flash through cache,
     * we need to read it ou first then write to other place
     */
#if defined(BUILD_OS_NON_OS)
    data = key_value_buffer;
#else
    /** key-value API should not be called in ISR context except in panic status,
     * so here we can use os_mem_malloc_panic in panic status
     */

    if (in_irq() || !cpu_get_int_enable()) {
        data = os_mem_malloc_panic(IOT_KEY_VALUE_MID, src->header.length);
    } else {
        data = os_mem_malloc(IOT_KEY_VALUE_MID, src->header.length);
    }

    assert(data != NULL);
#endif

    memcpy(&k_header, &src->header, sizeof(kv_header_t));
    memcpy(data, src->data, src->header.length);

    // Mark src key for deleting
    ret = key_value_set_key_state(src, KV_VALUE_DELETING);
    if (ret != KV_OK) {
        return ret;
    }

    ret = key_value_key_write(dst, &k_header, data);

#if !defined(BUILD_OS_NON_OS)
    if (in_irq() || !cpu_get_int_enable()) {
        os_mem_free_panic(data);
    } else {
        os_mem_free(data);
    }
#endif

    if (ret != KV_OK) {
        return ret;
    }

    // Mark src to invalid
    ret = key_value_set_key_state(src, KV_VALUE_INVALID);

    return ret;
}

static bool_t key_value_get_page_state_cb(kv_page_header_t *p_header, void *argv1, void *argv2,
                                          KV_ERROR *ret)
{
    kv_page_state_t *state = (kv_page_state_t *)argv1;

    UNUSED(argv2);
    switch (p_header->state) {
        case KV_PAGE_STATE_EMPTY:
            state->empty[state->empty_num++] = p_header;
            break;
        case KV_PAGE_STATE_USING:
            state->using[state->using_num++] = p_header;
            break;
        case KV_PAGE_STATE_DEFRAGGING:
            state->defrag[state->defrag_num++] = p_header;
            break;
        default:
            break;
    }

    *ret = KV_OK;
    return true;
}

static void key_value_get_all_page_state(kv_page_state_t *state)
{
    key_value_page_iterator(state, NULL, key_value_get_page_state_cb);
}

static bool_t key_value_recovery_defragging_page_key_cb(const key_value_t *kv, void *argv1,
                                                        void *argv2, KV_ERROR *ret)
{
    if (kv->header.state == KV_KEY_STATE_UNUSED) {
        return false;
    }

    UNUSED(argv1);
    UNUSED(argv2);

    /**
     * if is a valid key, the defrag must not start yet.
     */
    if (kv->header.state == KV_KEY_STATE_VALID) {
        key_value_set_key_state(kv, KV_VALUE_DELETING);

        key_value_t *kv_new;
        kv_header_t k_header;
        *ret = KV_NO_ENOUGH_SPACE;
        uint32_t remain = 0;

        // find a page to store the key
        kv_page_header_t *p = key_value_get_next_page(NULL);
        while (p) {
            uint32_t offset = 0;
            remain = key_value_get_page_remaining(p, &offset);
            if (remain >= (kv->header.length + sizeof(key_value_t))) {
                kv_new = (key_value_t *)((uint32_t)p + offset);
                memcpy(&k_header, &kv->header, sizeof(kv_header_t));
                *ret = key_value_key_write(kv_new, &k_header, kv->data);
                break;
            }
            p = key_value_get_next_page(p);
        }

        assert (*ret == KV_OK);

        key_value_set_key_state(kv, KV_VALUE_INVALID);
    } else if (kv->header.state == KV_KEY_STATE_DELETING) {
        kv_find_info_t key_info;
        key_value_t *found_key;
        key_info.id = kv->header.id;
        key_info.state = KV_KEY_STATE_VALID;

        if (key_value_find_key(&key_info, &found_key) == KV_OK) {
            // Have a valid key, just set this one invalid.
            key_value_set_key_state(kv, KV_VALUE_INVALID);
            return true;
        }

        key_info.state = KV_KEY_STATE_WRITING;
        if (key_value_find_key(&key_info, &found_key) == KV_OK) {
            // Stop in writing, write it again
            key_value_move_key(found_key, kv);
            key_value_set_key_state(kv, KV_VALUE_INVALID);
            return true;
        }

        // Not a writing or valid key, set the key to a new, with state valid.
        *ret =
            key_value_write_key(kv->header.id, kv->data, kv->header.length, !!kv->header.writeable);
        if (*ret != KV_OK) {
            return false;
        }

        key_value_set_key_state(kv, KV_VALUE_INVALID);
    }

    return true;
}

static KV_ERROR key_value_recovery_defragging_page(kv_page_header_t *p_header)
{
    KV_ERROR ret;

    ret = key_value_key_iterator(p_header, NULL, NULL, key_value_recovery_defragging_page_key_cb);
    if (ret != KV_OK) {
        return ret;
    }

    return key_value_page_format(p_header);
}

static bool_t key_value_defrag_key_cb(const key_value_t *kv, void *argv1, void *argv2, KV_ERROR *ret)
{
    kv_defrag_info_t *defrag_info = (kv_defrag_info_t *)argv1;
    kv_page_state_t *page_state = (kv_page_state_t *)argv2;

    *ret = KV_OK;
    if (kv->header.state == KV_KEY_STATE_UNUSED) {
        return false;
    } else if (kv->header.state == KV_KEY_STATE_VALID) {
        // Move the valid key to new page
        if (defrag_info->remain_length
            >= (KEY_VALUE_ALIGN(kv->header.length, KEY_VALUE_LENGTH_ALIGN) + sizeof(kv_header_t))) {
            key_value_move_key(defrag_info->dst_kv, kv);
        } else {
            defrag_info->empty_page_index++;
            // At least one page has been defragged.
            assert(defrag_info->empty_page_index < page_state->empty_num);
            defrag_info->dst_kv =
                (key_value_t *)((uint32_t)page_state->empty[defrag_info->empty_page_index]
                                + sizeof(kv_page_header_t));
            defrag_info->remain_length = KEY_VALUE_PAGE_SIZE - sizeof(kv_page_header_t);
            if (key_value_move_key(defrag_info->dst_kv, kv) != KV_OK) {
                *ret = KV_WRITE_ERROR;
                return false;
            }
        }
        defrag_info->dst_kv = key_value_get_next_key(defrag_info->dst_kv);
        defrag_info->remain_length -=
            (KEY_VALUE_ALIGN(kv->header.length, KEY_VALUE_LENGTH_ALIGN) + sizeof(kv_header_t));
    } else {
    }

    return true;
}

KV_ERROR key_value_defrag(void)
{
    kv_page_state_t page_state = {0};
    kv_page_header_t *empty[KEY_VALUE_PAGE_NUM];
    kv_page_header_t *using[KEY_VALUE_PAGE_NUM];
    kv_page_header_t *defrag[KEY_VALUE_PAGE_NUM];
    kv_defrag_info_t defrag_info;
    KV_ERROR ret;

    page_state.empty = empty;
    page_state.using = using;
    page_state.defrag = defrag;

    DBGLOG_KV_INFO("[KV]Do degrag ... \n");

    key_value_get_all_page_state(&page_state);

    assert(page_state.empty_num);

    defrag_info.empty_page_index = 0;
    defrag_info.remain_length = KEY_VALUE_PAGE_SIZE - sizeof(kv_page_header_t);
    defrag_info.dst_kv = (key_value_t *)(((uint32_t)page_state.empty[defrag_info.empty_page_index])
                                         + sizeof(kv_page_header_t));

    for (uint32_t j = 0; j < page_state.using_num; j++) {
        // Set page state to defragging
        key_value_set_page_state(page_state.using[j], KV_PAGE_DEFRAGGING);

        ret = key_value_key_iterator(page_state.using[j], &defrag_info, &page_state,
                                     key_value_defrag_key_cb);
        if (ret != KV_OK) {
            return ret;
        }

        // Page defrag done, format it and mark it as empty
        if (key_value_page_format(page_state.using[j]) != KV_OK) {
            return KV_WRITE_ERROR;
        }

        page_state.empty[page_state.empty_num++] = page_state.using[j];

        // A page only contains valid key, the first empty page must full here,
        // dst_kv maybe NULL.
        if (defrag_info.dst_kv == NULL) {
            defrag_info.empty_page_index++;
            assert(defrag_info.empty_page_index < page_state.empty_num);
            defrag_info.dst_kv =
                (key_value_t *)((uint32_t)page_state.empty[defrag_info.empty_page_index]
                                + sizeof(kv_page_header_t));
            defrag_info.remain_length = KEY_VALUE_PAGE_SIZE - sizeof(kv_page_header_t);
        }
    }

    return KV_OK;
}

static KV_ERROR key_value_recovery_deleting_key(const key_value_t *kv)
{
    key_value_t *new_key;
    kv_find_info_t key_info;
    uint8_t *data;

    // Find if there is a same key id has been set.
    key_info.id = kv->header.id;
    key_info.state = KV_KEY_STATE_VALID;
    if (key_value_find_key(&key_info, &new_key) == KV_OK) {
        // If there is a new key wrote done, invalidate this one.
        return key_value_set_key_state(kv, KV_VALUE_INVALID);
    } else {
        /** The new key not wrote, move this one as a valid.
         * Only one kv should be mark as deleting as the same time.
         * So if no enought space to store is, read the key out of flash
         * and do a defrag. then write it back.
         */
        KV_ERROR ret;

#if defined(BUILD_OS_NON_OS)
        data = key_value_buffer;
#else
        data = os_mem_malloc(IOT_KEY_VALUE_MID, kv->header.length);
        assert(data != NULL);
#endif

        memcpy(data, kv->data, kv->header.length);
        ret = key_value_write_key(kv->header.id, data, kv->header.length, !!kv->header.writeable);

#if !defined(BUILD_OS_NON_OS)
        os_mem_free(data);
#endif

        return ret;
    }
}

static bool_t key_value_recovery_key_cb(const key_value_t *kv, void *argv1, void *argv2, KV_ERROR *ret)
{
    UNUSED(argv1);
    UNUSED(argv2);
    if (kv->header.state == KV_KEY_STATE_UNUSED) {
        return false;
    } else if (kv->header.state == KV_KEY_STATE_WRITING) {
        // If header not write, set length to 0, and invalidate it.
        // If header wrote, just invalidate it.
        if (kv->header.length > KEY_VALUE_KEY_MAX_VALUE_LENGTH) {
            uint32_t length = 0;
            int32_t ret_code = key_value_flash_write((uint32_t)&kv->header.length,
                                                     (uint8_t *)&length, sizeof(uint32_t));
            if (ret_code) {
                DBGLOG_KV_ERROR("[KV]Write flash error code:%d\n", ret_code);
                *ret = KV_WRITE_ERROR;
                return false;
            }
        }

        *ret = key_value_set_key_state(kv, KV_VALUE_INVALID);
        if (*ret != KV_OK) {
            return false;
        }
    } else if (kv->header.state == KV_KEY_STATE_DELETING) {
        *ret = key_value_recovery_deleting_key(kv);
        if (*ret != KV_OK) {
            return false;
        }
    } else if (kv->header.state != KV_KEY_STATE_INVALID && kv->header.state != KV_KEY_STATE_VALID) {
        // Bad page, case interrupt erase page, format page
        kv_page_header_t *p_header =
            (kv_page_header_t *)KEY_VALUE_ALIGN_DOWN((uint32_t)kv, KEY_VALUE_PAGE_SIZE);
        key_value_page_format(p_header);
    }

    return true;
}

static bool_t key_value_recovery_cb(kv_page_header_t *p_header, void *argv1, void *argv2,
                                    KV_ERROR *ret)
{
    UNUSED(argv2);

    uint32_t defrag_state = (uint32_t)argv1;

    if (p_header->state != defrag_state) {
        return true;
    }

    // Using page,.check all key state
    switch (p_header->state) {
        case KV_PAGE_STATE_EMPTY:
            return true;
        case KV_PAGE_STATE_USING:
            *ret = key_value_key_iterator(p_header, NULL, NULL, key_value_recovery_key_cb);
            if (*ret != KV_OK) {
                return false;
            }
            break;
        case KV_PAGE_STATE_DEFRAGGING:
            DBGLOG_KV_WARNING("[KV]KV page %d state error, recovering...\n", p_header->id);
            // For defragging page, continue to finash it.
            *ret = key_value_recovery_defragging_page(p_header);
            if (*ret != KV_OK) {
                return false;
            }
            break;
        default:
            break;
    }

    return true;
}

KV_ERROR key_value_recovery(void)
{
    KV_ERROR ret;
    ret = key_value_page_iterator((void*)KV_PAGE_STATE_DEFRAGGING, NULL, key_value_recovery_cb);
    if (ret != KV_OK) {
        return ret;
    }

    return key_value_page_iterator((void*)KV_PAGE_STATE_USING, NULL, key_value_recovery_cb);
}

KV_ERROR key_value_init(void)
{
    KV_ERROR ret;
#if !defined(BUILD_OS_NON_OS)
    key_value_mutex = os_create_mutex(IOT_KEY_VALUE_MID);
#endif

    ret = key_value_page_magic_check();
    if (ret != KV_OK) {
        return ret;
    }

#if CONFIG_KEY_VALUE_CACHE_ENABLE
    key_value_cache_init();
#endif

    return key_value_recovery();
}

static bool_t key_value_force_reset_cb(kv_page_header_t *p_header, void *argv1, void *argv2,
                                       KV_ERROR *ret)
{
    UNUSED(argv1);
    UNUSED(argv2);
    *ret = key_value_page_format(p_header);
    if (*ret != KV_OK) {
        return false;
    }

    return true;
}

KV_ERROR key_value_force_reset(void)
{
    KV_ERROR ret;

    ret = key_value_page_iterator(NULL, NULL, key_value_force_reset_cb);
    if (ret != KV_OK) {
        return ret;
    }

    // Init again
    return key_value_page_magic_check();
}

static KV_ERROR key_value_set_page_state(const kv_page_header_t *p_header, KV_PAGE_STATE state)
{
    uint8_t data[4] = {0};
    uint8_t len = 0;
    uint8_t current_state = 0;

    if (p_header->state < ket_page_state_array[state]) {
        return KV_PAGE_STATE_ERROR;
    } else if (p_header->state == ket_page_state_array[state]) {
        return KV_OK;
    } else {
        for (uint8_t i = 0; i < ARRAY_SIZE(ket_page_state_array); i++) {
            if (p_header->state == ket_page_state_array[i]) {
                current_state = i;
                break;
            }
        }
        len = state - current_state;

        int32_t ret_code = key_value_flash_write((uint32_t)&p_header->state + current_state, data,
                                                 sizeof(uint8_t) * len);
        if (ret_code) {
            DBGLOG_KV_ERROR("[KV]Write flash error code:%d\n", ret_code);
            return KV_WRITE_ERROR;
        }
        return KV_OK;
    }
}

static KV_ERROR key_value_set_key_state(const key_value_t *kv, KV_VALUE_STATE state)
{
    uint8_t data[4] = {0};
    uint8_t current_state = 0;
    uint8_t len = 0;

    if (kv->header.state < ket_value_state_array[state]) {
        DBGLOG_KV_ERROR("[KV]Key state error new:0x%08x, old:0x%08x\n", state, kv->header.state);
        return KV_KEY_STATE_ERROR;
    } else if (kv->header.state == ket_value_state_array[state]) {
        return KV_OK;
    } else {

        for (uint8_t i = 0; i < ARRAY_SIZE(ket_value_state_array); i++) {
            if (kv->header.state == ket_value_state_array[i]) {
                current_state = i;
                break;
            }
        }

        len = state - current_state;

        int32_t ret_code = key_value_flash_write((uint32_t)&kv->header.state + current_state, data,
                                                 sizeof(uint8_t) * len);
        if (ret_code) {
            DBGLOG_KV_ERROR("[KV]Write flash error code:%d\n", ret_code);
            return KV_WRITE_ERROR;
        }
        return KV_OK;
    }
}

static KV_ERROR key_value_key_write(key_value_t *kv, kv_header_t *k_header, const uint8_t *data)
{
    // Set page state to using when write to a empty page
    kv_page_header_t *p_header = key_value_get_page((uint32_t)kv);
    key_value_set_page_state(p_header, KV_PAGE_USING);

    /**
     * 1. Write header with state KV_KEY_STATE_WRITING
     * 2. Write data
     * 3. Modify state to KV_KEY_STATE_VALID
     */
    k_header->state = KV_KEY_STATE_WRITING;
    int32_t ret_code =
        key_value_flash_write((uint32_t)kv, (uint8_t *)k_header, sizeof(kv_header_t));
    if (ret_code) {
        DBGLOG_KV_ERROR("[KV]Write header error code:%d\n", ret_code);
        return KV_WRITE_ERROR;
    }

    ret_code = key_value_flash_write((uint32_t)kv + sizeof(kv_header_t), data, k_header->length);
    if (ret_code) {
        DBGLOG_KV_ERROR("[KV]Write data error code:%d\n", ret_code);
        return KV_WRITE_ERROR;
    }

    return key_value_set_key_state(kv, KV_VALUE_VALID);
}

static KV_ERROR key_value_remove_key(uint16_t id)
{
    key_value_t *kv;
    kv_find_info_t key_info;
    KV_ERROR ret;

    /* First find the key id already exist*/
    key_info.id = id;
    key_info.state = KV_KEY_STATE_VALID;
    ret = key_value_find_key(&key_info, &kv);
    if (ret == KV_OK) {
        if (kv->header.writeable) {
            key_value_set_key_state(kv, KV_VALUE_INVALID);
            return ret;
        } else {
            assert(0);
            return KV_PERMISSION_DENIED;    //lint !e527 assert could be empty
        }
    } else if (ret != KV_KEY_NOT_FOUND) {
        return KV_READ_ERROR;
    } else {
        return ret;
    }
}

KV_ERROR key_value_write_key(uint16_t id, const uint8_t *data, uint32_t length, bool_t writeable)
{
    key_value_t *kv_old;
    key_value_t *kv_new;
    kv_header_t k_header;
    kv_find_info_t key_info;
    KV_ERROR ret;
    bool_t key_exist = false;

    if (length == 0) {
        // Remove a key
        return key_value_remove_key(id);
    }

    /* Must alloc at first, cause alloc may do a defrag */
    ret = key_value_try_alloc(&kv_new, length);
    if (ret == KV_TOO_LONG) {
        DBGLOG_KV_ERROR("[KV]Key: %d, value too long, need %d bytes\n", id, length);
        return KV_TOO_LONG;
    } else if (ret == KV_NO_ENOUGH_SPACE) {
        DBGLOG_KV_ERROR("[KV]No enought space in set key:%d, length:%d\n", id, length);
        return KV_NO_ENOUGH_SPACE;
    } else if (ret != KV_OK) {
        // Unkonwn error
        DBGLOG_KV_ERROR("[KV]Unkonwn error in set key:%d\n", id);
        return ret;
    } else {
        // KV_OK do nothing
    }

    /* First find the same key id already exist*/
    key_info.id = id;
    key_info.state = KV_KEY_STATE_VALID;
    ret = key_value_find_key(&key_info, &kv_old);
    if (ret == KV_OK) {
        key_exist = true;
    } else if (ret != KV_KEY_NOT_FOUND) {
        return KV_READ_ERROR;
    } else {
        // Do nothing
    }

    if (key_exist) {
        if ((kv_old->header.length == length) && (memcmp(kv_old->data, data, length) == 0)) {
            /* same key */
            return KV_OK;
        }
        // Old key exist mark it as deleting
        key_value_set_key_state(kv_old, KV_VALUE_DELETING);
    }

    assert(kv_new != NULL);

    k_header.id = id;
    k_header.length = length;
    k_header.writeable = writeable ? 1 : 0;
    k_header.reverved1 = 0xFF;

    ret = key_value_key_write(kv_new, &k_header, data);
    if (ret != KV_OK) {
        return ret;
    }

    // Mark the old key as invalid
    if (key_exist) {
        key_value_set_key_state(kv_old, KV_VALUE_INVALID);
    }

    return KV_OK;
}

KV_ERROR key_value_read_key(uint16_t id, uint8_t **data, uint32_t *length)
{
    kv_find_info_t key_info;
    key_value_t *kv;
    KV_ERROR ret;

    key_info.id = id;
    key_info.state = KV_KEY_STATE_VALID;

    ret = key_value_find_key(&key_info, &kv);

    if (ret == KV_OK) {
        *data = kv->data;
        *length = kv->header.length;
    }

    return ret;
}

KV_ERROR key_value_del_key(uint16_t id)
{
    DBGLOG_KV_INFO("[KV] del key id:%d\n", id);

    key_value_acquire_mutex();
    KV_ERROR ret = key_value_remove_key(id);
    key_value_release_mutex();
    return ret;
}

KV_ERROR key_value_set_key(uint16_t id, const uint8_t *data, uint32_t length, bool_t writeable)
{
    key_value_t *kv_old;
    kv_find_info_t key_info;
    KV_ERROR ret;

    DBGLOG_KV_INFO("[KV] set key id:%d, length:%d, writeable:%d\n", id, length, writeable);

    key_value_acquire_mutex();

    /* First find the same key id already exist*/
    key_info.id = id;
    key_info.state = KV_KEY_STATE_VALID;
    ret = key_value_find_key(&key_info, &kv_old);
    if (ret == KV_OK) {
        if ((kv_old->header.length == length) && (memcmp(kv_old->data, data, length) == 0)) {
            // set key same with old one, nothing to do.
        } else if (kv_old->header.writeable) {
#if CONFIG_KEY_VALUE_CACHE_ENABLE
            ret = key_value_cache_write_key(id, data, length, writeable);
#else
            ret = key_value_write_key(id, data, length, writeable);
#endif
        } else {
            assert(0);
            ret = KV_PERMISSION_DENIED; //lint !e527 assert could be empty
        }
    } else if (ret != KV_KEY_NOT_FOUND) {
        ret = KV_READ_ERROR;
    } else {
#if CONFIG_KEY_VALUE_CACHE_ENABLE
        ret = key_value_cache_write_key(id, data, length, writeable);
#else
        ret = key_value_write_key(id, data, length, writeable);
#endif
    }

    key_value_release_mutex();
    return ret;
}

KV_ERROR key_value_get_key(uint16_t id, uint8_t **data, uint32_t *length)
{
    KV_ERROR ret;

    key_value_acquire_mutex();

#if CONFIG_KEY_VALUE_CACHE_ENABLE
    ret = key_value_cache_get_key(id, data, length);
    if (ret == KV_OK) {
        key_value_release_mutex();

        if (*length == 0)
        {
            return KV_KEY_NOT_FOUND;
        }
        return KV_OK;
    }
#endif

    ret = key_value_read_key(id, data, length);

#if CONFIG_KEY_VALUE_CACHE_ENABLE
    if (ret == KV_OK) {
        key_value_cache_add_key(id, *data, *length);
    }
#endif

    key_value_release_mutex();

    return ret;
}

static bool_t key_value_dump_keys_cb(const key_value_t *kv, void *argv1, void *argv2, KV_ERROR *ret)
{
    const kv_header_t *k_header = &kv->header;

    UNUSED(argv1);
    UNUSED(argv2);
    *ret = KV_OK;
    if (k_header->state == KV_KEY_STATE_UNUSED) {
        return false;
    }

    DBGLOG_KV_INFO("[KV]Key: %04d, addr:%p, length:%d, state:0x%08x, writeable:%d\n", k_header->id,
             k_header, k_header->length, k_header->state, k_header->writeable);

    return true;
}

static bool_t key_value_dump_page_keys_cb(kv_page_header_t *p_header, void *argv1, void *argv2,
                                          KV_ERROR *ret)
{
    DBGLOG_KV_INFO("[KV]Page %p, state:0x%08x, id:%d, version:%d\n", p_header, p_header->state,
             p_header->id, p_header->version);
    UNUSED(argv1);
    UNUSED(argv2);
    *ret = KV_OK;
    if (p_header->state == KV_PAGE_STATE_EMPTY) {
        return true;
    }

    key_value_key_iterator(p_header, NULL, NULL, key_value_dump_keys_cb);
    return true;
}

void key_value_dump_all_keys(void)
{
    key_value_page_iterator(NULL, NULL, key_value_dump_page_keys_cb);
}
