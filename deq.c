#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "deq.h"
#include "error.h"

// indices and size of array of node pointers
// Head is always the first node
// Tail is always the last node
// 
typedef enum {Head,Tail,Ends} End;

// Node definition
typedef struct Node {
  struct Node *np[Ends];        // next/prev neighbors
  Data data; // Holds the payload
} *Node;

// Queue representation
typedef struct {
  Node ht[Ends];                // head/tail nodes
  int len;
} *Rep;

static Rep rep(Deq q) {
  // if q is null (error in its creation)
  if (!q) ERROR("zero pointer");
  // Else return the Deq q cast as Rep q
  return (Rep)q;
}

/**
 * Creates a new node with empty Head and Tail
 * 
 * @param d - Data to be stored in the Node
 */
static Node new_node(Data d){
  // Allocating Memory for new node
  Node n = (Node)malloc(sizeof(*n));
  if (!n){
    ERROR("Malloc failed"); 
  }
  // Setting node values
  n->data = d;
  n->np[Head] = NULL;
  n->np[Tail]= NULL;
  
  return n;
}
static void put(Rep r, End e, Data d) {
  // Create a new node
  Node newNode = new_node(d);

  // Get current Queue length
  int curQueueLen = deq_len(r);
  
  // If queue is empty make new node head and tail
  if(curQueueLen == 0){
    r->ht[Head]=newNode;
    r->ht[Tail]=newNode;
  }else{
    if (e == Head)
    {
    // Ex.) Put head h e l l o return o l l e h
    // Set new nodes next the current Head
    newNode->np[Tail] = r->ht[Head];

    // Set current Head previous to be new node
    r->ht[Head]->np[Head] = newNode;

    // Make head new node again
    r->ht[Head] = newNode;
    }else{
    // Ex.) Put tail h e l l o return h e l l o
    // Set new nodes prev the current Tail
    newNode->np[Head] = r->ht[Tail];

    // Set current Tail next to be new node
    r->ht[Tail]->np[Tail] = newNode;

    // Make tail new node again
    r->ht[Tail] = newNode;
    }
    
  }

  // Increment queue size
  r->len+=1;
}

static Data ith(Rep r, End e, int i)  { 
  // Loop through the queue for "i" iterations until the end or until the correct index has been found
  // 1. Check if the "i" is valid
  if (i < 0 || i >= r->len){
    printf("ERROR: Invalid i parameter\n");
    return NULL;
  }
  // Starting at the end based on "e"
  Node currNode = r->ht[e]; 

  for (int idx = 0; idx < i; idx++){
    // Based on which end we either call next or prev
    if (e == Head){
      currNode = currNode->np[Tail];
    }else{
      currNode = currNode->np[Head];
    }
  }
  Data result = currNode->data;
  return result; 
}

static Data get(Rep r, End e){ 
  int queueLength = r->len;

  // Check if queue is empty
  if (queueLength == 0){
    // DEBUG
    printf("Error: Called get on empty queue\n");
    return NULL;
  }
  // Depending on given "e" that is the one we will return
  Node returnNode = r->ht[e];
  // If queue has only one element logic is the same
  if (queueLength == 1){
    r->ht[Head]= NULL;
    r->ht[Tail]= NULL;
    
  }else if(e == Head)
  {
    // Getting from Head (removing Head)
    // 1. Set the new Queue Head to be the prev Head next
    r->ht[Head] = returnNode->np[Tail];
    // 2. Update the new Heads prev to be null
    r->ht[Head]->np[Head] = NULL;
  }else if(e == Tail){
    // Getting the Tail
    // 1. Set the new Queue Tail to be the prev Tail prev
    r->ht[Tail] = returnNode->np[Head];
    // 2. Update the new Tail next to be null
    r->ht[Tail]->np[Tail] = NULL;

  }

  // Decrementing Queue length
  r->len -= 1;
  // Free node removed
  Data result = returnNode->data; 
  free(returnNode);
  return result; 
}

