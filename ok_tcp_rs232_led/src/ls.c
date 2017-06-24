#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "tcp_rs232.h"

#include "ls.h"
int g_listid = 0;

static LIST_NODE* create_nodeex(char *data, uint8 datalen, LIST_NODE* prev,LIST_NODE* next){
 	 LIST_NODE* node = (LIST_NODE*)malloc(sizeof(LIST_NODE));
  	if(node == NULL)
	{
		printf("create_nodeex malloc null\r\n");
		return NULL;
	}
	memcpy(node->data.data, data, datalen);
	node->data.datalen = datalen;
	node->data.list_id = g_listid++;
	node->prev = prev;
	node->next = next;
	return node;
}
static LIST_NODE* destroy_node(LIST_NODE* node,LIST_NODE** prev){
  LIST_NODE* next = node->next;
	if(prev/*!=NULL*/) *prev = node->prev;
	free(node);
	return next;
}
void list_init(LIST* list){
	list->head = NULL;
	list->tail = NULL;
}
void list_deinit(LIST* list){
  while(list->head)
		list->head = destroy_node(list->head,0);
  list->tail = NULL;
}
int list_empty(LIST* list){
  return !list->head && !list->tail;
}

void list_appendex(LIST* list,char *data, uint8 datalen)
{
	rs232_send(data, datalen);return;
    list->tail = create_nodeex(data, datalen, list->tail, NULL);
	if(list->tail->prev)
    list->tail->prev->next = list->tail;
	else list->head = list->tail;
}

void list_remove(LIST* list,int list_id){                                                                    
        LIST_NODE* find = NULL;                                                                              
        for(find = list->head; find; find = find->next){                                                          
          if(find->data.list_id == list_id){       
         	LIST_NODE* prev = NULL;                                                                   
        	LIST_NODE* next = destroy_node(find,&prev);                        
         	if(prev) prev->next = next;                                                    
         	else list->head = next;                                                             
         	if(next) next->prev = prev;                                                         
         	else list->tail = prev;  
	  }                                                          
    }                                                                                                   
 }

int list_size(LIST* list){
    int size = 0;
	LIST_NODE* find = NULL;
	for(find = list->head; find; find = find->next)
		size++;
	return size;
}

void list_begin(LIST* list){
	list->frwd = list->head;
}
struct INLcpMsgSet_list* list_next(LIST* list){
	struct INLcpMsgSet_list* data = &(list->frwd->data);
	list->frwd = list->frwd->next;
	return data;
}
struct INLcpMsgSet_list* list_prev(LIST* list){
	struct INLcpMsgSet_list* data = &(list->frwd->data);
	list->frwd = list->frwd->prev;
        return data; 
}
struct INLcpMsgSet_list* list_current(LIST* list){
  return &(list->frwd->data);
}
int list_end(LIST* list){
  return ! list->frwd;
}
