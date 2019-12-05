/*********************************************************************/
/* Name: Simulation - M/M/1 Queueing System                             */
/* Author: Richard Hurley & Brianna Drew                                */
/* Description                                                          */
/*    This program simulates a single server queue with exponential     */
/* interarrival time, exonential service time, and RR (Quantum = 4)     */
/* scheduling discipline.  The parameter to the expon funciton is       */
/* scaled by 100 to avoid problems with generating exponential          */
/* variates. To turn the  debugging output off, change the constant     */
/* DEBUG to 0 and re-compile.                                           */
/*********************************************************************/
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <values.h>

/* simulation events */
#define ARRIVAL 0       /* arrival to queue */
#define COMPLETE 1      /* completion of service */
#define EOS 2           /* end of simulation */

/* programming constants */
#define FALSE 0
#define TRUE 1
#define DEBUG 0 /* set to 1 to turn debugging output on */

/* event list - which is a doubly linked list */
struct event_node{
        int ev_type;                    /* event type */
        long int ev_time;               /* time for event to occur */
        struct Custs *cust_index;       /* customer responsible for this event */
        struct event_node *forward;     /* forward link */
        struct event_node *backward;    /* backward link */
        } ;
struct event_node *top_event;    /* points to head of event list */
struct event_node *last_event;   /* points to end of event list */
/* customer nodes */
struct Custs{
        long int arrive_time;           /* arrival time of customer */
        long int CPU_time;              /* CPU burst time of customer - ADDED BY ME*/
        long int OG_CPU_time;           /* the original CPU burst time of customer (before decrease due to quantum) - ADDED BY ME */
        long int counter;               /* counts how many bursts each customer has completed - ADDED BY ME*/
        };
/* queue - simple linked list */
struct Queue {
        struct Custs *cust_index;       /* index of customer in the queue */
        struct Queue *next;             /* points to the next node in the queue */
        };

struct Queue_struct {
        struct Queue *q_head;     /* points to top of queue */
        struct Queue *q_last;     /* points to bottom of queue */
        };
struct Queue_struct rr;

/* statistics gathering variables */
float accum_resp_time;  /* accumulate customer response time */
float num_resp_time;    /* total number of custs in system */

/* input parameters */
float iarrive_time;     /* mean interarrival time */
float service_time;     /* mean service time */
long int sim_length;    /* length of simulation */

/* system variables */
long int clock;         /* simulation clock */
int busy;               /* flag indicating if server is busy */
unsigned seed;          /* seed for random num generator */

/* function declarations */
void arrive(struct event_node *ev_num);
void depart(struct event_node *ev_num);
void start_service(void);
void Gen_arrival(void);
void Gen_departure(struct Custs *index);
void Read_parms(void);
void Process_statistics(void);
void Initialize(void);
void Insert_event(int etype, long int etime, struct Custs *custind);
struct event_node *Remove_event(void);
void Puton_queue(struct Queue_struct *pqueue, struct Custs *pcust);
struct Custs *Takoff_queue(struct Queue_struct *pqueue);
long int expon(float time);

/*********************************************************************/
/* Name: main                                                   */
/* Description                                                  */
/*    This function performs the main control loop of the simulation.*/
/* It performs the following steps:                                     */
/*    1 - call routines to initialize global variables.                 */
/*    2 - schedules an end of simulation.                               */
/*    3 - generates the first arrival.                          */
/*    4 - processes the events on the event list until the end of       */
/*        simulation event i reached.                           */
/*    5 - frees event node after it has been processed.                 */
/*    6 - prints out the statistics when the simulation is finished. */
/*********************************************************************/
void main()
  {
  int not_done;
  struct event_node *event;
  /* initialization */
  Initialize();
  Read_parms();
  /* schedule an end of simulation */
  Insert_event(EOS, sim_length, NULL);
  /* generate first arrival */
  Gen_arrival();
  /* main loop to process the event list */
  not_done = TRUE;
  while(not_done)
    {
    /* get next event */
    event = Remove_event();
    /* update clock */
    clock = event->ev_time;
    /* process event type */
    switch (event->ev_type)
        {
                case ARRIVAL  : arrive(event);
                              break;
                case COMPLETE : depart(event);
                              break;
                case EOS      : Process_statistics();
                              not_done = FALSE;
                              break;
                default       : printf("***Error - invalid event type\n");
                }
        /* free event node by marking it unused */
        free(event);
        }
  }

