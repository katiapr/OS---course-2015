//katiapr
//Katia Prigon ID 322088154 katiapr89@gmail.com
//--------------------------------//
#include "mem_sim.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>//use for flags fd
#include <string.h>
//--------------------//
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
//--------------------------------//
#define VM_SIZE 		4096
#define NUM_OF_PAGES	128
#define PAGE_SIZE		32
#define RAM_SIZE		512
#define FRAME_SIZE 		32
#define NUM_OF_FRAMES 	16
//----
#define EMPTY 			0
#define TAKEN			1
#define NO_FRAME		-1
#define DIRTY			1
#define NOT_DIRTY		0

#define RD 				1
#define RDWR			0

#define TRUE			1
#define FALSE			0

#define FAIL 			-1
#define SUCCESS			0
#define VALID			1
#define INVALID			0

unsigned char RAM[RAM_SIZE];

typedef struct page_descriptor
{
	unsigned int valid; //is page mapped to frame
	unsigned int permission; //READ - 1 or READ-WRITE - 0
	unsigned int dirty; //has the page changed
	unsigned int frame; //number of frame if page mapped
	unsigned int inSwap; //
} page_descriptor_t;
//----List-data struct for LRU algorithm--//
typedef struct Node
{
	int value;//page number
	struct Node* next;
	struct Node* prev;

}Node;
typedef struct List
{
	int numOfElements;
	Node* head;
	Node* tail;
}List;
//----------//
struct sim_database
{
	page_descriptor_t* page_table; //pointer to page table
	char* swapfile_name; //name of swap file
	int swapfile_fd; //swap file fd
	char* executable_name; //name of executable file
	int executable_fd; //executable file fd
	//---------
	int text_size;
	int data_size;
	int bss_size;
	int heap_size;
	int heap_location;
	//----
	int total_num_access;// total number of accesses to memory
	int num_of_hits;
	int num_of_miss;
	int invalid_mem_access;//A number of accesses to address that invalid
	int attemp_WR_page;//attempts to write pages that permission for them

	int bit_map[NUM_OF_FRAMES];
	List* list;
};

//static funcs:
static void freeList(List* l);
static void freeNode(Node* n);
static int load_to_RAM(sim_database_t* sim_db, int page);
static int slot_frame(sim_database_t* sim_db);
static int read_page(sim_database_t* sim_db, int fd, int frame,int page);
static int write_page(sim_database_t* sim_db, int fd, int frame,int page);
//List funcs (LRU)
static int add_LRU(List* l, int value);
static int remove_LRU(List* l);
static int value_exist(List* l, int value);
static void update(List* l,int value);
static void insertion(List* l, int value);

sim_database_t* vm_constructor(char *executable, int text_size, int data_size, int bss_size)
{
	int i = 0;

	if(executable  == NULL)
	{
		perror("file name of executable is NULL");
		return NULL;
	}
	sim_database_t* sim_db = (sim_database_t*)malloc(sizeof(sim_database_t));
	if(sim_db == NULL)
	{
		perror("error at allocation memory");
		return NULL;
	}
	sim_db->executable_fd = open(executable, O_CREAT|O_RDWR, S_IRUSR | S_IWUSR);
	if(sim_db->executable_fd ==  FAIL)
	{
		perror("cannot open executable to WRITE-READ");
		vm_destructor(sim_db);
		return NULL;
	}

	sim_db->page_table = (page_descriptor_t*)malloc(NUM_OF_PAGES*sizeof(page_descriptor_t));
	if(sim_db->page_table == NULL)
	{
		perror("error at allocation memory for page table");
		vm_destructor(sim_db);
		return NULL;
	}
	for(i = 0 ; i < NUM_OF_PAGES;i++)
	{
		sim_db->page_table[i].dirty = EMPTY;
		sim_db->page_table[i].frame = NO_FRAME;//not in frame
		sim_db->page_table[i].inSwap = FALSE;
		if(i < text_size)
			sim_db->page_table[i].permission = RD;//1
		else sim_db->page_table[i].permission = RDWR;//0
		sim_db->page_table[i].valid = INVALID;//off 0
	}
	sim_db->swapfile_name = strdup("swap_file");
	if(sim_db->swapfile_name == NULL)
	{
		perror("error at allocation memory to swap file");
		vm_destructor(sim_db);
		return NULL;
	}
	sim_db->swapfile_fd = open(sim_db->swapfile_name, O_CREAT| O_RDWR,S_IRUSR |S_IWUSR);
	if(sim_db->swapfile_fd == FAIL)
	{
		perror("error at open swap file");
		close(sim_db->executable_fd);
		vm_destructor(sim_db);
		return NULL;
	}
	sim_db->executable_name = strdup(executable);
	if(sim_db->executable_name == NULL)
	{
		perror("error at allocation memory for executable name");
		vm_destructor(sim_db);
		return NULL;
	}
	//-------
	sim_db->bss_size = bss_size;
	sim_db->data_size = data_size;
	sim_db->text_size = text_size;

	sim_db->heap_location = text_size + data_size + bss_size;
	sim_db->heap_size = VM_SIZE - sim_db->heap_location;

	sim_db->total_num_access = 0;
	sim_db->num_of_hits = 0;
	sim_db->num_of_miss = 0;
	sim_db->invalid_mem_access = 0;
	sim_db->attemp_WR_page = 0;


	//init bit_map
	for(i = 0; i < NUM_OF_FRAMES; i++)
		sim_db->bit_map[i] = EMPTY;
	//-----List----//

	sim_db->list = (List*)malloc(sizeof(List));
	if(sim_db->list == NULL)
	{
		perror("cannot creat a list");
		vm_destructor(sim_db);
		return NULL;
	}
	sim_db->list->head = NULL;
	sim_db->list->tail = NULL;
	sim_db->list->numOfElements = 0;
	return sim_db;
}

