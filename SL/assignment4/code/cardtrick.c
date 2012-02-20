/**
 * This program will implement a trick of card as stated in the
 * assignment problem.
 */
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string.h>

#define pr_info(args...) fprintf(stdout, ##args)
#define pr_err(args...) fprintf(stdout, ##args)
#define NOMEM "No memory available\n"
#define CHINVAL "Not a valid choice\n"
#define NAME_SIZE 3
#define ONE "One"
#define TWO "Two"
#define THREE "Three"
#define FOUR "Four"
#define FIVE "Five"
#define SIX "Six"
#define SEVEN "Seven"
#define EIGHT "Eight"
#define NINE "Nine"
#define TEN "Ten"
#define JACK "Jack"
#define QUEEN "Queen"
#define KING "King"
#define HEARTS "Hearts"
#define DIAMONDS "Diamonds"
#define SPADES "Spades"
#define CLUBS "Clubs"

struct node {
	/** Previous pointer for doubly linked node */
	struct node *prev;
	/** Next pointer for doubly linked node */
	struct node *next;
	/** Node Data: Name of Card */
	char name[NAME_SIZE];
	/** No of letters in first word */
	int first;
	/** No of letters in third word */
	int third;
};

struct queue {
	/**
	 * Front pointer of queue: A node will always be removed from
	 * front
	 */
	struct node *front;
	/**
	 * Rear pointer of queue: A node will always be inserted to the
	 * rear
	 */
	struct node *rear;
};

struct stack {
	/**
	 * Top pointer of stack: A node will always be pushed and popped
	 * at the top.
	 */
	struct node *top;
};

/**
 * This function will allocate memory for a node and will initialize
 * both prev and next pointer to null.
 */
static struct node *alloc_node(void)
{
	struct node *node;

	node = malloc(sizeof(struct node));
	if (node) {
		node->prev = NULL;
		node->next = NULL;
	}

	return node;
}

/**
 * This function will free memory for allocated node
 */
static void free_node(struct node *node)
{
	free((void *)node);
}

/**
 * This function prints name of all the cards available in input.txt, so
 * that an user can select one of them.
 */
static void print_name(char *name, int count, struct node *node)
{
	char string[20];

	sprintf(string, "%d:", count);
	switch(*name) {
	case '1':
		strcat(string, ONE);
		node->first = sizeof(ONE) - 1;
		break;
	case '2':
		strcat(string, TWO);
		node->first = sizeof(TWO) - 1;
		break;
	case '3':
		strcat(string, THREE);
		node->first = sizeof(THREE) - 1;
		break;
	case '4':
		strcat(string, FOUR);
		node->first = sizeof(FOUR) - 1;
		break;
	case '5':
		strcat(string, FIVE);
		node->first = sizeof(FIVE) - 1;
		break;
	case '6':
		strcat(string, SIX);
		node->first = sizeof(SIX) - 1;
		break;
	case '7':
		strcat(string, SEVEN);
		node->first = sizeof(SEVEN) - 1;
		break;
	case '8':
		strcat(string, EIGHT);
		node->first = sizeof(EIGHT) - 1;
		break;
	case '9':
		strcat(string, NINE);
		node->first = sizeof(NINE) - 1;
		break;
	case 'A':
		strcat(string, TEN);
		node->first = sizeof(TEN) - 1;
		break;
	case 'J':
		strcat(string, JACK);
		node->first = sizeof(JACK) - 1;
		break;
	case 'Q':
		strcat(string, QUEEN);
		node->first = sizeof(QUEEN) - 1;
		break;
	case 'K':
		strcat(string, KING);
		node->first = sizeof(KING) - 1;
		break;
	}
	strcat(string, " of ");
	switch(*(name + 2)) {
	case 'H':
		strcat(string, HEARTS);
		node->third = sizeof(HEARTS) - 1;
		break;
	case 'D':
		strcat(string, DIAMONDS);
		node->third = sizeof(DIAMONDS) - 1;
		break;
	case 'S':
		strcat(string, SPADES);
		node->third = sizeof(SPADES) - 1;
		break;
	case 'C':
		strcat(string, CLUBS);
		node->third = sizeof(CLUBS) - 1;
		break;
	}
	pr_info("%s\n", string);
}

