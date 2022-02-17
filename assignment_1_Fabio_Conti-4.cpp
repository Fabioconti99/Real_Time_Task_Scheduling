//compile with: g++ -lpthread <sourcename> -o <executablename>

//This exercise shows how to schedule threads with Rate Monotonic, with a priority ceiling scheduling.

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <math.h>
#include <sys/types.h>
#include <sys/types.h>
#include <limits.h>

#define INT_MIN (-INT_MAX - 1)

// Colors for konsole prints.
const char *bhgreen = "\e[1;92m";
const char *bhyellow = "\e[1;93m";
const char *bhblue = "\e[1;94m";
const char *bhmagenta = "\e[1;95m";
const char *reset = "\033[0m";

//code of periodic tasks
void task1_code();
void task2_code();
void task3_code();
void task4_code();

//the Tasks shall right/read on 3 different variables

//T1T2: Task1 Shall write on it, Task2 shall read from it

//T1T4: Task1 Shall write on it, Task4 shall read from it

//T2T3: Task2 Shall write on it, Task3 shall read from it

int T1T2, T1T4, T2T3;

//characteristic function of the thread, only for timing and synchronization
//periodic tasks
void *task1(void *);
void *task2(void *);
void *task3(void *);
void *task4(void *);


// initialization of mutexes and conditions

pthread_mutex_t T1T2_protection;
pthread_mutex_t T1T4_protection;
pthread_mutex_t T2T3_protection;

#define LOOP 750

#define NPERIODICTASKS 4
#define NAPERIODICTASKS 0
#define NTASKS NPERIODICTASKS + NAPERIODICTASKS

long int periods[NTASKS];
struct timespec next_arrival_time[NTASKS];
double WCET[NTASKS] = {0.0, 0.0, 0.0, 0.0};

// Duration of the longest critical section of the task Ji; Maximum Blocking Time 
double B[NTASKS];

// Set of all the possible critical section for each of the 4 tasks.
double z_11, z_12, z_21, z_22, z_31, z_41;


pthread_attr_t attributes[NTASKS];
pthread_t thread_id[NTASKS];
struct sched_param parameters[NTASKS];
int missed_deadlines[NTASKS];


// Function that wastes time to expand the critical sections of each task.
void waste(int timeloss)
{
  int a = 0;
  for (int i = 0; i < timeloss; i++)
  {
    for (int j = 0; j < timeloss; j++)
    {
      a++;
    }
  }
}

// function that will find the highest val among the members of an array of doubles
double higher_val(double arr[], int size)
{
  if (size == 0)
  {
    return 0.0;
  }
  else
  {

    double val = INT_MIN;

    for (int i = 0; i < size; i++)
    {

      if (val < arr[i])
      {
        val = arr[i];
      }
    }
    return val;
  }
}

