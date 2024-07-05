#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

typedef struct Node
{
    void *data;
    struct Node *next;
    //int index;
} Node;

typedef struct LinkedList
{
    Node* front;
    Node* back;
    int size;
} LinkedList;

LinkedList *create_linked_list();

void free_linked_list(LinkedList *list);

int append(LinkedList *list, void *data);

int insert(LinkedList *list, void *data, int index);

void delete(LinkedList *list, Node *prev);