int vm_load(sim_database_t *sim_db, unsigned short address, unsigned char *p_char)
{
	int page = 0,offset = 0;
	if(address > VM_SIZE -1)
	{
		perror("invalid addres");
		sim_db->invalid_mem_access++;
		return FAIL;
	}

	page  = address >> 5;
	offset = address&31;
	if(sim_db->page_table[page].valid == VALID) // only valid if not nothing
	{
		sim_db->total_num_access++;
		sim_db->num_of_hits++;
		sim_db->bit_map[sim_db->page_table[page].frame] = EMPTY;
		remove_LRU(sim_db->list);
		*p_char = RAM[sim_db->page_table[page].frame*PAGE_SIZE+offset];
		return SUCCESS;
	}
	else
	{
		if(sim_db->page_table[page].inSwap == 1 && page >= sim_db->heap_location)
		{
			perror("cannot read from heap page");
			return FAIL;
		}
		if(load_to_RAM(sim_db,page) == FAIL)
		{
			perror("somehow cannot load to RAM\n");
		}
		sim_db->total_num_access++;
		sim_db->num_of_miss++;
		*p_char = RAM[sim_db->page_table[page].frame*PAGE_SIZE+offset];

	}
	return SUCCESS;
}

int vm_store(sim_database_t *sim_db, unsigned short address, unsigned char value)
{
	int page = 0, offset = 0;
	if(address > VM_SIZE - 1)
	{
		perror("invalid addres");
		sim_db->invalid_mem_access++;
		return FAIL;
	}

	page  = address >> 5;
	offset = address&31;

	if(sim_db->page_table[page].permission == RD)
	{
		perror("cannot write to READ_ONLY page");
		sim_db->invalid_mem_access++;
		return FAIL;

	}
	sim_db->total_num_access++;
	if(sim_db->page_table[page].valid == VALID)
		sim_db->num_of_hits++;
	else
	{
		//load_to_RAM(sim_db,page);
		if(load_to_RAM(sim_db,page)  == FAIL)
		{
			perror("somehow cannot load to RAM\n");
			return FAIL;
		}
		sim_db->num_of_miss++;
	}
	RAM[sim_db->page_table[page].frame*PAGE_SIZE+ offset] = value;
	sim_db->page_table[page].dirty = DIRTY;

	return SUCCESS;
}

void vm_destructor(sim_database_t *sim_db)
{

	free(sim_db->executable_name);
	free(sim_db->page_table);
	free(sim_db->swapfile_name);
	sim_db->executable_name = NULL;
	sim_db->page_table = NULL;
	sim_db->swapfile_name = NULL;
	if(sim_db->executable_fd >= 0)
		close(sim_db->executable_fd);
	if(sim_db->swapfile_fd >= 0)
		close(sim_db->swapfile_fd);
	freeList(sim_db->list);
	free(sim_db);


}
static void freeList(List* l)
{
	//if(l == NULL)
	//return;
	freeNode(l->head);
	free(l);
}
static void freeNode(Node* n)
{
	if(n == NULL)
		return;
	freeNode(n->next);
	free(n->next);
}

