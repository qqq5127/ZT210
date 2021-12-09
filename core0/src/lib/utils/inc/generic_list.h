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

#ifndef COMMON_LIST_H
#define COMMON_LIST_H

/* -----------------------------------------------------------------------*/

/* TYPES */

struct list_head {
    struct list_head *next;
    struct list_head *prev;
};

/* MACROS */

#define LIST_HEAD_INIT(name) \
    {                        \
        &(name), &(name)     \
    }

#define LIST_HEAD(name) struct list_head name = LIST_HEAD_INIT(name)

#define INIT_LIST_HEAD(_list_)                      \
    {                                               \
        (_list_)->next = (_list_)->prev = (_list_); \
    }

/**
 * @brief This function is to init the list
 *
 * @param list is two-way linked list
 */
static inline void list_init(struct list_head *list)
{
    list->prev = list;
    list->next = list;
} /* list_init() */

/* Insert an entry _after_ the specified entry */

/**
 * @brief This function is to add a member to the list
 *
 * @param newent is a new list member
 * @param afterthisent is pointer where to add members to a linked list from head
 */
static inline void list_add(struct list_head *newent, struct list_head *afterthisent)
{
    struct list_head *next = afterthisent->next;
    newent->next = next;
    newent->prev = afterthisent;
    afterthisent->next = newent;
    next->prev = newent;
} /* list_add() */

/**
 * @brief This function is to insert an entry _before_ the specified entry
 *
 * @param newent is a new list member
 * @param beforethisent is pointer where to add members to a linked list from tail
 */
static inline void list_add_tail(struct list_head *newent, struct list_head *beforethisent)
{
    struct list_head *prev = beforethisent->prev;
    newent->prev = prev;
    newent->next = beforethisent;
    beforethisent->prev = newent;
    prev->next = newent;
} /* list_add_tail() */

/**
 * @brief This function is to delete the specified entry
 *
 * @param ent is member that need to be deleted
 */
static inline void list_del(struct list_head *ent)
{
    ent->prev->next = ent->next;
    ent->next->prev = ent->prev;
} /* list_del() */

/**
 * @brief This function is to check if the list is empty
 *
 * @param list is two-way linked list
 * @return int true - link empty false - link is not empty
 */
static inline int list_empty(struct list_head *list)
{
    return (list->next == list);
} /* list_empty() */

/**
 * @brief This function is to check how many items in the list
 *
 * @param list is two-way linked list
 * @return int - length of the list
 */
static inline unsigned int list_len(struct list_head *list)
{
    unsigned int len = 0;
    const struct list_head *p = list;
    while (p->next != list) {
        p = p->next;
        len++;
    }
    return len;
} /* list_len() */

/** list_entry - Assuming you have a struct of type _type_ that contains a list which has the
 * name _member_ in that struct type, then given the address of that list in the struct, _list_,
 * this returns the address of the container structure
 */
/*lint -emacro(826, list_entry) Suspicious pointer-to-pointer conversion (area too small) */
#define list_entry(_list_, _type_, _member_) \
    ((_type_ *)((char *)(_list_) - (char *)(offsetof(_type_, _member_))))

/* list_for_each - using _ent_, iterate through list _list_ */
#define list_for_each(_ent_, _list_) \
    for ((_ent_) = (_list_)->next; (_ent_) != (_list_); (_ent_) = (_ent_)->next)

/*
 * list_for_each_entry - this function can be use to iterate over all
 * items in a list* _list_ with it's head at _head_ and link _item_
 */
/*lint -emacro({530}, list_for_each_entry) Symbol '_list_' (line 159) not initialized */
/*lint -emacro({826}, list_for_each_entry) Suspicious pointer-to-pointer conversion (area too small) */
#define list_for_each_entry(_list_, _head_, _item_)                      \
    for ((_list_) = list_entry((_head_)->next, typeof(*_list_), _item_); \
         &((_list_)->_item_) != (_head_);                                \
         (_list_) = list_entry((_list_)->_item_.next, typeof(*_list_), _item_))

/*
 * list_for_each_entry_safe - this function can be use to iterate over all
 * items in a list* _list_ with it's head at _head_ and link _item_
 * list_del can be called inside
 */
/*lint -emacro({530}, list_for_each_entry_safe) Symbol '_list_' (line 159) not initialized */
/*lint -emacro({613}, list_for_each_entry_safe) Possible use of null pointer _list_ */
/*lint -emacro({826}, list_for_each_entry_safe) Suspicious pointer-to-pointer conversion (area too small) */
#define list_for_each_entry_safe(_list_, _head_, _item_, _saved)         \
    for ((_list_) = list_entry((_head_)->next, typeof(*_list_), _item_), \
        _saved = (_list_)->_item_.next;                                  \
         &((_list_)->_item_) != (_head_);                                \
         (_list_) = list_entry(_saved, typeof(*_list_), _item_), _saved = (_list_)->_item_.next)

/* -----------------------------------------------------------------------*/
#endif /* #ifndef COMMON_LIST_H */
/* EOF list.h */