// rem: return by == comparing, len-- (iff found)
static Data rem(Rep r, End e, Data d) { 

  // Check if queue is empty
  if (r->len ==0 ){
    printf("ERROR: rem call on empty queue\n");
    return NULL;
  }
  
  // printf(">>>> REM\n");
  int queueLength = r->len;
  // Start at the end based on "e"
  Node currentNode = r->ht[e];
  Data currentNodeData = currentNode->data;

  // Loop through the queue until the end or until node data matching "d" has been found
  for (int i = 0; i < queueLength; i++){
    // Check if the node data is equal to d
  
    // If the data in the current node matches "d" remove current node. Return d
    if (currentNodeData == d){
      // If head or tail, delegate to get()
      if (currentNode == r->ht[Head]){
        return get(r, Head);
      }

      if (currentNode == r->ht[Tail]) {
        return get(r, Tail);
      }
       // Implement logic to remove the node that had the matchin data
      // 1. Get the current node's prev and next
      // Setting the currentNode prev to be its next
      currentNode->np[Head]->np[Tail] = currentNode->np[Tail];
      // Setting the currentNode next to be its prev
      currentNode->np[Tail]->np[Head] = currentNode->np[Head];
      // Free node removed
      free(currentNode);
      // Decrement the length of Queue
      r->len -=1;
      return currentNodeData;
    }

    // Handle the situation in which the "d" is not found
    // Check if we've made through all the queue
    if (i == queueLength -1){
      // At the end of the queue and matching data has not been found
      return NULL;
    }
    

    // "d" has not been found and there are still more nodes to check
    // Based on which end we either call next or prev
    if (e == Head){
      currentNode = currentNode->np[Tail];
    }else{
      currentNode = currentNode->np[Head];
    }
    currentNodeData = currentNode->data; 

  }

  printf("Data d has NOT BEEN FOUND\n");
  return NULL; 
}

/**
 * Instantiate a new queue with a Head and Tail that are null and lenght is 0
 */
extern Deq deq_new() {
  Rep r=(Rep)malloc(sizeof(*r));
  if (!r) ERROR("malloc() failed");
  r->ht[Head]=0;
  r->ht[Tail]=0;
  r->len=0;
  return r;
}
/**
 * Returns the length of the queue
 * 
 * @param q - An already instantiated queue to find the length of
 */
extern int deq_len(Deq q) { return rep(q)->len; }


extern void deq_head_put(Deq q, Data d) {
  put(rep(q),Head,d); 
}
extern Data deq_head_get(Deq q)         { return get(rep(q),Head);   }
extern Data deq_head_ith(Deq q, int i)  { return ith(rep(q),Head,i); }
extern Data deq_head_rem(Deq q, Data d) { return rem(rep(q),Head,d); }

extern void deq_tail_put(Deq q, Data d) {        put(rep(q),Tail,d); }
extern Data deq_tail_get(Deq q)         { return get(rep(q),Tail);   }
extern Data deq_tail_ith(Deq q, int i)  { return ith(rep(q),Tail,i); }
extern Data deq_tail_rem(Deq q, Data d) { return rem(rep(q),Tail,d); }

extern void deq_map(Deq q, DeqMapF f) {
  for (Node n=rep(q)->ht[Head]; n; n=n->np[Tail])
    f(n->data);
}

extern void deq_del(Deq q, DeqMapF f) {
  if (f) deq_map(q,f);
  Node curr=rep(q)->ht[Head];
  while (curr) {
    Node next=curr->np[Tail];
    free(curr);
    curr=next;
  }
  free(q);
}

extern Str deq_str(Deq q, DeqStrF f) {
  char *s=strdup("");
  for (Node n=rep(q)->ht[Head]; n; n=n->np[Tail]) {
    char *d=f ? f(n->data) : n->data;
    char *t; asprintf(&t,"%s%s%s",s,(*s ? " " : ""),d);
    free(s); s=t;
    if (f) free(d);
  }
  return s;
}