/**
 * This function inserts node in the rear of a queue
 */
static void insert_queue(struct queue *queue, struct node *node)
{
	struct node *rear = queue->rear;

	node->prev =rear;
	if (rear)
		rear->next = node;
	else
		queue->front = node;
	queue->rear = node;
}

/**
 * This function will remove one node from front and will return it.
 */
static struct node *remove_queue(struct queue *queue)
{
	struct node *node = queue->front;

	if(node) {
		queue->front = node->next;
		queue->front->prev = NULL;
		node->next = node->prev = NULL;
	}
	return node;
}

/** This function will push one node to stack. */
static void push(struct stack *stack, struct node *node)
{
	struct node *top = stack->top;

	if (top)
		top->next = node;

	stack->top = node;
	node->prev = top;
}

/** This function will pop one node from stack. */
static struct node * pop(struct stack *stack)
{
	struct node *top = stack->top;

	stack->top = top->prev;
	if (stack->top)
		stack->top->next = NULL;
	return top;
}

/**
 * Main function of program reads input file and then implement the
 * algorithm described in design document.
 */
int main(int argc, char** argv)
{
	FILE *input, *output;
	char name[NAME_SIZE], ch;
	struct node *node, *temp;
	struct queue queue;
	struct stack stack;
	int name_count, letter_count, err = 0, i, first, third, iteration;
	int count;
	unsigned int select;
	char out[12];

	input = fopen("input.txt", "r");
	if (!input) {
		pr_info("Could not open input text file\n");
		return -EINVAL;
	}
	/* initialize variables */
	name_count = 1;
	letter_count = 0;
	queue.front = queue.rear = NULL;
	stack.top = NULL;

	/* read input cards */
	while (fread(&ch, sizeof(ch), 1, input)) {
		switch (letter_count) {
		case 0:
			if ((ch >= '1' && ch <= '9') || ch == 'J' || ch == 'Q'
					|| ch == 'K' || ch == 'A') {
				name[letter_count] = ch;
				letter_count++;
			}
			break;
		case 1:
			if (ch == ' ')
				letter_count++;
			break;
		case 2:
			if (ch == 'H' || ch == 'D' || ch == 'S' || ch == 'C') {
				name[letter_count] = ch;
				letter_count = 0;
				node = alloc_node();
				if (!node) {
					pr_err(NOMEM);
					err = -ENOMEM;
					goto free_node;
				}
				print_name(name, name_count, node);
				name_count++;
				memcpy(node->name, name, NAME_SIZE);
				insert_queue(&queue, node);
			}
			break;
		}
	}

	fclose(input);

	/* select card by user */
	pr_info("Select card:");
	scanf("%d", &select);
	if (select >= name_count) {
		pr_err(CHINVAL);
		err = -EINVAL;
		goto free_node;
	}

	/* get info from selected node */
	node = queue.front;
	for (i =1; i < select; i++)
		node = node->next;

	first = node->first;
	third = node->third;

	for(iteration = 0; iteration < 3; iteration++) {
		if (iteration == 0)
			count = first;
		else if (iteration == 1)
			count = 2;
		else
			count = third;
		for (i = 0; i < count; i++) {
			node = remove_queue(&queue);
			push(&stack, node);
		}
		for (i = 0; i < count; i++) {
			node = pop(&stack);
			insert_queue(&queue, node);
		}
		node = queue.front;
		sprintf(out, "output%d.txt", iteration + 1);
		output = fopen(out, "w");
		if (!output) {
			pr_info("Could not open output%d text file\n",
					iteration);
			err = -EINVAL;
			goto free_node;
		}

		for (i = 0; i < name_count - 1; i++) {
			fprintf(output, "%c %c\n", node->name[0], node->name[2]);
			node = node->next;
		}
		fprintf(output, "\n");
		fclose(output);
	}

free_node:
	node = queue.front;
	while (node) {
		temp = node->next;
		node = node->next;
		free(node);
	}
	return err;
}
