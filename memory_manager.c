#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

int m, n;
int total,fault;

void hit_log(int vpi, int pfi)
{
    //fprintf(fp,"Hit, %d=>%d\n", vpi,pfi);
    printf("Hit, %d=>%d\n", vpi,pfi);
}

void miss_log(int pfi,int evpi,int des,int vpi,int src)
{
    //fprintf(fp,"Miss, %d, %d>>%d, %d<<%d\n",pfi,evpi,des,vpi,src);
    printf("Miss, %d, %d>>%d, %d<<%d\n",pfi,evpi,des,vpi,src);
}

struct entry
{
    int pdi;
    bool in_use;
    bool present;
    bool ref;
    bool dirty;
};

struct entry* pte;

struct pf
{
    bool is_free;
    int vpi;
};

struct pf* pfmem;

int free_mem() //return a pfi
{
    int i,r=-1;
    for(i=0; i<n; i++)
    {
        if(pfmem[i].is_free==true)
        {
            r=i;
            break;
        }
    }
    return r;
}

struct block
{
    bool is_free;
    int vpi;
};

struct block* disk;

int free_block() //return a pdi
{
    int i,r=-1;
    for(i=0; i<m; i++)
    {
        if(disk[i].is_free==true)
        {
            r=i;
            break;
        }
    }
    return r;
}

struct fifo_e
{

    int vpi;
    struct fifo_e* next;
};

struct fifo_list
{
    struct fifo_e* head;
    struct fifo_e* tail;
    int size;
    int cnt;
};

struct fifo_list f_list;

void fifo_push(struct fifo_list* list,int vpi) //push via vpi
{
    struct fifo_e* t=malloc(sizeof(struct fifo_e));
    t->next=NULL;
    t->vpi=vpi;

    if(list->head==list->tail&&list->head==NULL)//0
    {
        list->head=list->tail=t;
        t->next=NULL;
        return;
    }
    else if(list->head==list->tail&&list->head!=NULL)//1
    {
        list->head=t;
        t->next=list->tail;
        return;
    }
    else//2
    {
        t->next=list->head;
        list->head=t;
        return;
    }
}

int fifo_pop(struct fifo_list* list) //pop a vpi
{
    struct fifo_e* t;
    int r;
    //rintf("0\n");
    if(list->head==list->tail&&list->head==NULL)//0
    {
        //printf("0\n");
        return -1;
    }
    else if(list->head==list->tail&&list->head!=NULL)//1
    {
        //printf("1\n");
        t=list->tail;
        t->next=NULL;
        list->head=list->tail=NULL;
        r=t->vpi;
        free(t);
        return r;
    }
    else if(list->head!=list->tail&&list->head->next==list->tail)//2
    {
        //printf("2\n");
        t=list->tail;
        t->next=NULL;
        list->tail=list->head;
        list->head->next=NULL;
        r=t->vpi;
        free(t);
        return r;
    }
    else//3
    {
        //printf("3\n");
        t=list->head;
        while(t->next->next!=NULL)
        {
            t=t->next;
        }
        r=t->next->vpi;
        free(t->next);
        t->next=NULL;
        list->tail=t;
        return r;
    }
}

void fifo(char op,int vpi)
{

    if(pte[vpi].present==false) //miss
    {
        int freef=free_mem();
        if(freef!=-1) //remain
        {
            if(pte[vpi].in_use==false) //first
            {
                pte[vpi].in_use=true;
                pte[vpi].present=true;
                pte[vpi].pdi=freef;

                pfmem[freef].is_free=false;

                fifo_push(&f_list,vpi);

                miss_log(pte[vpi].pdi,-1,-1,vpi,-1);
            }
            else  //not first
            {
                printf("impossible 1\n");
            }
        }
        else  //full
        {
            int repl=fifo_pop(&f_list);
            int place=free_block();

            if(pte[vpi].in_use==false) //first
            {
                disk[place].is_free=false;
                disk[place].vpi=repl;

                pte[repl].present=false;
                pfmem[pte[repl].pdi].is_free=true;
                pte[repl].pdi=place;

                pte[vpi].in_use=true;
                pte[vpi].present=true;
                int freef=free_mem();
                pte[vpi].pdi=freef;
                pfmem[freef].is_free=false;

                fifo_push(&f_list,vpi);

                miss_log(pte[vpi].pdi,repl,pte[repl].pdi,vpi,-1);
            }
            else  //not first
            {
                disk[place].is_free=false;
                disk[place].vpi=repl;

                pte[repl].present=false;
                pfmem[pte[repl].pdi].is_free=true;
                pte[repl].pdi=place;

                int at_b=pte[vpi].pdi;
                disk[at_b].is_free=true;
                disk[at_b].vpi=-1;

                pte[vpi].present=true;
                int freef=free_mem();
                pte[vpi].pdi=freef;

                pfmem[freef].is_free=false;

                fifo_push(&f_list,vpi);

                miss_log(pte[vpi].pdi,repl,place,vpi,at_b);
            }
        }
        fault++;
    }
    else  //hit
    {
        hit_log(vpi,pte[vpi].pdi);
    }

}