int main()
{

  
  // set task periods in nanoseconds
  //the first task has period 80 millisecond
  //the second task has period 100 millisecond
  //the third task has period 160 millisecond
  //the fourth task has period 200 millisecond

  // they are already odered based on their period's length
  // the RM priority is invertionally proportional to the length of their period
  // the way the periods are sorted corresponds to the priority of the tasks they relate to
  periods[0] = 80000000;  //in nanosecods
  periods[1] = 100000000; //in nanoseconds
  periods[2] = 160000000; //in nanoseconds
  periods[3] = 200000000; //in nanoseconds


  //this is not strictly necessary, but it is convenient to
  //assign a name to the maximum and the minimum priotity in the
  //system. We call them priomin and priomax.

  struct sched_param priomax;
  priomax.sched_priority = sched_get_priority_max(SCHED_FIFO);
  struct sched_param priomin;
  priomin.sched_priority = sched_get_priority_min(SCHED_FIFO);

  // set the maximum priority to the current thread (you are required to be
  // superuser). Check that the main thread is executed with superuser privileges
  // before doing anything else.

  if (getuid() == 0)
    pthread_setschedparam(pthread_self(), SCHED_FIFO, &priomax);

  // execute all tasks in standalone modality in order to measure execution times.
  // We'll execute each task 100 times to get only the worst execution time obtained 
  // among all tries.

  int i;
  for (i = 0; i < NTASKS; i++)
  {

    // initialize time_1 and time_2 required to read the clock
    struct timespec time_1, time_2;
    double worst_time = 0;

    //we should execute each task more than one for computing the WCET
    //periodic tasks
    
    for (int j = 0; j < 100; j++)
    {
      clock_gettime(CLOCK_REALTIME, &time_1);
      if (i == 0)
      {
        task1_code();
      }
      if (i == 1)
      {
        task2_code();
      }
      if (i == 2)
      {
        task3_code();
      }
      if (i == 3)
      {
        task4_code();
      }

      clock_gettime(CLOCK_REALTIME, &time_2);

      // compute the Worst Case Execution Time.
      // These nested cycles run the tasks 100 times each. 
      // The longest execution time for each task will be assigned as its execution time.

      worst_time = 1000000000 * (time_2.tv_sec - time_1.tv_sec) + (time_2.tv_nsec - time_1.tv_nsec);
      
      
      if (worst_time > WCET[i])
      {
        WCET[i] = worst_time;
      }
    }

    printf("\nWorst Case Execution Time %d=%f \n", i, WCET[i]);
  }

  // Set of all critical section that could block Ji (Beta_s_*i*)
  // All the critical sections are determined within the task during the previos standalone
  // execution. (This times will be assigned to the Z_*ij* global variables)
  // The sections assigned to the following sets are the actual ones that could block 
  // Ji (Beta_s_*i*) for direct or indirect blocking due to the semaphores protecting the shared
  // memory areas.
  double Beta_s_1[2] = {z_21, z_41};
  double Beta_s_2[2] = {z_31, z_41};
  double Beta_s_3[1] = {z_41};
  double Beta_s_4[0] = {};
  
  // According to the Priority Ceiling scheduling protocol, the worst execution for each task is the longest critical section that could block the task Ji
  double B_1 = 0.0, B_2 = 0.0, B_3 = 0.0, B_4 = 0.0;

  for (int i = 0; i < NTASKS; i++)
  {
    if (i == 0)
    {
      B_1 = higher_val(Beta_s_1, 2);
    }
    if (i == 1)
    {
      B_2 = higher_val(Beta_s_2, 2);
    }
    if (i == 2)
    {
      B_3 = higher_val(Beta_s_3, 1);
    }
    if (i == 3)
    {
      B_4 = higher_val(Beta_s_4, 0);
    }
  }
  
  // Set of all blocking time for each critical section
  double B[4]{B_1, B_2, B_3, B_4};

  // compute U
  double U[4] = {0.0, 0.0, 0.0, 0.0};

  for (i = 0; i < NTASKS; i++)
  {

    for (int j = 0; j <= i; j++)
    {
      U[i] += WCET[j] / periods[j];
    }

    U[i] += B[i] / periods[i];
  }
  
  //Compute Ulub for non-harmonic relationship between the periods of the tasks.
  
  // Ulub for each of the tasks subsets.
  double Ulub[4];

  for (i = 0; i < NTASKS; i++)
  {
    Ulub[i] = (i + 1) * (pow(2.0, (1.0 / (i + 1))) - 1);
  }
  
  //check the sufficient conditions: if they are not satisfied, exit
  for (i = 0; i < NTASKS; i++)
  {
    if (U[i] > Ulub[i])
    {
      printf("\n U=%lf Ulub=%lf the sufficient condition is not met", U[i], Ulub[i]);
      return (-1);
    }
  }
  
  for (int i = 0; i < NTASKS; i++)
  {
    printf("\n U=%lf Ulub=%lf Scheduable Task Set\n", U[i], Ulub[i]);
  }

  fflush(stdout);
  sleep(5);

  // set the minimum priority to the current thread: this is now required because
  //we will assign higher priorities to periodic threads to be soon created
  //pthread_setschedparam

  if (getuid() == 0)
    pthread_setschedparam(pthread_self(), SCHED_FIFO, &priomin);

  // set the attributes of each task, including scheduling policy and priority
  for (i = 0; i < NPERIODICTASKS; i++)
  {
    //initialize the attribute structure of task i
    pthread_attr_init(&(attributes[i]));

    //set the attributes to tell the kernel that the priorities and policies are explicitly chosen,
    //not inherited from the main thread (pthread_attr_setinheritsched)
    pthread_attr_setinheritsched(&(attributes[i]), PTHREAD_EXPLICIT_SCHED);

    // set the attributes to set the SCHED_FIFO policy (pthread_attr_setschedpolicy)
    pthread_attr_setschedpolicy(&(attributes[i]), SCHED_FIFO);

    //properly set the parameters to assign the priority inversely proportional
    //to the period
    parameters[i].sched_priority = priomin.sched_priority + NTASKS - i;

    //set the attributes and the parameters of the current thread (pthread_attr_setschedparam)
    pthread_attr_setschedparam(&(attributes[i]), &(parameters[i]));
  }

  // initialization of the 3 mutexs that will protect the shared resources
  pthread_mutexattr_t mymutexattr;
  pthread_mutexattr_init(&mymutexattr);
  pthread_mutexattr_setprotocol(&mymutexattr, PTHREAD_PRIO_PROTECT);
  
  // assigning the priority to the mutexs. 
  
  // According to the priority ceiling protocol, the mutexs will be assigned with the highest
  // priority among the tasks they directly block. 
  pthread_mutexattr_setprioceiling(&mymutexattr, parameters[1].sched_priority);
  pthread_mutex_init(&T2T3_protection, &mymutexattr);

  pthread_mutexattr_setprioceiling(&mymutexattr, parameters[0].sched_priority);
  pthread_mutex_init(&T1T4_protection, &mymutexattr);

  pthread_mutexattr_setprioceiling(&mymutexattr, parameters[0].sched_priority);
  pthread_mutex_init(&T1T2_protection, &mymutexattr);


  //declare the variable to contain the return values of pthread_create
  int iret[NTASKS];

  //declare variables to read the current time
  struct timespec time_1;
  clock_gettime(CLOCK_REALTIME, &time_1);

  // set the next arrival time for each task. This is not the beginning of the first
  // period, but the end of the first period and beginning of the next one.
  for (i = 0; i < NPERIODICTASKS; i++)
  {
    long int next_arrival_nanoseconds = time_1.tv_nsec + periods[i];
    //then we compute the end of the first period and beginning of the next one
    next_arrival_time[i].tv_nsec = next_arrival_nanoseconds % 1000000000;
    next_arrival_time[i].tv_sec = time_1.tv_sec + next_arrival_nanoseconds / 1000000000;
    missed_deadlines[i] = 0;
  }

  // create all threads(pthread_create)
  iret[0] = pthread_create(&(thread_id[0]), &(attributes[0]), task1, NULL);
  iret[1] = pthread_create(&(thread_id[1]), &(attributes[1]), task2, NULL);
  iret[2] = pthread_create(&(thread_id[2]), &(attributes[2]), task3, NULL);
  iret[3] = pthread_create(&(thread_id[3]), &(attributes[3]), task4, NULL);

  // join all threads (pthread_join)
  pthread_join(thread_id[0], NULL);
  pthread_join(thread_id[1], NULL);
  pthread_join(thread_id[2], NULL);
  pthread_join(thread_id[3], NULL);

  
  // Prints of the number of missed deadlines for each task.
  for (i = 0; i < NTASKS; i++)
  {
    printf("\nMissed Deadlines Task %d=%d\n", i+1, missed_deadlines[i]);
    fflush(stdout);
  }
  
  
  pthread_mutexattr_destroy(&mymutexattr);
  exit(0);
}

