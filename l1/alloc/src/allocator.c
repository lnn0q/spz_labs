#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "allocator.h"
#include "block.h"
#include "config.h"
#include "kernel.h"

#define ARENA_SIZE (ALLOCATOR_ARENA_PAGES * ALLOCATOR_PAGE_SIZE)
#define BLOCK_SIZE_MAX (ARENA_SIZE - BLOCK_STRUCT_SIZE)

static tree_type blocks_tree = TREE_INITIALIZER;

static struct block *
arena_alloc(void)
{
    struct block *block;

    block = kernel_alloc(ARENA_SIZE);
    if (block != NULL)
        arena_init(block, ARENA_SIZE - BLOCK_STRUCT_SIZE);
    return block;
}

static void
tree_add_block(struct block *block)
{
    assert(block_get_flag_busy(block) == false);
    tree_add(&blocks_tree, block_to_node(block), block_get_size_curr(block));
}

static void
tree_remove_block(struct block *block)
{
    assert(block_get_flag_busy(block) == false);
    tree_remove(&blocks_tree, block_to_node(block));
}

void *
mem_alloc(size_t size)
{
    struct block *block, *block_r;
    tree_node_type *node;

    if (size > BLOCK_SIZE_MAX)
        return NULL;
    if (size < BLOCK_SIZE_MIN)
        size = BLOCK_SIZE_MIN;
    size = ROUND_BYTES(size);

    node = tree_find_best(&blocks_tree, size);
    if (node == NULL) {
        block = arena_alloc();
        if (block == NULL)
            return NULL;
    } else {
        tree_remove(&blocks_tree, node);
        block = node_to_block(node);
    }
    block_r = block_split(block, size);
    if (block_r != NULL)
        tree_add_block(block_r);

    return block_to_payload(block);
}

void
mem_free(void *ptr)
{
    struct block *block, *block_r, *block_l;

    if (ptr == NULL)
        return;

    block = payload_to_block(ptr);
    block_clr_flag_busy(block);

    if (!block_get_flag_last(block)) {
        block_r = block_next(block);
        if (!block_get_flag_busy(block_r)) {
            tree_remove_block(block_r);
            block_merge(block, block_r);
        }
    }
    if (!block_get_flag_first(block)) {
        block_l = block_prev(block);
        if (!block_get_flag_busy(block_l)) {
            tree_remove_block(block_l);
            block_merge(block_l, block);
            block = block_l;
        }
    }
    if (block_get_flag_first(block) && block_get_flag_last(block)) {
        kernel_free(block, ARENA_SIZE);
    } else {
        // TODO check possible memory reset
        // [@@@@                                     ]
        //      xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
        //         |         |         |         |   =======>
        //         ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
        //         ^ ptr ------------------------>
        //                        size
        //              kernel_reset(ptr, size)
        tree_add_block(block);
    }
}

void *
mem_realloc(void *ptr1, size_t size)
{
    // 1.     <------
    //    [          ]
    //
    //               --------->
    // 2. [          ][      free         ]
    //
    //    xxxxxxxxxxxx
    // 3. [          ][       busy or not 2   ]

    struct block *block1;
    void *ptr2;
    size_t size_curr;

    if (size > BLOCK_SIZE_MAX)
        return NULL;
    if (size < BLOCK_SIZE_MIN)
        size = BLOCK_SIZE_MIN;
    size = ROUND_BYTES(size);

    if (ptr1 == NULL)
        return mem_alloc(size);

    block1 = payload_to_block(ptr1);
    size_curr = block_get_size_curr(block1);
    if (size == size_curr)
        return ptr1;

    // TODO
    if (size < size_curr)
        return NULL;

    // TODO
    // increase in-place

    ptr2 = mem_alloc(size);
    if (ptr2 != NULL) {
        memcpy(ptr2, ptr1, size_curr);
        mem_free(ptr1);
    }
    return ptr2;
}

static void
show_node(const tree_node_type *node, const bool linked)
{
    const struct block *block = node_to_block(node);

    printf("[%20p] %20zu %20zu %s%s%s%s\n",
        (void *)block,
        block_get_size_curr(block), block_get_size_prev(block),
        block_get_flag_busy(block) ? "busy" : "free",
        block_get_flag_first(block) ? " first " : "",
        block_get_flag_last(block) ? " last" : "",
        linked ? " linked" : "");
}

void
mem_show(const char *msg)
{
    printf("%s:\n", msg);
    if (tree_is_empty(&blocks_tree))
        printf("Tree is empty\n");
    else
        tree_walk(&blocks_tree, show_node);
}