/*********************************************************************/
/* Name: arrive                                                 */
/* Description                                                  */
/*    This function processes an arrival to the system.                 */
/*     It performs                                              */
/* the followiong steps:                                                */
/*    1 - generates the next arrival.                                   */
/*    2 - sets the system statistics.                                   */
/*    3 - puts the customer into the queue.                             */
/*    4 - if the server is not busy then calls start_service.           */
/**********************************************************************/
void arrive(struct event_node *ev_num)
  {
  struct Custs *index;
  /* generate the next arrival */
  Gen_arrival();
  /* set statistics gathering variable */
  index = ev_num->cust_index;
  index->arrive_time = clock;
  /* generate "random" CPU burst time for customer */
  index->CPU_time = expon(service_time); // ADDED BY ME
  index->OG_CPU_time = index->CPU_time; // ADDED BY ME
  /* put the customer n the queue */
  Puton_queue(&rr, index);
  /* if server is not busy then start service */
  if(!busy)
         start_service();
  return;
  }

/**************************************************************/
/* Name: start_service                                        */
/* Description                                                */
/*    This function performs the following steps:             */
/*    1 - removes the first customer from the queue.          */
/*    2 - sets the server to busy.                            */
/*    3 - schedules a departure event.                        */
/**************************************************************/
void start_service(void)
  {
  struct Custs *index;
  /* remove the first customer from the queue */
  index = Takoff_queue(&rr);

  /* set server to busy */
  busy = TRUE;

/* if it is the customer's first burst, increase counter to 1 - ADDED BY ME */
  if(index->OG_CPU_time == index->CPU_time){
    index->counter = 1;
  }
  else{
    index->counter++; // if not, increase counter by 1
  }

  /* if the CPU burst time is still greater than or equal to the Quantum 4... - ADDED BY ME*/
  if(index->CPU_time >= 4){
    index->CPU_time = index->CPU_time - 4; // subtract quantum to simulate a single burst
  }

  /* if the CPU burst time is less than the Quantum 4... - ADDED BY ME*/
  if(index->CPU_time < 4){
      index->CPU_time = 0; // subtract remaining CPU burst time to represent final burst
  }
  /* schedule a departure event */
  Gen_departure(index);
  return;
  }

/********************************************************************/
/* Name: depart                                                     */
/* Description                                                      */
/*    This function processes a departure from the server event. It */
/* performs the following steps:                                    */
/*    1 - sets the server to idle.                                  */
/*    2 - accumulate response time statistics.                      */
/*    3 - remove the customer from the system.                      */
/*    4 - if the queue is not empty, then start service.            */
/********************************************************************/
void depart(struct event_node *ev_num)
  {
  struct Custs *index;
  long int temp;
  /* set server to idle */
  busy = FALSE;
  /* accumulate response time */
  index = ev_num->cust_index;

  /* if it was the customer's first burst, calculate the response time and customer count - ADDED BY ME*/
  if(index->counter = 1){
    temp = clock - index->arrive_time;
    accum_resp_time += temp; num_resp_time++;
  }

#if DEBUG
  printf(" Response time for customer is %d\n", temp);
#endif
  /* remove customer from the system */
  free(index);
 /* if queue is non-empty, start service */
  if(rr.q_head != NULL)
         start_service();
  return;
  }

/*********************************************************************/
/* Name: Gen_arrival                                                 */
/* Description                                                       */
/*  This function will generate a new arrival. It has one parameter, */
/* stream, which is the random number generator stream to be used.It */
/* performs the following steps:                                     */
/*    1 - gets a new customer.                                       */
/*    2 - generates an exponential arrival time.                     */
/*    3 - inserts arrival event into the event list.                 */
/*********************************************************************/
void Gen_arrival(void)
  {
  long int time;
  struct Custs *index;
  /* get new customer */
  index = (struct Custs *) malloc(sizeof(struct Custs));
  /* generate exponential interarrival time */
  time = expon(iarrive_time);
#if DEBUG
  printf(" Interarrival time for customer is %d\n", time);
  printf(" Arrival time for customer is %d\n", clock + time);
#endif
  /* add the event to the list */
  Insert_event(ARRIVAL, clock+time, index);
  return;
}