// application specific task_1 code

void task1_code()
{
  //print the id of the current task
  printf("%s 1[ %s",bhmagenta,reset);
  fflush(stdout);
  
  //declare variables to calculate the length of the critical sections
  struct timespec time_ris_1, time_ris_2, time_ris_1_2, time_ris_2_2;


  clock_gettime(CLOCK_REALTIME, &time_ris_1);
  pthread_mutex_lock(&T1T2_protection);
  waste(LOOP);
  T1T2 = rand() * rand();
  pthread_mutex_unlock(&T1T2_protection);
  clock_gettime(CLOCK_REALTIME, &time_ris_2);

  clock_gettime(CLOCK_REALTIME, &time_ris_1_2);
  pthread_mutex_lock(&T1T4_protection);
  waste(LOOP);
  T1T4 = rand() * rand();
  pthread_mutex_unlock(&T1T4_protection);
  clock_gettime(CLOCK_REALTIME, &time_ris_2_2);

// computation of the critical section time lenght in nano seconds.
  z_11 = 1000000000 * (time_ris_2.tv_sec - time_ris_1.tv_sec) + (time_ris_2.tv_nsec - time_ris_1.tv_nsec);
  z_12 = 1000000000 * (time_ris_2_2.tv_sec - time_ris_1_2.tv_sec) + (time_ris_2_2.tv_nsec - time_ris_1_2.tv_nsec);


  //print the id of the current task
  printf("%s ]1 %s",bhmagenta,reset);
  fflush(stdout);
}

