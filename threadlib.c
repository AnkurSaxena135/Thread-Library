#include"mythread.h"
#include<stdio.h>
#include<ucontext.h>
#include<stdlib.h>
#include<string.h>
#define MEM 8192

typedef void *MyThread;
typedef void *MySemaphore;

typedef struct tInfo
{
	int tID;
	int children;
	int parentID;
	int JoinAll;
	int Join;
	ucontext_t *tCon;
	struct tInfo *link;
}tInfo;

typedef struct sInfo
{
	int sVal;
	struct	tInfo *sfront;
	struct	tInfo *srear;
}sInfo;


tInfo *ready_f,*ready_r=NULL;
ucontext_t *mainCon;
tInfo *block_f,*block_r=NULL;
tInfo *current=NULL;
int tID=1;


//Initialize all the queues to NULL
/*void queue_init_all(void)
{
 	ready_f=ready_r=block_f=block_r= current=next=NULL;
}
*/
int isReadyEmpty(void)	//empty=0; filled=1
{
	if((ready_r==NULL) && (ready_f==NULL))
	return 0;
	else 
	return 1;
}

int isBlockEmpty(void)	//empty=0; filled=1
{
	if((block_r==NULL) && (block_f==NULL))
	return 0;
	else 
	return 1;
}

void isplayReady() 
{
	tInfo *temp = block_f;
	while (temp != NULL)
	{
		printf ("block queue %d -> ", temp -> tID);
		temp = temp -> link;
	}
	printf ("NULL\n");	
}

int semEmpty(sInfo *sem)
{
	if( sem->sfront==NULL && sem->srear==NULL)
	return 1;
	else
	return 0;
}
void assignToCurrent(tInfo *info)
{
	current=info;	
}


tInfo* enqueueSem(sInfo *sem,tInfo *info)
{
	if (sem->sfront == NULL)
	{
		sem->sfront = sem->srear=info;	
		sem->srear -> link = NULL;
	}
	else
	{
		sem->srear -> link = info;
		sem->srear = info;
		sem->srear -> link = NULL;
	}
//	printf("\n%d was enqueued",ready_r->tID);
	return sem->srear;
}

tInfo* dequeueSem(sInfo* sem)
{
	tInfo *info;
	if(sem->sfront != NULL)
	{
		info=sem->sfront;
		sem->sfront=sem->sfront->link;
		if (sem->sfront == NULL)
		sem->srear=NULL;
		return info;
	}
//	printf("%d was dequeued\n",info->tID);
	
}

tInfo* enqueueReady(tInfo *info)
{
	if (ready_r == NULL)
	{
		ready_f = ready_r = info;	
		ready_r -> link = NULL;
	}
	else
	{
		ready_r -> link = info;
		ready_r = info;
		ready_r -> link = NULL;
	}
//	printf("\n%d was enqueued",ready_r->tID);
	return ready_r;
}


tInfo* dequeueReady(void)
{
	tInfo *info;
	if(ready_f != NULL)
	{
		info=ready_f;
		ready_f=ready_f->link;
		if (ready_f == NULL)
		ready_r=NULL;
		return info;
	}
//	printf("%d was dequeued\n",info->tID);
	
}

void enqueueBlock(tInfo *info)
{
	if (block_r == NULL)
	{
		block_f = block_r = info;	
		block_r -> link = NULL;
	}
	else
	{
		block_r -> link = info;
		block_r = info;
		block_r -> link = NULL;
	}
//	isplayReady();
}


tInfo* dequeueBlock(void)
{
	tInfo *info;
	if(block_f != NULL)
	{
		info=block_f;
		block_f=block_f->link;
		if (block_f == NULL)
		block_r=NULL;
		return info;
	}
	
}

