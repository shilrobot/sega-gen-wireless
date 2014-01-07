#ifndef TASKS_H_
#define TASKS_H_

#include <stdint.h>

typedef int (*TaskCallback)(void);

void clearTasks();
void addTask(TaskCallback cb, uint16_t interval);

#endif /* TASKS_H_ */
