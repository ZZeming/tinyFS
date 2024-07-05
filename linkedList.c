#include "linkedList.h"

LinkedList *create_linked_list()
{
    // create list
    LinkedList *list = (LinkedList*) malloc(sizeof(LinkedList));
    if (list == NULL)
        return list;

    // add dummy node
    list->front = (Node *) malloc(sizeof(Node));
    if (list->front == NULL)
    {
        perror("malloc");
        free(list);
        return NULL;
    }
    list->front->next = NULL;
    list->front->data = NULL;

    // set queue back
    list->back = list->front;

    // set queue size
    list->size = 0;

    return list;
}

void free_linked_list(LinkedList *list)
{
    Node *cur = list->front;
    Node *next;
    while (cur->next != NULL)
    {
        next = cur->next;
        free(cur->data);
        free(cur);
        cur = next;
    }
    free(cur->data);
    free(cur);
    free(list);
}

int append(LinkedList *list, void *data)
{
    return insert(list, data, list->size);
}

int insert(LinkedList *list, void *data, int index)
{
    Node *node = (Node *) malloc(sizeof(Node));
    if (node == NULL)
        return -1;
    node->data = data;
    //node->index = index;
    
    // move to correct index
    Node *cur = list->front;
    int i;
    for (i = 0; i < index; i++)
        cur = cur->next;

    // insert new node
    if (index == list->size)
    {
        node->next = NULL;

        // update back of list
        list->back = node;
    }
    else
        node->next = cur->next->next;
    cur->next = node;

    // increment list size
    list->size += 1;
    return 0;
}

void delete(LinkedList *list, Node *prev)
{
    // if last node to delete is back
    if (prev->next == list->back)
        list->back = prev;

    // free node and redirect previous node next
    Node *temp = NULL;
    temp = prev->next->next;
    free(prev->next->data);
    free(prev->next);
    prev->next = temp;

    // decrement size
    list->size -= 1;
}

// void delete(LinkedList *list, int index)
// {
//     // if index is too high, return -1
//     if (index >= list->size)
//         return -1;

//     // move to correct index
//     Node *cur = list->front;
//     int i;
//     for (i = 0; i < index; i++)
//         cur = cur->next;

//     // free node
//     Node *temp = NULL;
//     if (index < list->size - 1)
//         temp = cur->next->next;
//     free(cur->next->data);
//     free(cur->next);
//     cur->next = temp;

//     // decrement size
//     list->size -= 1;
// }

// Dumps the queue
// void dump_queue(LinkedList* queue)
// {
//     Node*cur = queue->front->next;
//     while (cur != NULL && cur->next != NULL)
//     {
//         printf("%d ", ((TLB_entry_2 *)cur->data)->page);
//         cur = cur->next;
//     }
// }