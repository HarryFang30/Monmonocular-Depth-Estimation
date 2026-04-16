#include "buzzer.h"
#include "esp_log.h"

static const char *TAG = "BUZZER";
static bool alarm_active = false;

/**
 * @brief       蜂鸣器开启（低电平触发）
 * @param       无
 * @retval      无
 */
void buzzer_on(void)
{
    xl9555_pin_write(BEEP_IO, 0);  // 低电平开启蜂鸣器
}

/**
 * @brief       蜂鸣器关闭（高电平停止）
 * @param       无
 * @retval      无
 */
void buzzer_off(void)
{
    xl9555_pin_write(BEEP_IO, 1);  // 高电平关闭蜂鸣器
}

/**
 * @brief       蜂鸣器短促报警（单次蜂鸣）
 * @param       duration_ms: 蜂鸣持续时间（毫秒）
 * @retval      无
 */
void buzzer_beep(int duration_ms)
{
    buzzer_on();
    vTaskDelay(pdMS_TO_TICKS(duration_ms));
    buzzer_off();
}

/**
 * @brief       蜂鸣器报警模式（持续间歇性蜂鸣）
 * @param       enable: 1开启报警，0停止报警
 * @retval      无
 */
void buzzer_alarm(int enable)
{
    alarm_active = enable;
    if (!enable) {
        buzzer_off();  // 立即停止蜂鸣器
        ESP_LOGI(TAG, "Buzzer alarm stopped immediately");
    } else {
        ESP_LOGI(TAG, "Buzzer alarm started");
    }
}

/**
 * @brief       蜂鸣器报警任务（间歇性蜂鸣）
 * @param       pvParameters: 未使用
 * @retval      无
 */
static void buzzer_alarm_task(void *pvParameters)
{
    while (1) {
        if (alarm_active) {
            // 报警模式：短促蜂鸣 + 短暂停止
            buzzer_on();
            
            // 分段延时，每50ms检查一次alarm_active状态
            for (int i = 0; i < 4 && alarm_active; i++) {
                vTaskDelay(pdMS_TO_TICKS(50));  // 50ms * 4 = 200ms总蜂鸣时间
            }
            
            buzzer_off();
            
            // 分段延时，每50ms检查一次alarm_active状态
            for (int i = 0; i < 6 && alarm_active; i++) {
                vTaskDelay(pdMS_TO_TICKS(50));  // 50ms * 6 = 300ms总停止时间
            }
        } else {
            // 确保蜂鸣器关闭
            buzzer_off();
            vTaskDelay(pdMS_TO_TICKS(100));  // 空闲时等待100ms
        }
    }
}

/**
 * @brief       初始化蜂鸣器报警任务
 * @param       无
 * @retval      无
 */
void buzzer_init_alarm_task(void)
{
    static bool task_created = false;
    if (!task_created) {
        xTaskCreate(buzzer_alarm_task, "buzzer_alarm", 2048, NULL, 5, NULL);
        task_created = true;
        ESP_LOGI(TAG, "Buzzer alarm task created");
    }
}