void vm_print(sim_database_t* sim_db)
{
	int i = 0,j = 0,frame = 0;
	char temp;
	unsigned char hexa[3],bin[9];

	//main data of sim_database:
	for(i = 0; i < NUM_OF_PAGES; i++)
	{
		fprintf(stdout,"page# %d: ",i);
		if(sim_db->page_table[i].valid == VALID)
			fprintf(stdout,"VALID\t\t");
		else fprintf(stdout,"INVALID\t\t");
		if(sim_db->page_table[i].permission == RDWR)
			fprintf(stdout,"RDWR\t");
		else fprintf(stdout,"RD\t");
		if(sim_db->page_table[i].dirty == DIRTY)
			fprintf(stdout,"DIRTY\t");
		else fprintf(stdout,"NOT_DIRTY\t");
		if(sim_db->page_table[i].frame != -1)
			fprintf(stdout,"frame : %d",sim_db->page_table[i].frame);
		else fprintf(stdout,"NOT_IN_FRAME");
		fprintf(stdout,"\n");
	}
	fprintf(stdout,"\n");
	fprintf(stdout,"swap file name: %s\n", sim_db->swapfile_name);
	fprintf(stdout,"swap file descriptor: %d\n", sim_db->swapfile_fd);
	fprintf(stdout,"executable file name: %s\n", sim_db->executable_name);
	//binary:
	fprintf(stdout,"\n");
	fprintf(stdout,"Binary:\n");
	for(frame = 0;frame < 4; frame ++)
	{
		for(i = 0; i < PAGE_SIZE; i++)
		{
			temp  = RAM[frame*PAGE_SIZE+i];
			for(j = 7;j >= 0; j--, temp  = temp/2)
				bin[j] = temp % 2 + '0';
			bin[8] = '\0';
			fprintf(stdout,"%s\n",bin);
		}
	}
	//hexa:
	fprintf(stdout,"\n");
	fprintf(stdout,"Hexa:\n");
	for(frame = 0;frame < 4; frame ++)
	{
		for(i = 0; i < PAGE_SIZE; i++)
		{
			sprintf(hexa,"%02x",RAM[frame*PAGE_SIZE+i]);//conversion
			fprintf(stdout,"%s ",hexa);
		}
		printf("\n");
	}
	//statistics:
	fprintf(stdout,"\n");
	fprintf(stdout,"statistics:\n");
	fprintf(stdout,"total number of memory access: %d\n",sim_db->total_num_access);
	fprintf(stdout,"hits: %d\n",sim_db->num_of_hits);
	fprintf(stdout,"misses (page faults): %d\n", sim_db->num_of_miss);
	fprintf(stdout,"number of invalid memory access: %d\n", sim_db->invalid_mem_access);
	fprintf(stdout,"number of attempt write to page: %d\n", sim_db->attemp_WR_page);

}

static int load_to_RAM(sim_database_t* sim_db, int page)
{
	int target_frame = slot_frame(sim_db);
	int page_out = 0;

	if(target_frame >= 0)//there is a free frame
	{
		if(add_LRU(sim_db->list,page) == FAIL)
		{
			perror("cannot add the page");
			return FAIL;
		}
		sim_db->bit_map[target_frame] = TAKEN;
		sim_db->page_table[page].valid = VALID;
		sim_db->page_table[page].frame = target_frame;
		if(sim_db->page_table[page].inSwap == 1)//if the page in swap file
		{
			if(read_page(sim_db, sim_db->swapfile_fd, target_frame, page) == FAIL)
			{
				perror("cannot read the page");
				return FAIL;
			}
		}
		else//if the page in exe
		{
			if(read_page(sim_db, sim_db->executable_fd, target_frame, page) == FAIL)
			{
				perror("cannot read the page");
				return FAIL;
			}
		}
		return SUCCESS;
	}
	if(target_frame == FAIL)//everything is full
	{
		page_out = remove_LRU(sim_db->list);
		if(page_out == FAIL)
		{
			perror("cannot remove to get free space");
			return FAIL;
		}

		sim_db->bit_map[sim_db->page_table[page_out].frame] = EMPTY;
		target_frame = sim_db->page_table[page_out].frame;
		sim_db->page_table[page_out].frame = NO_FRAME;
		sim_db->page_table[page_out].valid = INVALID;
	}
	if(sim_db->page_table[page_out].dirty == DIRTY)
	{
		sim_db->page_table[page_out].dirty = NOT_DIRTY;
		sim_db->page_table[page_out].inSwap = 1;
		if(write_page(sim_db, sim_db->swapfile_fd, target_frame, page_out) == FAIL)
		{
			perror("cannot write the page");
			return FAIL;
		}
	}
	
	if(add_LRU(sim_db->list,page) == FAIL)
	{
		perror("cannot add the page");
		return FAIL;
	}
	sim_db->page_table[page].valid = VALID;
	sim_db->page_table[page].frame = target_frame;
	if(sim_db->page_table[page].inSwap == 1)
	{
		if(read_page(sim_db, sim_db->swapfile_fd, target_frame, page) == FAIL)
		{
			perror("cannot read the page");
			return FAIL;
		}
	}
	else//page in exe
	{
		if(read_page(sim_db, sim_db->executable_fd, target_frame, page) == FAIL)
		{
			perror("cannot read the page");
			return FAIL;
		}
	}

	return SUCCESS;
}