void childrenUpdate(int flag)
{

	if(flag==1)//from parent while creating thread
	{
		current->children++;
//		printf("create child no:%d for thread %d\n",tID,current->tID);
	}
	if(flag==2)//from child while exiting the thread
	{
//		printf("it entered\n");
		tInfo *info=ready_f;
//		printf("child %d entered ready queue\n",current->tID);
		while(1)	
		{	
//			printf("child %d entered ready queue ",current->tID);
			if(info==NULL)
			break;
			if(info->tID==current->parentID)
			{
//				printf(" changed %d parent from count %d",info->tID,info->children);
				info->children--;
//				printf(" to %d\n",info->children);
				break;
			}
			else 
			{
//				printf(" checked parent %d\n",info->tID);
				info=info->link;
			
			}
		}
		info=block_f;
//		printf("%d",block_f->tID);
		while(1)	
		{	
//			printf("child %d entered block queue\n",current->tID);
			if(info==NULL)
			break;
			if(info->tID==current->parentID)
			{
//				printf(" changed %d parent from count %d",info->tID,info->children);
				info->children--;
//				printf(" to %d\n",info->children);
				break;
			}
			else 
			{
				info=info->link;
			}
	
		}
	}
}


void unblockParent(void)
{
//	isplayReady();
//	printf("%d entered with parentID %d to unblock\n",current->tID,current->parentID);
	tInfo *info=block_f;
	while(1)	
	{	
//		isplayReady();
		if(info==NULL)
		{
			break;
		}
		if(info->JoinAll==1 && info->tID==current->parentID && info->children==0)
		{
//			printf("\n%d child found %d blocked parent",current->tID,info->tID);
//			printf("\n%d child is the last executing child of %d",current->tID,info->parentID);
			info->JoinAll=-1;
			enqueueReady(info);
			dequeueBlock();
//				printf("parentid=%d, children=%d\n",info->tID,info->children);
//			info=info->link;
//			if(info==NULL)
//			printf("block empty");
			break;
			
//		}		info->children--;	
		}
		else 
		{	
//			printf("\n%d child tried to unblock %d",current->tID,info->tID);
			info=info->link;
		}
		
	}

}

void unjoinParent(void)
{
	tInfo *info=block_f;
//	printf("child %d entered to unjoin\n",current->tID);
	while(info!=NULL)	
	{	
		if(info==NULL)
		break;
		if(info->Join==current->tID)
		{
//			printf("\n%d was joined on %d",info->tID,current->tID);
			info->Join=-1;
			enqueueReady(info);
			dequeueBlock();
//			printf("parentid=%d, children=%d\n",info->tID,info->children);
//			info=info->link;
		}
//				info->children--;
		else 
		{
//			printf("\n %d check to unjoin %d\n",current->tID,info->tID);
			info=info->link;
		}
	}
}	
	



//Unix process init
void MyThreadInit(void(*start_funct)(void *), void *args)
{
	mainCon=malloc(sizeof(ucontext_t));
	getcontext(mainCon);
	
	ucontext_t *a=malloc(sizeof(ucontext_t));
	getcontext(a);
	a->uc_link=0;
	a->uc_stack.ss_sp=malloc(MEM);
 	a->uc_stack.ss_size=MEM;
 	a->uc_stack.ss_flags=0;
	makecontext(a,(void*)start_funct,1,args);
	
	tInfo *info=malloc(sizeof(tInfo));
	info->tID=tID;
	info->tCon=a;
	info->parentID=tID-1;
	info->link==NULL;
//	printf ("Created init thread %d\n", tID);
	assignToCurrent(info);	
//	printf("current->tID=%d\ncurrent->parentID->%d\n",current->tID,current->parentID);	
	swapcontext(mainCon,a);
}


MyThread MyThreadCreate(void(*start_funct)(void *), void *args)
{
	tID++;
	childrenUpdate(1);
	//Assigning local ucontext_t to statr_funct
//	printf("entered thread crreate\n");
//	printf("current->tID=%d\ncurrent->parentID->%d\n",current->tID,current->parentID);
	ucontext_t *a=malloc(sizeof(ucontext_t));	//a=local ucontext_t
	getcontext(a);
//	printf("getcontext done\n");
	a->uc_stack.ss_sp=malloc(MEM);
	a->uc_stack.ss_size=MEM;
 	a->uc_stack.ss_flags=0;
	makecontext(a,(void*)start_funct,1,args);

//	current->parentID+=1;
	tInfo *info=malloc(sizeof(tInfo));		//info= local tInfo
	info->tID=tID;
	info->JoinAll=0;
	info->Join=0;
	info->tCon=a;
	info->parentID=current->tID;
	info->link==NULL;
	
//	printf("After thread creadtion: currentID=%d\n currentparentID=%d\n",info->tID,info->parentID);
	return enqueueReady(info);
	
}

