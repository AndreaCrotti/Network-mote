
#ifndef __QUEUE_H
#define __QUEUE_H


// This should be assigned to any newly created struct s_queue variable
#define QUEUE_NEW {NULL,NULL,0}

// Queue element data structure
struct s_queue_elem {
	void* data;
	unsigned int datalen;
	struct s_queue_elem *next;
};

// Queue data structure
struct s_queue {
	struct s_queue_elem *first;
	struct s_queue_elem *last;
	unsigned int len;
};

// Function prototypes (documentation in .c file)
int queue_push(struct s_queue*, char *, unsigned int);
struct s_queue_elem queue_pop(struct s_queue*, int);
struct s_queue* queue_new(void);
void queue_free(struct s_queue*);

#endif // __QUEUE_H
