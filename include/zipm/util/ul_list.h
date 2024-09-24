/*
 * Copyright (c) 2024 Felipe Neves
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __UL_LIST_H
#define __UL_LIST_H


#include <stdbool.h>

struct ul_dlist {
    struct ul_dlist *prev;
    struct ul_dlist *next;
};

typedef struct ul_dlist ul_dnode_t;

static inline void ul_dlist_init(struct ul_dlist *list)
{
    if(list) {
        list->next = list;
        list->prev = list;
    }
}

static inline void ul_dlist_append(struct ul_dlist *list, ul_dnode_t *node)
{
    if(list->next == list) {
        node->prev = list;
        node->next = list; 
        list->next = node;
        list->prev = node;
    } else {
        node->next = list->next;
        node->prev = list;
        list->next = node;
    }
}

static inline bool ul_dlist_is_empty(struct ul_dlist *list)
{
    return  (list->next == list);
}

static inline ul_dnode_t * ul_dlist_peek_head(struct ul_dlist *list)
{
    if(!ul_dlist_is_empty(list))
        return(list->prev);
    else 
        return NULL;
}

static inline void ul_dlist_remove_node(ul_dnode_t *node)
{
    node->prev->next = node->next;
    node->next->prev = node->prev;
}

#endif