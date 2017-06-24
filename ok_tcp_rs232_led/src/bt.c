#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include "bt.h"

//data ->struct zb_tree
BSTREE bstree;
static BSTREE_NODE* create_node(void* data)
{
   BSTREE_NODE* node=malloc(sizeof(BSTREE_NODE));
   struct zb_tree *zb_vals=(struct zb_tree *)data;
   node->zb_data.macaddr[0]=zb_vals->macaddr[0];
   node->zb_data.macaddr[1]=zb_vals->macaddr[1];
   node->zb_data.macaddr[2]=zb_vals->macaddr[2];
   node->zb_data.macaddr[3]=zb_vals->macaddr[3];
   node->zb_data.macaddr[4]=zb_vals->macaddr[4];
   node->zb_data.macaddr[5]=zb_vals->macaddr[5];
   node->zb_data.macaddr[6]=zb_vals->macaddr[6];
   node->zb_data.macaddr[7]=zb_vals->macaddr[7];
   node->zb_data.nwkaddr[0]=zb_vals->nwkaddr[0];
   node->zb_data.nwkaddr[1]=zb_vals->nwkaddr[1];
   node->zb_data.live_val=zb_vals->live_val;
   node->left=NULL;
   node->right=NULL;
   node->zb_data.testtime = 0;
   node->zb_data.neighborcount = 0;
   memset(node->zb_data.neighbor, 0 , sizeof(NODE_DESC)*MAXNEIGHBORCOUNT);
    printf("__func=%s,  macaddr= 0x%02X%02X%02X%02X%02X%02X%02X%02X,    val=%d,nwkaddr=(0x%x,0x%x)\n",__func__,node->zb_data.macaddr[0],node->zb_data.macaddr[1],node->zb_data.macaddr[2],node->zb_data.macaddr[3],node->zb_data.macaddr[4],node->zb_data.macaddr[5],node->zb_data.macaddr[6],node->zb_data.macaddr[7],node->zb_data.live_val,node->zb_data.nwkaddr[0],node->zb_data.nwkaddr[1]);
   return node;
}

static void destroy_node(BSTREE_NODE* node)
{
	if(node !=NULL)
	{
	   free(node);
	   node= NULL;
	}
}

void bstree_init(BSTREE* bstree)
{
  bstree->root=NULL;
  bstree->size=0;
}
/* recursion  */
static void clear(BSTREE_NODE** root)
{
   if(*root){
     clear(&(*root)->left);
     clear(&(*root)->right);
     destroy_node(*root);
     *root=NULL;
   }
}

static void insert(BSTREE_NODE* node,BSTREE_NODE** root)
{
   if(! *root) *root=node;
   else if(node){
    if(strcmp(node->zb_data.macaddr,(*root)->zb_data.macaddr)<0)
       insert(node,&(*root)->left);
    else insert(node,&(*root)->right);
   }
}

static BSTREE_NODE **find(void* data,BSTREE_NODE** root)
{
   struct zb_tree zb_vals=*(struct zb_tree *)data;
   
   if(! *root) return root;
   else if(strcmp(zb_vals.macaddr,(*root)->zb_data.macaddr)==0) return root;
   else if(strcmp(zb_vals.macaddr,(*root)->zb_data.macaddr)<0)
        return find((void *)&zb_vals,&(*root)->left);
   else return find((void *)&zb_vals,&(*root)->right);
} //return farther node

