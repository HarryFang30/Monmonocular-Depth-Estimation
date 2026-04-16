#ifndef __BUZZER_H__
#define __BUZZER_H__

#include "xl9555.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief       蜂鸣器开启（低电平触发）
 * @param       无
 * @retval      无
 */
void buzzer_on(void);

/**
 * @brief       蜂鸣器关闭（高电平停止）
 * @param       无
 * @retval      无
 */
void buzzer_off(void);

/**
 * @brief       蜂鸣器短促报警（单次蜂鸣）
 * @param       duration_ms: 蜂鸣持续时间（毫秒）
 * @retval      无
 */
void buzzer_beep(int duration_ms);

/**
 * @brief       蜂鸣器报警模式（持续间歇性蜂鸣）
 * @param       enable: 1开启报警，0停止报警
 * @retval      无
 */
void buzzer_alarm(int enable);

/**
 * @brief       初始化蜂鸣器报警任务
 * @param       无
 * @retval      无
 */
void buzzer_init_alarm_task(void);

#ifdef __cplusplus
}
#endif

#endif /* __BUZZER_H__ */
