#include "cstack.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define UNUSED(VAR) (void)(VAR)

static void* resize_array(
    void* arr, size_t type_size, unsigned int* arr_size, size_t new_size
)
{
    if (new_size == 0 || type_size == 0)
    {
        return NULL;
    }
    void* new_arr = realloc(arr, new_size * type_size);
    if (new_arr == NULL)
    {
        return NULL;
    }
    *arr_size = (unsigned int)new_size;
    return new_arr;
}

struct stack_node
{
    struct stack_node* prev;
    char* data;
    size_t size;
};

struct stack
{
    struct stack_node* top;
};

struct stacks
{
    unsigned int count;
    unsigned int deleted_count;
    unsigned int* deleted;
    struct stack* stacks;
};

struct stacks g_stacks = {0, 0, NULL, NULL};

hstack_t stack_new(void)
{
    if (g_stacks.stacks == NULL)
    {
        g_stacks.stacks = (struct stack*)malloc(sizeof(struct stack));
        if (g_stacks.stacks == NULL)
        {
            return -1;
        }
        g_stacks.stacks[0].top = NULL;
        g_stacks.count = 1;
        return 0;
    }
    if (g_stacks.deleted != NULL && g_stacks.deleted_count > 0)
    {
        --g_stacks.deleted_count;
        unsigned int deleted_index = g_stacks.deleted[g_stacks.deleted_count];
        g_stacks.stacks[deleted_index].top = NULL;
        return (hstack_t)deleted_index;
    }
    size_t old_count = g_stacks.count;
    struct stack* new_stacks = (struct stack*)resize_array(
        g_stacks.stacks, sizeof(struct stack), &g_stacks.count, old_count + 1
    );
    if (new_stacks == NULL)
    {
        return -1;
    }
    g_stacks.stacks = new_stacks;
    g_stacks.stacks[old_count].top = NULL;
    return (hstack_t)old_count;
}

static void stack_clear(struct stack* s)
{
    struct stack_node* current = s->top;
    while (current != NULL)
    {
        struct stack_node* temp = current;
        current = current->prev;
        free(temp->data);
        free(temp);
    }
    s->top = NULL;
}

void stack_free(const hstack_t hstack)
{
    if (!stack_valid_handler(hstack))
    {
        return;
    }
    stack_clear(&g_stacks.stacks[hstack]);
    if ((unsigned int)hstack == g_stacks.count - 1)
    {
        if (g_stacks.count == 1)
        {
            free(g_stacks.stacks);
            g_stacks.stacks = NULL;
            g_stacks.count = 0;
            free(g_stacks.deleted);
            g_stacks.deleted = NULL;
            g_stacks.deleted_count = 0;
        }
        else
        {
            size_t old_count = g_stacks.count;
            struct stack* new_stacks = (struct stack*)resize_array(
                g_stacks.stacks,
                sizeof(struct stack),
                &g_stacks.count,
                old_count - 1
            );
            if (new_stacks != NULL)
            {
                g_stacks.stacks = new_stacks;
            }
        }
    }
    else if (g_stacks.deleted == NULL)
    {
        g_stacks.deleted = (unsigned int*)malloc(sizeof(unsigned int));
        if (g_stacks.deleted == NULL)
        {
            return;
        }
        g_stacks.deleted[0] = (unsigned int)hstack;
        g_stacks.deleted_count = 1;
    }
    else
    {
        size_t old_count = g_stacks.deleted_count;
        unsigned int* new_deleted = (unsigned int*)resize_array(
            g_stacks.deleted,
            sizeof(unsigned int),
            &g_stacks.deleted_count,
            old_count + 1
        );
        if (new_deleted == NULL)
        {
            return;
        }
        g_stacks.deleted = new_deleted;
        g_stacks.deleted[old_count] = (unsigned int)hstack;
    }
}

void stack_free_all(void)
{
    if (g_stacks.stacks != NULL)
    {
        struct stack* it = g_stacks.stacks;
        struct stack* end = g_stacks.stacks + g_stacks.count;
        while (it != end)
        {
            stack_clear(it);
            ++it;
        }
    }
    free(g_stacks.stacks);
    g_stacks.stacks = NULL;
    g_stacks.count = 0;
    free(g_stacks.deleted);
    g_stacks.deleted = NULL;
    g_stacks.deleted_count = 0;
}

int stack_valid_handler(const hstack_t hstack)
{
    if (g_stacks.stacks == NULL || hstack < 0 ||
        (unsigned int)hstack >= g_stacks.count)
    {
        return 0;
    }
    for (unsigned int i = 0; i < g_stacks.deleted_count; ++i)
    {
        if (g_stacks.deleted[i] == (unsigned int)hstack)
        {
            return 0;
        }
    }
    return 1;
}

unsigned int stack_size(const hstack_t hstack)
{
    if (!stack_valid_handler(hstack))
    {
        return 0;
    }
    unsigned int size = 0;
    struct stack_node* current = g_stacks.stacks[hstack].top;
    while (current != NULL)
    {
        ++size;
        current = current->prev;
    }
    return size;
}

void
stack_push(const hstack_t hstack, const void* data_in, const unsigned int size)
{
    if (!stack_valid_handler(hstack) || data_in == NULL || size == 0)
    {
        return;
    }
    struct stack_node* new_node =
        (struct stack_node*)malloc(sizeof(struct stack_node));
    if (new_node == NULL)
    {
        return;
    }
    new_node->data = (char*)malloc(size);
    if (new_node->data == NULL)
    {
        free(new_node);
        return;
    }
    new_node->prev = g_stacks.stacks[hstack].top;
    new_node->size = size;
    memcpy(new_node->data, data_in, size);
    g_stacks.stacks[hstack].top = new_node;
}

unsigned int
stack_pop(const hstack_t hstack, void* data_out, const unsigned int size)
{
    if (!stack_valid_handler(hstack) || data_out == NULL || size == 0)
    {
        return 0;
    }
    struct stack_node* top_node = g_stacks.stacks[hstack].top;
    if (top_node == NULL)
    {
        return 0;
    }
    if ((size_t)size < top_node->size)
    {
        return 0;
    }
    unsigned int bytes = (unsigned int)top_node->size;
    memcpy(data_out, top_node->data, bytes);
    g_stacks.stacks[hstack].top = top_node->prev;
    free(top_node->data);
    free(top_node);
    return bytes;
}

unsigned stack_top_size(const hstack_t hstack)
{
    if (!stack_valid_handler(hstack))
    {
        return 0;
    }
    struct stack_node* top_node = g_stacks.stacks[hstack].top;
    if (top_node == NULL)
    {
        return 0;
    }
    return (unsigned int)top_node->size;
}

hstack_t stack_get_all(void)
{
    hstack_t new_stack = stack_new();
    if (new_stack == -1)
    {
        return -1;
    }
    for (unsigned int i = 0; i < g_stacks.count; ++i)
    {
        hstack_t h = (hstack_t)i;
        if (h != new_stack && stack_valid_handler(h))
        {
            stack_push(new_stack, &h, sizeof(hstack_t));
        }
    }
    return new_stack;
}