static void travel(BSTREE_NODE* root)
{
  if(root){
    travel(root->left); 
    if(((root->zb_data).live_val)>=0)
	{
    	((root->zb_data).live_val)--;
        if(((root->zb_data).live_val)<0)
        { 
             zigbee_off_line(root->zb_data,CHECK_OFF_LINE);
        }
		printf("__func=%s,  macaddr= 0x%02X%02X%02X%02X%02X%02X%02X%02X,    val=%d,nwkaddr=(0x%x,0x%x)\n",__func__,root->zb_data.macaddr[0],root->zb_data.macaddr[1],root->zb_data.macaddr[2],root->zb_data.macaddr[3],root->zb_data.macaddr[4],root->zb_data.macaddr[5],root->zb_data.macaddr[6],root->zb_data.macaddr[7],root->zb_data.live_val,root->zb_data.nwkaddr[0],root->zb_data.nwkaddr[1]);
    }
    travel(root->right);
  }
}
static int height(BSTREE_NODE* root)
{
   if(root){
     int lh=height(root->left);
     int rh=height(root->right);
     return ((lh>rh)?lh:rh)+1;
   }
  return 0;
}
/************************/

int bstree_height(BSTREE* bstree)
{
   return height(bstree->root);
}
void bstree_travel(BSTREE* bstree)
{
   travel(bstree->root);
}

int bstree_erase(BSTREE* bstree,void* data)
{
   BSTREE_NODE** node=find(data,&bstree->root);
   if(*node){
      insert((*node)->left,&(*node)->right);
      BSTREE_NODE* temp=*node;
      *node=(*node)->right;
      destroy_node(temp);
      --bstree->size;
      return ZB_SUCCESS;
   }
   return ZB_FAILED;
}

void bstree_clear(BSTREE* bstree)
{
   bstree_deinit(bstree);
}
void bstree_insert(BSTREE* bstree,void* data)
{
   insert(create_node(data),&bstree->root);
   bstree->size++;
}
int bstree_update(BSTREE* bstree,void* data)
{
   struct zb_tree zb_vals=*(struct zb_tree *)data;
   BSTREE_NODE** node=find(data,&bstree->root);
   if(*node){
        if(((*node)->zb_data.live_val)<0)
              zigbee_off_line((*node)->zb_data,ZB_ON_LINE);

       (*node)->zb_data.live_val=zb_vals.live_val;
       (*node)->zb_data.nwkaddr[0]=zb_vals.nwkaddr[0];
       (*node)->zb_data.nwkaddr[1]=zb_vals.nwkaddr[1];
	   memcpy((char*)((*node)->zb_data.neighbor), (char *)zb_vals.neighbor, sizeof(6*MAXNEIGHBORCOUNT));
	   printf("bstree_update mac=%02x%02x%02x%02x\r\n",zb_vals.macaddr[0], zb_vals.macaddr[1], zb_vals.macaddr[2], zb_vals.macaddr[3] );
        return ZB_SUCCESS;
   }
   return ZB_FAILED;
}
int bstree_exist(BSTREE* bstree,void* data)
{
   return *find(data,&bstree->root) !=NULL;
}
int bstree_size(BSTREE* bstree)
{
   return bstree->size;
}

void bstree_deinit(BSTREE* bstree)
{
  clear(&bstree->root);
  bstree->size=0;
}

int bstree_live(BSTREE* bstree,void* data,void*  list_data)
{
   struct INLcpMsgSet_list  *rs232_save_msg_list=(struct INLcpMsgSet_list *)list_data;
   BSTREE_NODE** node=find(data,&bstree->root);
//    struct zb_tree *zb=(struct zb_tree *)data;
//    printf("__func__=%s ++++++++++++++++++->bstree_insert(),  macaddr= 0x%x ,macaddr[1]=0x%x,macaddr[2]=0x%x,macaddr[3]=0x%x,macaddr[4]=0x%x,macaddr[5]=0x%x,macaddr[6]=0x%x,macaddr[7]=0x%x,    live_val=%d\n",__func__,zb->macaddr[0],zb->macaddr[1],zb->macaddr[2],zb->macaddr[3],zb->macaddr[4],zb->macaddr[5],zb->macaddr[6],zb->macaddr[7],zb->live_val);
   if(*node){
	if(((*node)->zb_data).live_val>=0){
   	   rs232_save_msg_list->nwkaddr[0]=((*node)->zb_data).nwkaddr[0];
   	   rs232_save_msg_list->nwkaddr[1]=((*node)->zb_data).nwkaddr[1];
	   printf("=========================================live_val=%d,list=(0x%x,0x%x),tree=(0x%x,0x%x)\n",((*node)->zb_data).live_val,rs232_save_msg_list->nwkaddr[0],rs232_save_msg_list->nwkaddr[1],((*node)->zb_data).nwkaddr[0],((*node)->zb_data).nwkaddr[1]);
 	   return ZB_SUCCESS;
	}
 	return  ZB_OFF_LINE;
   }
  return  ZB_NOT_EXIST;
}