//thread code for task_1 (used only for temporization)
void *task1(void *ptr)
{
  // set thread affinity, that is the processor on which threads shall run
  cpu_set_t cset;
  CPU_ZERO(&cset);
  CPU_SET(0, &cset);
  pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cset);

  //execute the task one hundred times... it should be an infinite loop (too dangerous)
  int i = 0;
  for (i = 0; i < 100; i++)
  {
    // execute application specific code
    task1_code();

    // deadline is met if the previus next_arrival_time is grater than the current time
    struct timespec current_time, old_arrival_time;
    clock_gettime(CLOCK_REALTIME, &current_time);
    double ct = current_time.tv_sec * 1000000000 + current_time.tv_nsec;
    double at = next_arrival_time[0].tv_sec * 1000000000 + next_arrival_time[0].tv_nsec;

    if (ct > at)
    {
      missed_deadlines[0] += 1;
    }

    // sleep until the end of the current period (which is also the start of the
    // new one
    clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &next_arrival_time[0], NULL);

    // the thread is ready and can compute the end of the current period for
    // the next iteration

    long int next_arrival_nanoseconds = next_arrival_time[0].tv_nsec + periods[0];
    next_arrival_time[0].tv_nsec = next_arrival_nanoseconds % 1000000000;
    next_arrival_time[0].tv_sec = next_arrival_time[0].tv_sec + next_arrival_nanoseconds / 1000000000;
  }
}

void task2_code()
{
  //print the id of the current task
  printf("%s 2[ %s",bhyellow,reset);
  fflush(stdout);
  
  double uno;

  struct timespec time_ris_1, time_ris_2, time_ris_1_2, time_ris_2_2;

  clock_gettime(CLOCK_REALTIME, &time_ris_1);
  pthread_mutex_lock(&T1T2_protection);
  waste(LOOP);
  uno = T1T2;

  clock_gettime(CLOCK_REALTIME, &time_ris_2);
  pthread_mutex_unlock(&T1T2_protection);

  clock_gettime(CLOCK_REALTIME, &time_ris_1_2);
  pthread_mutex_lock(&T2T3_protection);
  waste(LOOP);
  T2T3 = rand() * rand();
  pthread_mutex_unlock(&T2T3_protection);
  clock_gettime(CLOCK_REALTIME, &time_ris_2_2);

  z_21 = 1000000000 * (time_ris_2.tv_sec - time_ris_1.tv_sec) + (time_ris_2.tv_nsec - time_ris_1.tv_nsec);
  z_22 = 1000000000 * (time_ris_2_2.tv_sec - time_ris_1_2.tv_sec) + (time_ris_2_2.tv_nsec - time_ris_1_2.tv_nsec);

  printf("%s ]2 %s",bhyellow,reset);
  fflush(stdout);
}

void *task2(void *ptr)
{
  // set thread affinity, that is the processor on which threads shall run
  cpu_set_t cset;
  CPU_ZERO(&cset);
  CPU_SET(0, &cset);
  pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cset);

  int i = 0;
  for (i = 0; i < 100; i++)
  {
    task2_code();

    struct timespec current_time, old_arrival_time;
    clock_gettime(CLOCK_REALTIME, &current_time);
    double ct = current_time.tv_sec * 1000000000 + current_time.tv_nsec;
    double at = next_arrival_time[1].tv_sec * 1000000000 + next_arrival_time[1].tv_nsec;

    if (ct > at)
    {
      missed_deadlines[1] += 1;
    }

    clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &next_arrival_time[1], NULL);
    long int next_arrival_nanoseconds = next_arrival_time[1].tv_nsec + periods[1];
    next_arrival_time[1].tv_nsec = next_arrival_nanoseconds % 1000000000;
    next_arrival_time[1].tv_sec = next_arrival_time[1].tv_sec + next_arrival_nanoseconds / 1000000000;
  }
}

void task3_code()
{
  //print the id of the current task
  printf("%s 3[ %s",bhgreen,reset);
  fflush(stdout);
  
 
  double uno;

  struct timespec time_ris_1, time_ris_2;

  clock_gettime(CLOCK_REALTIME, &time_ris_1);

  pthread_mutex_lock(&T2T3_protection);
  waste(LOOP);
  uno = T2T3;

  pthread_mutex_unlock(&T2T3_protection);

  clock_gettime(CLOCK_REALTIME, &time_ris_2);

  //print the id of the current task
  z_31 = 1000000000 * (time_ris_2.tv_sec - time_ris_1.tv_sec) + (time_ris_2.tv_nsec - time_ris_1.tv_nsec);

  printf("%s ]3 %s",bhgreen,reset);
  fflush(stdout);
}

