/*
 * FreeRTOSConfig.h
 *
 * Created: 2026-09-18 오후 2:17:14
 *  Author: User
 */ 


#ifndef FREERTOSCONFIG_H_
#define FREERTOSCONFIG_H_

#include <avr/io.h>

#ifndef F_CPU
#define F_CPU 14745600UL
#endif
#define configCPU_CLOCK_HZ				((uint32_t) F_CPU)

#define configUSE_IDLE_HOOK				0
#define configUSE_TICK_HOOK				0
#define configUSE_PREEMPTION			1
#define configTICK_RATE_HZ				((TickType_t) 1000)

#define configMINIMAL_STACK_SIZE		((uint16_t) 85)
#define configTOTAL_HEAP_SIZE			((size_t) 1500)

#define configMAX_PRIORITIES			4
#define configMAX_TASK_NAME_LEN			8

#define configUSE_16_BIT_TICKS			1
#define configIDLE_SHOULD_YIELD			1

#define INCLUDE_vTaskPrioritySet		1
#define INCLUDE_uxTaskPriorityGet		1
#define INCLUDE_vTaskDelete				1
#define INCLUDE_vTaskCleanUpResources	0
#define INCLUDE_vTaskSuspend			1
#define INCLUDE_vTaskDelayUntil			1
#define INCLUDE_vTaskDelay				1



#endif /* FREERTOSCONFIG_H_ */