BSTREE_NODE* travelex(BSTREE_NODE* root, long currentime)
{
	BSTREE_NODE* tempnode = NULL;
	long interval =0;
	if(root)
  	{
	    tempnode= travelex(root->left, currentime); 
		if(tempnode!= NULL)
		{
			return tempnode;
		}
		interval = currentime - root->zb_data.testtime;
	    if(interval >= TESTINTERVAL)
		{ 
			root->zb_data.testtime = currentime;
	    	return root;
	    }
	    printf("__func=%s,  macaddr=  0x%02X%02X%02X%02X%02X%02X%02X%02X,    val=%d,nwkaddr=(0x%x,0x%x)\n",__func__,root->zb_data.macaddr[0],root->zb_data.macaddr[1],root->zb_data.macaddr[2],root->zb_data.macaddr[3],root->zb_data.macaddr[4],root->zb_data.macaddr[5],root->zb_data.macaddr[6],root->zb_data.macaddr[7],root->zb_data.live_val,root->zb_data.nwkaddr[0],root->zb_data.nwkaddr[1]);

	    tempnode= travelex(root->right, currentime); 
		if(tempnode!= NULL)
		{
			return tempnode;	
		}
	}
	return NULL;
}
BSTREE_NODE* gettestneighborNode(BSTREE *bstree, long currentime)
{
	
	BSTREE_NODE * tempnode = NULL;
	tempnode=travelex(bstree->root, currentime);
	return tempnode;
}

void updateneighbor(char *mac,char *nodebuf, int nodebuflen)
{
	BSTREE_NODE * tempnode = NULL;
	tempnode=travelex(bstree.root, 0);
	if (tempnode != NULL)
	{
		memcpy((char*)tempnode->zb_data.neighbor,nodebuf, nodebuflen);
	}
	return ;	

}
void travelprint(BSTREE_NODE* root )
{
	BSTREE_NODE* tempnode = NULL;
	long interval =0;
	if(root)
  	{
	    travelprint(root->left); 


	    printf("__func=%s,  macaddr=  0x%02X%02X%02X%02X%02X%02X%02X%02X, live_val=%d,nwkaddr=(0x%x,0x%x)\n",__func__,root->zb_data.macaddr[0],root->zb_data.macaddr[1],root->zb_data.macaddr[2],root->zb_data.macaddr[3],root->zb_data.macaddr[4],root->zb_data.macaddr[5],root->zb_data.macaddr[6],root->zb_data.macaddr[7],root->zb_data.live_val,root->zb_data.nwkaddr[0],root->zb_data.nwkaddr[1]);
		int i;
		for(i=0; i<MAXNEIGHBORCOUNT; i++)
		{
			printf(" neighbor[%d]=0x%02X%02X%02X%02X% rssi=%d, fail=%d\n",i,root->zb_data.neighbor[i].uiNode_Addr[0],root->zb_data.neighbor[i].uiNode_Addr[1],root->zb_data.neighbor[i].uiNode_Addr[2],root->zb_data.neighbor[i].uiNode_Addr[3],root->zb_data.neighbor[i].cRSSI,root->zb_data.neighbor[i].ucFailure_Or_Age);
		}
	    travelprint(root->right); 
	}
}

void bstree_print(BSTREE* bstree)
{
   travelprint(bstree->root);
}