int cir_p=0;

int esca_repl()
{
    int i,cnt;

    cir_p++;
    if(cir_p==n)
        cir_p=0;
    //(a) Cycle through the frame looking for (0, 0)
    //If one is found, use that page
    for(i=cir_p,cnt=0; cnt<n; i++,cnt++)
    {
        if(i==n)
            i=0;

        if(pfmem[i].is_free==true)
            continue;

        if(!pte[pfmem[i].vpi].ref&&!pte[pfmem[i].vpi].dirty)
        {
            return pfmem[i].vpi;
        }
    }
    //(b) Cycle through the frame looking for (0, 1)
    //Set the reference bit to 0 for all frames bypassed
    for(i=cir_p,cnt=0; cnt<n; i++,cnt++)
    {
        if(i==n)
            i=0;

        if(pfmem[i].is_free==true)
            continue;

        if(!pte[pfmem[i].vpi].ref&&pte[pfmem[i].vpi].dirty)
        {
            return pfmem[i].vpi;
        }
        else
        {
            pte[pfmem[i].vpi].ref=0;
        }
    }
    // (c) If step (b) failed, all reference bits will now be zero
    //and repetition of steps (a) and (b) are guaranteed to find
    //a frame for replacement

    //(a) Cycle through the frame looking for (0, 0)
    //If one is found, use that page
    for(i=cir_p,cnt=0; cnt<n; i++,cnt++)
    {
        if(i==n)
            i=0;

        if(pfmem[i].is_free==true)
            continue;

        if(!pte[pfmem[i].vpi].ref&&!pte[pfmem[i].vpi].dirty)
        {
            return pfmem[i].vpi;
        }
    }
    //(b) Cycle through the frame looking for (0, 1)
    //Set the reference bit to 0 for all frames bypassed
    for(i=cir_p,cnt=0; cnt<n; i++,cnt++)
    {
        if(i==n)
            i=0;

        if(pfmem[i].is_free==true)
            continue;

        if(!pte[pfmem[i].vpi].ref&&pte[pfmem[i].vpi].dirty)
        {
            return pfmem[i].vpi;
        }
        else
        {
            pte[pfmem[i].vpi].ref=0;
        }
    }

    printf("impossible 3\n");
}

void esca(char op,int vpi)
{
    if(op=='W')
    {
        pte[vpi].dirty=1;//set write
    }
    pte[vpi].ref=1;//used

    if(pte[vpi].present==false) //miss
    {
        int freef=free_mem();
        if(freef!=-1) //remain
        {
            if(pte[vpi].in_use==false) //first
            {
                pte[vpi].in_use=true;
                pte[vpi].present=true;
                pte[vpi].pdi=freef;

                pte[vpi].ref=true;//set swap in
                cir_p=freef;

                pfmem[freef].is_free=false;
                pfmem[freef].vpi=vpi;

                miss_log(pte[vpi].pdi,-1,-1,vpi,-1);
            }
            else  //not first
            {
                printf("impossible 2\n");
            }
        }
        else  //full
        {

            int repl=esca_repl();
            int place=free_block();

            if(pte[vpi].in_use==false) //first
            {
                disk[place].is_free=false;
                disk[place].vpi=repl;

                pte[repl].present=false;
                pte[repl].dirty=0;
                pte[repl].ref=0;
                pfmem[pte[repl].pdi].is_free=true;
                pfmem[pte[repl].pdi].vpi=-1;
                pte[repl].pdi=place;

                pte[vpi].in_use=true;
                pte[vpi].present=true;
                freef=free_mem();
                pte[vpi].pdi=freef;
                pte[vpi].ref=true;//set swap in
                pfmem[freef].is_free=false;
                pfmem[freef].vpi=vpi;
                cir_p=freef;

                miss_log(pte[vpi].pdi,repl,place,vpi,-1);
            }
            else  //not first
            {
                disk[place].is_free=false;
                disk[place].vpi=repl;

                pte[repl].present=false;
                pte[repl].dirty=0;
                pte[repl].ref=0;
                pfmem[pte[repl].pdi].is_free=true;
                pfmem[pte[repl].pdi].vpi=-1;
                pte[repl].pdi=place;

                int at_b=pte[vpi].pdi;
                disk[at_b].is_free=true;
                disk[at_b].vpi=-1;

                pte[vpi].present=true;
                freef=free_mem();
                pte[vpi].ref=true;//set swap in
                pte[vpi].pdi=freef;
                cir_p=freef;

                pfmem[freef].is_free=false;
                pfmem[freef].vpi=vpi;

                miss_log(pte[vpi].pdi,repl,place,vpi,at_b);
            }
        }
        fault++;
    }
    else  //hit
    {
        hit_log(vpi,pte[vpi].pdi);
    }
}