static int slot_frame(sim_database_t* sim_db)
{
	int i = 0;
	for(i = 0; i < NUM_OF_FRAMES; i++)
	{
		if(sim_db->bit_map[i] == EMPTY)
		{
			return i;
		}
	}
	return FAIL;
}
static int read_page(sim_database_t* sim_db, int fd, int frame,int page)
{
	if(lseek(fd,page*PAGE_SIZE,SEEK_SET) < 0)
	{
		perror("cannot move the fd pointer in the file");
		return FAIL;
	}
	if(read(fd,&RAM[frame*PAGE_SIZE],PAGE_SIZE) < 0)
	{
		perror("cannot read the file");
		return FAIL;
	}
	return SUCCESS;
}
static int write_page(sim_database_t* sim_db, int fd, int frame,int page)
{
	if(lseek(fd,page*PAGE_SIZE,SEEK_SET) < 0)
	{
		perror("cannot move the fd pointer in the file");
		return FAIL;
	}
	if(write(fd,&RAM[frame*PAGE_SIZE],PAGE_SIZE) < 0)
	{
		perror("cannot read the file");
		return FAIL;
	}
	return SUCCESS;
}
//*** LRU - implementation with linked list
static int add_LRU(List* l, int value)
{
	if(l == NULL)
	{
		perror("the list is not exist");
		return FAIL;
	}
	if(value_exist(l,value))
	{
		update(l,value);
	}
	else
	{
		if(l->numOfElements == NUM_OF_FRAMES)
		{
			remove_LRU(l);
		}
		insertion(l,value);
	}
	return SUCCESS;
}
static int remove_LRU(List* l)
{
	if(l == NULL)
	{
		perror("the list is not exist");
		return 0;
	}
	if(l->numOfElements == 0)
		return 0;
	int temp = l->head->value;
	if(l->numOfElements == 1)
	{
		free(l);
		l->head = NULL;
		l->tail = NULL;
		l->numOfElements = 0;
	}
	else
	{
		Node* rmv = l->head;
		l->head = rmv->next;
		l->head->prev = NULL;
		l->numOfElements--;

		rmv->next = NULL;
		free(rmv);
	}
	return temp;

}
static int value_exist(List* l, int value)
{
	if(l == NULL)
	{
		perror("the list is not exist");
		return FALSE;
	}
	if(l->numOfElements == 0)//empty list
		return FALSE;
	if(l->numOfElements == 1)//only one element
	{
		if( l->head->value == value)
			return TRUE;
		else return FALSE;
	}
	Node* temp = l->head;
	if(l->head == NULL)
	{
		printf("list is empty\n");
		return FALSE;
	}
	while(temp != NULL)
	{
		if(temp->value == value)
			return TRUE;
		temp = temp->next;
	}

	return FALSE;
}
static void update(List* l, int value)
{
	if(l == NULL)
	{
		perror("the list is not exist");
		return;
	}
	if(l->numOfElements == 0 || l->numOfElements == 1)//there is no elements or only one
		return;
	Node* temp = l->head;
	while(temp!=NULL)//check the place of element
	{
		if(temp->value == value)
			break;
		temp = temp->next;
	}
	if(temp->prev == NULL)//first in list -> last in list
	{
		//next->first
		l->head = temp->next;
		temp->next->prev = NULL;
		//first->last
		temp->next = NULL;
		l->tail->next = temp;
		l->tail = temp;
	}
	else if(l->tail == temp)//no updation
		return;
	else//somewhere at middle
	{
		temp->prev->next = temp->next;
		temp->next->prev = temp->prev;

		temp->next = NULL;
		l->tail->next = temp;
		l->tail = temp;
	}
}
static void insertion(List* l, int value)
{
	if(l == NULL)
	{
		perror("the list is not exist");
		return;
	}
	Node* newElement = (Node*)malloc(sizeof(Node));
	if(newElement == NULL)
	{
		perror("cannot create new Node");
		return;
	}
	newElement->next = NULL;
	newElement->prev = NULL;
	newElement->value = value;
	if(l->numOfElements == 0)//empty
	{
		l->head = newElement;
		l->tail = newElement;
	}
	else
	{
		newElement->prev = l->tail;
		l->tail->next = newElement;
		l->tail = newElement;
	}
	l->numOfElements++;
}
