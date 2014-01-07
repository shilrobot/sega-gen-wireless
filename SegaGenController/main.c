#include "hal.h"
#include "radio.h"
#include "tasks.h"

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

void radioSleep()
{
    // Ensure radio is powered down
    radioWriteRegisterByte(RADIO_REG_CONFIG, 0);

    // Clear all queues
    radioFlushTX();
    radioFlushRX();
    radioWriteRegisterByte(RADIO_REG_STATUS, BIT6 | BIT5 | BIT4);
}

void sleepMode_begin();
void awakeMode_begin();

int sleepMode_pollButtons()
{
    uint8_t buttons = halReadButtons();
    if (buttons)
    {
        awakeMode_begin();
        return 1;
    }

    return 0;
}

void sleepMode_begin()
{
    halBeginNoInterrupts();

    radioSleep();
    halSetTimerInterval(250, 1);
    halSetRadioIRQCallback(0);
    clearTasks();
    addTask(&sleepMode_pollButtons, 250);

    halEndNoInterrupts();
}

void initCB()
{
    // NOTE this is called w/ interrupts disabled

    halSetTimerCallback(&runTasks);
    awakeMode_begin();
}

int main(void)
{
    halMain(&initCB);
}