struct fifo_list a_list,i_list;

bool test_hit(char ch,int vpi)
{
    struct fifo_list* temp;
    if(ch=='a')
    {
        temp=&a_list;
    }
    else if(ch=='i')
    {
        temp=&i_list;
    }

    struct fifo_e* e=temp->head;
    while(e!=NULL)
    {
        if(e->vpi==vpi)
        {
            return true;
        }
        e=e->next;
    }

    return false;
}

void slru(char op,int vpi)
{
    if(pte[vpi].present==false) //miss
    {

        int repl=-1,place=-1;
        if(pte[vpi].in_use==false) //first
        {
            pte[vpi].in_use=true;

            struct fifo_e* e=i_list.tail;

            int freef;

            if(i_list.cnt<i_list.size)
            {
                freef=free_mem();
            }
            else  //need swap out
            {

                while(pte[e->vpi].ref!=0)
                {
                    pte[e->vpi].ref=0;
                    fifo_push(&i_list,fifo_pop(&i_list));
                    e=i_list.tail;
                }

                //swap out...
                repl=fifo_pop(&i_list);
                place=free_block();

                disk[place].is_free=false;
                disk[place].vpi=repl;
                pte[repl].present=false;
                pfmem[pte[repl].pdi].is_free=true;

                freef=pte[repl].pdi;

                pte[repl].pdi=place;
                i_list.cnt--;
            }

            //swap in ...
            pte[vpi].present=true;
            pte[vpi].pdi=freef;
            pte[vpi].ref=1;//set swap in
            pfmem[freef].is_free=false;
            //swap to inactive
            fifo_push(&i_list,vpi);
            i_list.cnt++;

            miss_log(pte[vpi].pdi,repl,place,vpi,-1);

        }
        else  //not first
        {
            struct fifo_e* e=i_list.tail;

            int freef;

            if(i_list.cnt<i_list.size)
            {
                freef=free_mem();
            }
            else  //need swap out
            {
                while(pte[e->vpi].ref!=0)
                {
                    pte[e->vpi].ref=0;
                    fifo_push(&i_list,fifo_pop(&i_list));
                    e=i_list.tail;
                }

                //swap out...
                repl=fifo_pop(&i_list);
                place=free_block();
                disk[place].is_free=false;
                disk[place].vpi=repl;
                pte[repl].present=false;
                pfmem[pte[repl].pdi].is_free=true;

                freef=pte[repl].pdi;

                pte[repl].pdi=place;
                i_list.cnt--;
            }


            int at_b=pte[vpi].pdi;
            disk[at_b].is_free=true;
            disk[at_b].vpi=-1;
            //swap in ...
            pte[vpi].present=true;
            pte[vpi].pdi=freef;
            pte[vpi].ref=1;//set swap in
            pfmem[freef].is_free=false;
            //swap to inactive
            fifo_push(&i_list,vpi);
            i_list.cnt++;

            miss_log(pte[vpi].pdi,repl,place,vpi,at_b);
        }

        fault++;
    }
    else  //hit
    {
        if(test_hit('a',vpi)) //in active
        {
            //printf("hit in a\n");

            struct fifo_e* e=a_list.head;
            struct fifo_e* pre=NULL;
            while(1)
            {
                if(e->vpi==vpi)
                {
                    break;
                }
                pre=e;
                e=e->next;
            }
            //if the page has reference bit = 0
            //set reference bit and move the page to active list head
            //if the page has reference bit = 1
            //move the page to active list head
            if(pte[e->vpi].ref==0)
            {
                pte[e->vpi].ref=1;
            }
            else if(pte[e->vpi].ref==1)
            {

            }

            if(e!=a_list.head&&e!=a_list.tail) //middle
            {
                pre->next=e->next;
                fifo_push(&a_list,e->vpi);
            }
            else if(e==a_list.head)
            {

            }
            else if(e==a_list.tail)
            {
                fifo_push(&a_list,fifo_pop(&a_list));
            }
        }
        else if(test_hit('i',vpi))  //in inactive
        {
            //printf("hit in i\n");

            struct fifo_e* e=i_list.head;
            struct fifo_e* pre=NULL;
            while(1)
            {
                if(e->vpi==vpi)
                {
                    break;
                }
                pre=e;
                e=e->next;
            }
            // if the page has reference bit = 0
            // set reference bit and move the page to inactive list head
            // if the page has reference bit = 1
            // clear reference bit and move the page to active list head
            if(pte[e->vpi].ref==0)
            {
                pte[e->vpi].ref=1;

                if(e!=i_list.head&&e!=i_list.tail) //middle
                {
                    pre->next=e->next;
                    fifo_push(&i_list,e->vpi);
                }
                else if(e==i_list.head)
                {

                }
                else if(e==i_list.tail)
                {
                    fifo_push(&i_list,fifo_pop(&i_list));
                }
            }
            else if(pte[e->vpi].ref==1)
            {
                pte[e->vpi].ref=0;

                // Replacement in active list
                // (occurs when the page in inactive list with reference bit = 1 page hit
                // but there is no free space in active list)
                if(a_list.size!=0)
                {


                    if(a_list.cnt==a_list.size)
                    {
                        //printf("refill\n");
                        // Search the tail of the active list for evicted page
                        // if the page has reference bit = 0
                        // refill the page to inactive list head
                        // if the page has reference bit = 1
                        // clear reference bit and move the page to active list head
                        // search the tail of the active list for another evicted page
                        struct fifo_e* t=a_list.tail;
                        while(pte[t->vpi].ref!=0)
                        {
                            pte[t->vpi].ref=0;
                            fifo_push(&a_list,fifo_pop(&a_list));
                        }

                        int repl=fifo_pop(&a_list);
                        fifo_push(&i_list,repl);
                        a_list.cnt--;
                        i_list.cnt++;
                    }

                    if(e!=i_list.head&&e!=i_list.tail) //middle
                    {
                        pre->next=e->next;
                        fifo_push(&a_list,e->vpi);
                    }
                    else if(e==i_list.head)
                    {

                        i_list.head=i_list.head->next;
                        if(i_list.head==NULL)
                            i_list.tail=NULL;
                        fifo_push(&a_list,e->vpi);
                    }
                    else if(e==a_list.tail)
                    {
                        fifo_push(&a_list,fifo_pop(&i_list));
                    }
                    a_list.cnt++;
                    i_list.cnt--;
                }
            }



        }

        hit_log(vpi,pte[vpi].pdi);
    }
}