void MyThreadYield(void)
{
	if(isReadyEmpty())
	{
//		printf ("entered yeild\n");
		ucontext_t *a=malloc(sizeof(ucontext_t));//a=local ucontext_t
		getcontext(a);
		
		tInfo *info=malloc(sizeof(tInfo));
		info=current;
		info->tCon=a;
		info->link=NULL;
//		printf ("%d was the yielding thread\n", info -> tID);
		enqueueReady(info);		//queue yielding thread
		info=dequeueReady();		//dequeue ready 
//		printf("%d fetched from queue\n", info -> tID);
		assignToCurrent(info);
		swapcontext(a,info->tCon);	//jump to current thread		
	}
}


int MyThreadJoin(MyThread thread)
{
	tInfo *temp=malloc(sizeof(tInfo));
	temp=(tInfo *)thread;	
//	printf("%d is joined on %d",current->tID,temp->tID);

	ucontext_t *a=malloc(sizeof(ucontext_t));
	getcontext(a);
	tInfo *info=malloc(sizeof(tInfo));
	info=current;
	info->tCon=a;
	info->Join=temp->tID;
	info->link=NULL;
	enqueueBlock(info);
	info=dequeueReady();
	assignToCurrent(info);
//	printf("exiting join");	
	swapcontext(a,info->tCon);	
}

void MyThreadJoinAll(void)
{	
//	printf("\n%d parentwith %d children was joinedALL\n",current->tID,current->children);
	if(current->children !=0)
	{
		ucontext_t *a=malloc(sizeof(ucontext_t));
		getcontext(a);
	
		tInfo *info=malloc(sizeof(tInfo));
		info=current;
		info->tCon=a;
		info->JoinAll=1;
		info->link=NULL;
//		printf("Joined parent has:-%d children\n",info->children);
		enqueueBlock(info);
//		printf("joinall value=%d",info->JoinAll);
		info=dequeueReady();
		assignToCurrent(info);
//		printf("%d child executed after joinAll\n",info->tID);
		swapcontext(a,info->tCon);
		
	}
	if(current->children ==0)
	{
		ucontext_t *a=malloc(sizeof(ucontext_t));
		getcontext(a);
	
		tInfo *info=malloc(sizeof(tInfo));
		info=current;
		info->tCon=a;
		info->link=NULL;
		enqueueReady(info);
		info=dequeueReady();
		assignToCurrent(info);
		swapcontext(a,info->tCon);
	}	
}

void MyThreadExit(void)
{
	if(!isReadyEmpty())
	{
		setcontext(mainCon);
	}
	else
	{
//		printf("exit entered\n");
		childrenUpdate(2);
		unjoinParent();
		unblockParent();
		
		ucontext_t *a=malloc(sizeof(ucontext_t));
		getcontext(a);

		tInfo *info=malloc(sizeof(tInfo));
//:wqinfo->tcon=a;
		info=dequeueReady();
		assignToCurrent(info);
		swapcontext(a,current->tCon);
	}
}

MySemaphore MySemaphoreInit(int initialValue)
{
	sInfo *sem=malloc(sizeof(sInfo));
	
	if(initialValue>=0)
	{
		sem->sVal=initialValue;
		sem->sfront=sem->srear=NULL;
	}
	else 
	return NULL;
	
}


void MySemaphoreSignal(MySemaphore sem)
{
	sInfo* a=sem;
	a->sVal++;

	if(a->sVal<=0)
	{
		tInfo *info=malloc(sizeof(tInfo));
		info=dequeueSem(a);
		enqueueReady(info);
	}	
}

void MySemaphoreWait(MySemaphore sem)
{
	sInfo* a=sem;
	a->sVal--;
	
	if(a->sVal<0)
	{
		tInfo* info=malloc(sizeof(tInfo));
		ucontext_t *b=malloc(sizeof(ucontext_t));
		getcontext(b);

		info=current;
		info->tCon=b;
		info->link=NULL;
		enqueueSem(a,info);
		info=dequeueSem(a);
		assignToCurrent(info);
		swapcontext(info->tCon,current->tCon);
	}

}

int MySemaphoreDestroy(MySemaphore sem)
{
	sInfo *a=sem;
	if(semEmpty(a)==1)
	{
		free(a);
		return 0;
	}
	else
	return -1;

}