/*********************************************************************/
/* Name: Gen_departure                                          */
/* Description                                                  */
/*   This function generates a departure event from the server. It      */
/*   has two parameters: 1) stream - random number generator stream, */
/*   and 2) index - index of customer departing. The following  */
/*   steps are performed:                                            */
/*    1 - generate the service time.                                 */
/*    2 - insert the departure event into the event list.            */
/*********************************************************************/
void Gen_departure(struct Custs *index)
  {
  long int time;

  /* generate exponential service time - CHANGED BY ME*/
  time = index->OG_CPU_time - index->CPU_time;

  /* if the customer still has remaining CPU burst after previous burst... - ADDED BY ME*/
  if(index->CPU_time >= 4){
    Puton_queue(&rr, index); // put back on the queue
  }

#if DEBUG
  printf(" Service time for customer is %d\n", time);
  printf(" Departure time for customer is %d\n", clock + time);
#endif
  /* add departure event to the event list */
  Insert_event(COMPLETE, time+clock, index);

  return;
  }

/**************************************************************/
/* Name: Read_parms                                           */
/* Description                                                */
/*    This function inputs the required simulation parameters.*/
/**************************************************************/
void Read_parms(void)
  {
  printf("   SIMULATION -- M/M/1 Queueing System\n");
  printf("      Input the following parameters:\n");
  printf("      mean interarrival time => ");
  scanf("%e", &iarrive_time);
  printf("      mean service time => ");
  scanf("%e", &service_time);
  printf("      length of simulation => ");
  scanf("%ld", &sim_length);
  printf("      seed for the random number generator => ");
  scanf("%d", &seed);
  /* initialize random number generator */
  srand(seed);
  printf(" Simulation time = %ld units\n", sim_length);
  printf(" Simulation begins...\n");
  }

/*********************************************************************/
/* Name: Process_statistics                                     */
/* Description                                                  */
/*  This function computes and prints the mean response time for the */
/*  customers in an M/M/1 system.                               */
/*********************************************************************/
void Process_statistics(void)
  {
  float mean_resp_time;
  /* compute mean response time */
  mean_resp_time = accum_resp_time / (float) (100.0 * num_resp_time);
  /* print out results */
  printf("...Simulation ends\n");
  printf(" Simulation results\n");
  printf(" mean response time ---------> %-6.3f\n", mean_resp_time);
  }

/*********************************************************************/
/* Name: Initialize                                             */
/* Description                                                  */
/*   This function initializes the event list, queue, customer list, */
/*   and global variables.                                           */
/*********************************************************************/
void Initialize(void)
  {
  /* initialize the event list */
  top_event = NULL;
  last_event = NULL;
  /* initialize the queue */
  rr.q_head = NULL;
  rr.q_last = NULL;
  /* initialize the global variables */
  clock = 0;
  busy = FALSE;
  accum_resp_time = 0;
  num_resp_time = 0;
  }

/*********************************************************************/
/* Name: Insert_event                                                   */
/* Description                                                          */
/*    This procedure will insert the simulation event into a doubly     */
/*    linked queue.  The parameters are as follows:                     */
/*     etype - type of event to be inserted.                            */
/*     etime - the time the event will occur.                        */
/*     custind - index of the customer associated with this event.      */
/* The following steps are performed:                                   */
/*     1 - get a free node and add the event information.               */
/*     2 - insert node into the proper place in the queue.              */
/*         2a - into an empty queue.                                    */
/*         2b - at the top of the queue.                                */
/*         2c - at the bottom of the queue.                             */
/*         2d - regular insertion (someplace in the middle).            */
/*********************************************************************/
void Insert_event(int etype, long int etime,struct Custs *custind)
  {
  int not_found;
  struct event_node *loc, *pos;
  loc = (struct event_node *) malloc(sizeof (struct event_node));
 /* add the information to the structure */
  loc->ev_type = etype;
  loc->ev_time = etime;
  loc->cust_index = custind;
  loc->forward = NULL;
  loc->backward = NULL;
 /* determine if the list is empty */
  if(top_event == NULL)
         {
         top_event = loc;
         last_event = loc;
         return;
         }
  /* see if it belongs on top */
   if(top_event->ev_time > etime)
         {
         top_event->backward = loc;
         loc->forward = top_event;
         top_event = loc;
         return;
         }
 /* see if it belongs at the bottom */
  if(last_event->ev_time <= etime)
         {
         last_event->forward = loc;
         loc->backward = last_event;
         last_event = loc;
         return;
         }
 /* it belongs somewhere in the middle so find its place */
  not_found = TRUE;
  pos = top_event;
  while(pos != NULL && not_found)
         {
        if(pos->ev_time > etime)
                not_found = FALSE;
         else
                pos = pos->forward;
         }
  /* check to see if we found something as we should have */
  if(not_found)
         {
         printf(" ***Error - problems in insert event routine***\n");
         return;
         }
  /* add node to appropriate place */
  loc->forward = pos;
  loc->backward = pos->backward;
  (pos->backward)->forward = loc;
  pos->backward = loc;
  return;
}

