#ifndef _BT_H_
#define _BT_H_
#include<sys/types.h>
#include "INLcpMsg.h"
#include "zigbeemsg.h"

#define TESTINTERVAL 30
#define MAXNEIGHBORCOUNT 12
struct zb_tree{
   uint8 macaddr[8];
   uint8 nwkaddr[2];
   int live_val;
   long testtime;
   int neighborcount;
   NODE_DESC neighbor[MAXNEIGHBORCOUNT];
};

typedef struct BsTreeNode{
   struct zb_tree zb_data;
   struct BsTreeNode *left;
   struct BsTreeNode *right;
}BSTREE_NODE;

typedef struct BsTree{
  BSTREE_NODE *root;
  size_t size;
}BSTREE;



//init tree
void bstree_init(BSTREE* bstree);
//delete tree
void bstree_deinit(BSTREE* bstree);

//data insert and clear
void  bstree_insert(BSTREE* bstree,void* data);
int   bstree_erase(BSTREE* bstree,void* data);
int   bstree_check(BSTREE* bstree,void* data,void* list_data);
void  bstree_clear(BSTREE* bstree);
int   bstree_update(BSTREE* bstree,void* data);
//live
int bstree_exist(BSTREE* bstree,void* data);
void bstree_travel(BSTREE* bstree);
int bstree_size(BSTREE* bstree);
int bstree_height(BSTREE* bstree);
int bstree_live(BSTREE* bstree,void* data,void*  list_data);
//send Zigbee does off_line ,Between bt.c and tcp.c.
void zigbee_off_line(struct zb_tree zb_data,uint8 check_code);
void updateneighbor(char *mac,char *nodebuf);
BSTREE_NODE* gettestneighborNode(BSTREE_NODE* root, long currentime);
#endif
