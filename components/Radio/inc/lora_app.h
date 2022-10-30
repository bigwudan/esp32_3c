#ifndef _LORA_APP_H_
#define _LORA_APP_H_

#include <stdint.h>

//创建任务
void lora_app_create_task();

void lorawan_main();

void lora_app_init();

void lora_app_set_pingpong();

void lora_app_process();
#endif

