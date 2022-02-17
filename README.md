# RTOS_Assignment_1
Assignment for the realtime operating system course. (Task Scheduling)

Starting from the exercise with 3 tasks scheduled with Rate Monotonic



- Create an application with 4 periodic tasks, with period 80ms, 100ms, 200ms, 160ms (the task with the highest priority is called Task1, the one with the lowest priority Task4)

- Create 3 global variables called T1T2, T1T4, T2T3.

- Task1  shall write something into T1T2, Task 2 shall read from it.

- Task1  shall write something into T1T4, Task 4 shall read from it.

- Task2  shall write something into T2T3, Task 3 shall read from it.

-All critical sections shall be protected by semaphores

- Semaphores shall use Priority Ceiling
