#ifndef HUB_CONTROL_PROCESSES_H
#define HUB_CONTROL_PROCESSES_H

#include <stdint.h>

void processes_terminate_all(void);
void processes_launch_all(void);
int processes_control_loop(void);

#endif