/*********************************************************************/
/* Name: Remove_event                                           */
/* Description                                                  */
/*    This function returns the next event from the head of the */
/* event list.  It checks for a special case where there is only        */
/* one event so the event list can be marked empty.                     */
/*********************************************************************/
struct event_node *Remove_event(void)
  {
  struct event_node *ev_ptr;
  /* check to see if event list is empty */
  if(last_event == NULL)
         {
         printf(" ***Error - Event list underflow***\n");
         return(NULL);
         }
  /* remove top element */
  ev_ptr = top_event;
  /* see if it was the only event - special case to mark empty */
  if(top_event == last_event)
         {
         top_event = NULL;
         last_event = NULL;
         return(ev_ptr);
         }
  /* event list has more than one element so just relink */
  top_event = top_event->forward;
  top_event->backward = NULL;
  ev_ptr->forward = NULL;
  return(ev_ptr);
  }

/*********************************************************************/
/* Name: Puton_queue                                            */
/* Description                                                          */
/*    This procedure inserts a customer at the end of the given         */
/* queue. The parameters are as follows:                        */
/*     pqueue - pointer to the queue.                                   */
/*     pcust - index of the customer to be inserted.                    */
/* The procedure performs the following steps:                          */
/*     1 - get a free node for the customer.                    */
/*     2 - inset the node ar the end of the queue.              */
/*         2a - into an empty queue                             */
/*         2b - normal insertion                                */
/*********************************************************************/
void Puton_queue(struct Queue_struct *pqueue, struct Custs *pcust)
  {
  struct Queue *newnode;
  /* get an new node */
  newnode = (struct Queue *) malloc(sizeof(struct Queue));
  /* now loc is the index of a free node in queue */
  /* put information in the node */
  newnode->cust_index = pcust;
  newnode->next = NULL;
 /* check to see if the queue is initially empty */
  if(pqueue->q_last == NULL)
         {
         pqueue->q_head = newnode;
         pqueue->q_last = newnode;
         return;
         }
  /* otherwise add it to the end of the queue and relink */
  pqueue->q_last->next = newnode;
  pqueue->q_last = newnode;
  return;
  }

/*********************************************************************/
/* Name: Takoff_queue                                                   */
/* Description                                                          */
/*    This function returns the index into the customer array of        */
/* the head element of the given queue. The parameters are:             */
/*     pqueue -  pointer to the given queue.                            */
/* If the customer removed is the last remaining customer, the  */
/* queue is marked empty.                                       */
/*********************************************************************/
struct Custs *Takoff_queue(struct Queue_struct *pqueue)
  {
  struct Queue *loc;
  struct Custs *index;
  /* check if the queue is empty */
  if(pqueue->q_head == NULL)
         {
         printf(" ***Error - queue underflow***\n");
         return(NULL);
         }
  /* remove top element from queue */
  loc = pqueue->q_head;
  /* get customer index */
  index = loc->cust_index;
 /* check if queue now empty and relink */
  if(pqueue->q_head == pqueue->q_last)
         {
         pqueue->q_last = NULL;
         pqueue->q_head = NULL;
         return(index);
         }
  /* otherwise just relink */
  pqueue->q_head = loc->next;
  free(loc);
  return(index);
  }

/*********************************************************************/
/* Name: expon                                                          */
/* Description                                                          */
/*    This function is used to generate an exponential variate given */
/* the mean time.                                                       */
/*********************************************************************/
long int expon(float time)
  {
  long int val;
  double temp;
  time = time * 100;
  temp = 1.0 - (rand()/ (float) RAND_MAX);
  val = ceil(-time * log((double) temp));
  return(val);
  }