void loglist()
{
    struct fifo_e* e=i_list.head;
    printf("i list\n");
    while(e!=NULL)
    {
        printf("%d ",e->vpi);
        e=e->next;
    }
    e=a_list.head;
    printf("\na list\n");
    while(e!=NULL)
    {
        printf("%d ",e->vpi);
        e=e->next;
    }
    printf("\n");

}

int main(int argc, char *argv[])
{
    //printf("%d\n",argc);

    //FILE *infile,*outfile;
    //infile = fopen(argv[2], "r");
    //outfile = fopen(argv[4], "w");
    char policy[10],temp[20];


    scanf("%s %s\n",&temp,&policy);
    scanf("Number of Virtual Page: %d\n",&m);
    pte=malloc(m*sizeof(struct entry));
    scanf("Number of Physical Frame: %d\n",&n);
    pfmem=malloc(n*sizeof(struct pf));
    scanf("%s\n",&temp);
    disk=malloc(m*sizeof(struct block));

    a_list.size=(int)floor(n/2.0);
    a_list.cnt=0;
    i_list.size=(int)ceil(n/2.0);
    i_list.cnt=0;


    //printf("%d %d %d %d\n",m,n,i_list.size,a_list.size);

    int i;
    for(i=0; i<m; i++)
    {
        pte[i].pdi=-1;
        pte[i].in_use=false;
        pte[i].present=false;
        pte[i].ref=false;
        pte[i].dirty=false;

        disk[i].is_free=true;
        disk[i].vpi=-1;
    }
    for(i=0; i<n; i++)
    {
        pfmem[i].is_free=true;
        pfmem[i].vpi=-1;
    }

    while(1)
    {
        char op[6];
        int target;
        if(scanf("%s %d\n",&op,&target)!=2)
            break;
        //...
        //printf("%d\n",target);
        if(policy[0]=='F'&&policy[1]=='I'&&policy[2]=='F'&&policy[3]=='O')
        {
            fifo(op[0],target);
        }
        else if(policy[0]=='S'&&policy[1]=='L'&&policy[2]=='R'&&policy[3]=='U')
        {
            //loglist();
            slru(op[0],target);
        }
        else if(policy[0]=='E'&&policy[1]=='S'&&policy[2]=='C'&&policy[3]=='A')
        {
            esca(op[0],target);
            //printf("%d\n",cir_p);
        }
        total++;
    }
    printf("Page Fault Rate: %.3f\n",1.0f*fault/total);
    //fclose(infile);
    //fclose(outfile);
}