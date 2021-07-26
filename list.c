#include "helpers.h"


List *createList(char* topic)
{   List* list;
    list=(List*)malloc(sizeof(List));
    list->topic = strdup(topic);
    list->head = NULL;
    list->tail = NULL;
        return list;

}

ListNode *createListNode(char* data, int sock)
{   
    ListNode* nou;
    nou=(ListNode*)malloc(sizeof(ListNode));
    nou->data = strdup(data);
    nou->sock = sock;
    nou->next=NULL;
  return nou;

}

void destroyListNode(ListNode *node)
{   
    node->next = NULL;
    free(node); 
    
}

void destroyListNodes(List *node)
{   
    ListNode *p = node->head;
    
    ListNode *q;
    

    while( p!= NULL )
    {   
        q=p;
        p=p->next;

        free(q);

        
    }
  
        node->tail=NULL;
        node->head=NULL;


}

void destroyList(List *list)
{
    free(list);
    
    
}

int isEmpty(List *list)
{
    if (list->head == NULL )
        return 1;
    return 0;
}

void insert(List *list, char* data, int sock)
{   

    if(isEmpty(list)){
         ListNode* nou = createListNode(data, sock);
         nou->next = NULL;
         list->head = nou;
         list->tail = nou;

    }
    else{
        ListNode* nou = createListNode(data, sock);
        nou->next = list->head;
        list->head = nou;
    }

}

void delete(List *list, char* data)
{


	if (list->head == NULL){
		printf("Eroare delete");
		return;
	}

	if (strcmp(list->head->data,data) == 0) {
		ListNode *t = list->head;
		list->head = list->head->next;

		destroyListNode(t);

		return;
	}
	ListNode *p = list->head;
	ListNode *q = p;

	while (p != NULL && strcmp(data, p->data) != 0) {
		q = p;
		p = p->next;
	}

	q->next = p->next;
	destroyListNode(p);
    
}

ListNode *search(List *list, char* data)
{
     ListNode *p=list->head;
    
    while( p != NULL && strcmp(data, p->data) != 0){
        p = p->next;
    }
    if( p != NULL )
        return p;
    return NULL;
}

void printList(List *list)
{ 
    ListNode* temp = list->head;
    while( temp != NULL ){
        printf(" %s %d\n",temp->data,temp->sock);
        temp = temp->next;
    }
    
}
void insertSFList(ListSFNode **root, char *id)
{
	ListSFNode *newnode = malloc(sizeof(ListSFNode));
	newnode->id = strdup(id);
	newnode->list_topic_size = 0;

	newnode->next = *root;
	*root = newnode;
}

void updateSFList(ListSFNode ** root, char *id, char *newtopic)
{
	ListSFNode *p;

	p = searchSF(root,id);

	if (p == NULL)
		printf("Err at search\n");
	else {
		strcpy(p->topics[p->list_topic_size], newtopic);
		p->list_topic_size++;
	}

}

ListSFNode *searchSF(ListSFNode ** root, char *id)
{
	ListSFNode *p;

	for (p = *root; p != NULL; p = p->next)
		if (strcmp(p->id, id) == 0)
			return p;
	return NULL;	

}
