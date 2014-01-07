#include "hal.h"
#include "radio.h"
#include "tasks.h"
#include "sleep.h"

typedef struct _Task
{
    uint16_t intervalMillis;
    uint16_t millisLeft;
    TaskCallback callback;
} Task;

#define MAX_TASKS 4

static unsigned int g_numTasks;
static Task g_tasks[MAX_TASKS];

void clearTasks()
{
    g_numTasks = 0;
}

void addTask(TaskCallback cb, uint16_t interval)
{
    if (g_numTasks < MAX_TASKS)
    {
        Task* tsk = &g_tasks[g_numTasks];
        tsk->intervalMillis = interval;
        tsk->millisLeft = interval;
        tsk->callback = cb;
        g_numTasks++;
    }
}

void runTasks(uint16_t deltaMillis)
{
    unsigned int i;
    for (i=0; i < g_numTasks; ++i)
    {
        Task* task = &g_tasks[i];
        if (task->millisLeft <= deltaMillis)
        {
            if(task->callback())
            {
                // Terminate early.
                return;
            }

            task->millisLeft = task->intervalMillis;
        }
        else
        {
            task->millisLeft -= deltaMillis;
        }
    }
}

void initCB()
{
    // NOTE this is called w/ interrupts disabled

    halSetTimerCallback(&runTasks);
    sleepMode_begin();
}

int main(void)
{
    halMain(&initCB);
}
