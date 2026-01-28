
### Test Case Description:

- Task1 priority = 1
- Task2 priority = 2
- mutex1 and mutex2

Test Steps:
- task1
    - take mutex1.
- task2
    - take mutex1 and block indefinitely.
- task1
    - take mutex2.
    - give mutex1.
- task2
    - unblock and take mutex1.
- task1
    - take mutex2 again and block itself for a period of time.
    - block timeout and `vTaskPriorityDisinheritAfterTimeout` be called.
- failed assertion triggered.

### Explanation:

1. Task1 inherited the priority from task2 when holding mutex1.

2. Task1 take mutex2 then give mutex1, so the inherited priotiry 
   won't disinherit back to the original priority. And the 
   number of mutex held by task1 is only one.

3. Task1 take mutex2 again and block itself for a period of time.
   After timeout, the context switch calls `xTaskIncrementTick` to unblock
   task1. Then the "EventItem" of task1 will be removed from the list of
   waiting to take the mutex2. Now, the list is empty.

4. Then the `vTaskPriorityDisinheritAfterTimeout` will be called.

5. In `vTaskPriorityDisinheritAfterTimeout` function.
    - a. The `uxHighestPriorityWaitingTask` will be `0`. Because the
       List of waiting to take the mutex2 is empty.
       - So the following will be set: 
            - `uxPriorityToUse = pxTCB->uxBasePriority;`
    - b. The following will be true:
       - `if( pxTCB->uxPriority != uxPriorityToUse )`
           - Due to uxPriority != uxPriorityToUse == uxBasePriority
    - c. The following will be true:
        - `if( pxTCB->uxMutexesHeld == uxOnlyOneMutexHeld )`
           - Due to task1 only holds one mutex.
    - d. The following assertion will fail:
       - `configASSERT( pxTCB != pxCurrentTCB );`
           - Because task1 is the holder of mutex2 and the `pxCurrentTCB == task1`. 


