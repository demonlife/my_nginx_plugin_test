
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_LIST_H_INCLUDED_
#define _NGX_LIST_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


typedef struct ngx_list_part_s  ngx_list_part_t;

struct ngx_list_part_s {
    void             *elts; // 指向数组的起始地址
    ngx_uint_t        nelts; // 数组已经使用了多个元素
    ngx_list_part_t  *next; // 下一个链表的元素ngx_list_part_t的地址
};


typedef struct {
    ngx_list_part_t  *last; // 指向最后一个数组元素
    ngx_list_part_t   part; // 链表的首个数字元素
    size_t            size; // 表示每一个数组元素的占用的空间大小
    ngx_uint_t        nalloc; // 每个ngx_list_part_t数组的容量
    ngx_pool_t       *pool; // 链表中管理内存分配的内存池对象，
    //事实上，ngx_list_t的所有数据都是由ngx_pool_t类型的pool内存池分配的
} ngx_list_t;


ngx_list_t *ngx_list_create(ngx_pool_t *pool, ngx_uint_t n, size_t size);

static ngx_inline ngx_int_t
ngx_list_init(ngx_list_t *list, ngx_pool_t *pool, ngx_uint_t n, size_t size)
{
    list->part.elts = ngx_palloc(pool, n * size);
    if (list->part.elts == NULL) {
        return NGX_ERROR;
    }

    list->part.nelts = 0;
    list->part.next = NULL;
    list->last = &list->part;
    list->size = size;
    list->nalloc = n;
    list->pool = pool;

    return NGX_OK;
}


/*
 *
 *  the iteration through the list:
 *
 *  part = &list.part;
 *  data = part->elts;
 *
 *  for (i = 0 ;; i++) {
 *
 *      if (i >= part->nelts) {
 *          if (part->next == NULL) {
 *              break;
 *          }
 *
 *          part = part->next;
 *          data = part->elts;
 *          i = 0;
 *      }
 *
 *      ...  data[i] ...
 *
 *  }
 */


void *ngx_list_push(ngx_list_t *list);


#endif /* _NGX_LIST_H_INCLUDED_ */