void *task3(void *ptr)
{
  // set thread affinity, that is the processor on which threads shall run
  cpu_set_t cset;
  CPU_ZERO(&cset);
  CPU_SET(0, &cset);
  pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cset);

  int i = 0;
  for (i = 0; i < 100; i++)
  {
    task3_code();

    struct timespec current_time, old_arrival_time;
    clock_gettime(CLOCK_REALTIME, &current_time);
    double ct = current_time.tv_sec * 1000000000 + current_time.tv_nsec;
    double at = next_arrival_time[2].tv_sec * 1000000000 + next_arrival_time[2].tv_nsec;

    if (ct > at)
    {
      missed_deadlines[2] += 1;
    }

    clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &next_arrival_time[2], NULL);
    long int next_arrival_nanoseconds = next_arrival_time[2].tv_nsec + periods[2];
    next_arrival_time[2].tv_nsec = next_arrival_nanoseconds % 1000000000;
    next_arrival_time[2].tv_sec = next_arrival_time[2].tv_sec + next_arrival_nanoseconds / 1000000000;
  }
}

void task4_code()
{
  //print the id of the current task
  printf("%s 4[ %s", bhblue,reset);
  fflush(stdout);
  
  double uno;

  struct timespec time_ris_1, time_ris_2;
  
  clock_gettime(CLOCK_REALTIME, &time_ris_1);
  pthread_mutex_lock(&T1T4_protection);
  waste(LOOP);
  uno = T1T4;
  pthread_mutex_unlock(&T1T4_protection);
  clock_gettime(CLOCK_REALTIME, &time_ris_2);

  //print the id of the current task

  z_41 = 1000000000 * (time_ris_2.tv_sec - time_ris_1.tv_sec) + (time_ris_2.tv_nsec - time_ris_1.tv_nsec);

  printf("%s ]4 %s", bhblue,reset);
  fflush(stdout);
}

void *task4(void *ptr)
{
  // set thread affinity, that is the processor on which threads shall run
  cpu_set_t cset;
  CPU_ZERO(&cset);
  CPU_SET(0, &cset);
  pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cset);

  int i = 0;
  for (i = 0; i < 100; i++)
  {
    task4_code();

    struct timespec current_time, old_arrival_time;
    clock_gettime(CLOCK_REALTIME, &current_time);
    double ct = current_time.tv_sec * 1000000000 + current_time.tv_nsec;
    double at = next_arrival_time[3].tv_sec * 1000000000 + next_arrival_time[3].tv_nsec;

    if (ct > at)
    {
      missed_deadlines[3] += 1;
    }

    clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &next_arrival_time[3], NULL);
    long int next_arrival_nanoseconds = next_arrival_time[3].tv_nsec + periods[3];
    next_arrival_time[3].tv_nsec = next_arrival_nanoseconds % 1000000000;
    next_arrival_time[3].tv_sec = next_arrival_time[3].tv_sec + next_arrival_nanoseconds / 1000000000;
  }
}

//////////////////////////////////////////CONCLUSIONS/////////////////////////////////////

// In order to make the computation of the Utilization factor a little bit more realistic, 
// I decided to add to the code an extra cycle that will execute every task_code_i 100 times. 
// The Worst Execution Time will be assigned as the worst task's execution among them.
// Since the next arrival time is always computed within the thread's charatteristic function,
// calculating the missed deadlines turns out to be a fairly easy task.
// The only extra thing you have to do is calculating a current time struct at every iteration of
// the task and compare it to the last recorded "next arrival time" value. If the task didn't
// meet the deadline the current time will be higher than the next arrival time. The task will
// outomatically record it in the corrisponding array.

// The Waste time function is computed with an high amount of loops. If the Utilization factor
// doesen't meet the Ulub you could either try to run the simulation again or lower the loops'
// number by changing the value of the LOOP parameter.

// If the simulation begins, it means that the calculated Utilization factors are lower than the 
// U Lower Upper Bounds calculated for each  of the Tasks'SubSets. This means that the simulation
// shouldn't lead to any missed deadlines. After running the simualtion many times I've never got
// any Missed deadlines. If it'd ever happend it means that some higher priority "process" is 
//taking place in the backgound during the simulation.
 
