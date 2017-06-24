#ifndef _LS_H_
#define _LS_H_
#include "INLcpMsg.h"

typedef struct ListNode{
  	struct INLcpMsgSet_list data;
	struct ListNode* prev;
	struct ListNode* next;
} LIST_NODE;
typedef struct List{
    LIST_NODE* head;
	LIST_NODE* tail;
	LIST_NODE* frwd;
	LIST_NODE* bkwd;
} LIST;
void list_init(LIST* list);
void list_deinit(LIST* list);
int list_empty(LIST* list);
void list_append(LIST* list,struct INLcpMsgSet_list data);
void list_appendex(LIST* list,char *data, uint8 datalen);

void list_remove(LIST* list,int list_id);
int list_size(LIST* list);


void list_begin(LIST* list);
struct INLcpMsgSet_list* list_next(LIST* list);
struct INLcpMsgSet_list* list_prev(LIST* list);
struct INLcpMsgSet_list* list_current(LIST* list);
int list_end(LIST* list);
//LIST list;
#endif
