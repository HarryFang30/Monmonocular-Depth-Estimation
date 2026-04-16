/**
 ****************************************************************************************************
 * @file        system_state_manager.c
 * @author      AI Assistant
 * @version     V1.0
 * @date        2025-07-25
 * @brief       系统状态管理器实现 - 协调人脸识别和拍照上传
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 */

#include "system_state_manager.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "photo_uploader.h"
#include "buzzer.h"
#include <inttypes.h>

static const char *TAG = "SystemStateMgr";

/* 全局状态管理器 */
system_state_manager_t g_system_state = {0};

/**
 * @brief 初始化系统状态管理器
 */
esp_err_t system_state_manager_init(void)
{
    g_system_state.current_mode = SYSTEM_MODE_FACE_DETECTION;
    g_system_state.photo_upload_requested = false;
    g_system_state.face_detection_paused = false;
    g_system_state.mode_switch_timestamp = 0;
    g_system_state.alarm_start_timestamp = 0;
    g_system_state.alarm_timeout_enabled = false;
    g_system_state.photo_upload_in_progress = false;
    
    ESP_LOGI(TAG, "System state manager initialized - starting in face detection mode");
    return ESP_OK;
}

/**
 * @brief 请求切换到拍照上传模式
 */
void system_request_photo_upload(void)
{
    if (g_system_state.current_mode == SYSTEM_MODE_FACE_DETECTION && 
        !g_system_state.photo_upload_requested && 
        !g_system_state.photo_upload_in_progress) {
        ESP_LOGI(TAG, "📸 Photo upload requested - switching modes");
        printf("🖥️  Pausing LCD display for photo upload...\r\n");
        g_system_state.photo_upload_requested = true;
        g_system_state.face_detection_paused = true;
        g_system_state.current_mode = SYSTEM_MODE_TRANSITIONING;
        g_system_state.mode_switch_timestamp = esp_timer_get_time() / 1000; // 转换为毫秒
        
        printf("🔄 Switching to photo upload mode...\r\n");
    } else {
        ESP_LOGW(TAG, "Photo upload already requested or in progress (mode: %d, requested: %d, in_progress: %d)", 
                 g_system_state.current_mode, 
                 g_system_state.photo_upload_requested,
                 g_system_state.photo_upload_in_progress);
    }
}

/**
 * @brief 完成拍照上传，切换回人脸识别模式
 */
void system_photo_upload_complete(void)
{
    ESP_LOGI(TAG, "📸 Photo upload completed - switching back to face detection");
    g_system_state.photo_upload_requested = false;
    g_system_state.face_detection_paused = false;
    g_system_state.current_mode = SYSTEM_MODE_FACE_DETECTION;
    g_system_state.mode_switch_timestamp = esp_timer_get_time() / 1000;
    
    printf("🔄 Switching back to face detection mode...\r\n");
}

/**
 * @brief 启动报警超时计时
 */
void system_start_alarm_timeout(uint32_t timeout_ms)
{
    g_system_state.alarm_start_timestamp = esp_timer_get_time() / 1000;
    g_system_state.alarm_timeout_enabled = true;
    ESP_LOGI(TAG, "Alarm timeout started: %" PRIu32 " ms", timeout_ms);
}

/**
 * @brief 停止报警超时计时
 */
void system_stop_alarm_timeout(void)
{
    g_system_state.alarm_timeout_enabled = false;
    ESP_LOGD(TAG, "Alarm timeout stopped");
}

/**
 * @brief 获取是否允许LCD显示
 */
bool system_can_update_lcd(void)
{
    // 在拍照上传期间禁用LCD显示，避免SPI冲突
    return (g_system_state.current_mode == SYSTEM_MODE_FACE_DETECTION);
}

/**
 * @brief 获取当前系统模式
 */
system_mode_t system_get_current_mode(void)
{
    return g_system_state.current_mode;
}

/**
 * @brief 检查是否可以进行人脸识别
 */
bool system_can_do_face_detection(void)
{
    return (g_system_state.current_mode == SYSTEM_MODE_FACE_DETECTION && 
            !g_system_state.face_detection_paused);
}

/**
 * @brief 检查是否需要进行拍照上传
 */
bool system_need_photo_upload(void)
{
    return g_system_state.photo_upload_requested;
}

/**
 * @brief 系统状态管理任务处理函数
 */
void system_state_task_handler(void)
{
    static uint32_t last_log_time = 0;
    uint32_t current_time = esp_timer_get_time() / 1000;
    
    // 检查报警超时（拍照上传期间的安全措施）
    if (g_system_state.alarm_timeout_enabled) {
        if (current_time - g_system_state.alarm_start_timestamp > 10000) { // 10秒超时
            ESP_LOGW(TAG, "⏰ Alarm timeout - automatically stopping buzzer");
            printf("⏰ Alarm timeout - stopping buzzer for safety\r\n");
            buzzer_alarm(0);
            g_system_state.alarm_timeout_enabled = false;
        }
    }
    
    switch (g_system_state.current_mode) {
        case SYSTEM_MODE_FACE_DETECTION:
            // 正常人脸识别模式，无需特殊处理
            break;
            
        case SYSTEM_MODE_TRANSITIONING:
            // 模式切换中，检查是否需要执行拍照上传
            if (g_system_state.photo_upload_requested && !g_system_state.photo_upload_in_progress) {
                ESP_LOGI(TAG, "🔄 Starting photo upload process...");
                printf("🚫 LCD display DISABLED for SPI exclusive access\r\n");
                g_system_state.current_mode = SYSTEM_MODE_PHOTO_UPLOAD;
                g_system_state.photo_upload_in_progress = true;
            }
            break;
            
        case SYSTEM_MODE_PHOTO_UPLOAD:
            // 执行拍照上传（只执行一次）
            if (g_system_state.photo_upload_in_progress) {
                ESP_LOGI(TAG, "📸 Executing photo capture and upload...");
                esp_err_t upload_ret = capture_and_upload_photo();
                if (upload_ret == ESP_OK) {
                    printf("✅ Photo uploaded successfully! ✅\r\n");
                    ESP_LOGI(TAG, "✅ Photo upload successful");
                } else {
                    printf("❌ Photo upload failed! ❌\r\n");
                    ESP_LOGW(TAG, "❌ Photo upload failed");
                }
                
                // 完成上传，立即切换回人脸识别模式
                g_system_state.photo_upload_in_progress = false;
                printf("🖥️  LCD display RE-ENABLED - resuming normal operation\r\n");
                system_photo_upload_complete();
            }
            break;
            
        default:
            ESP_LOGW(TAG, "Unknown system mode: %d", g_system_state.current_mode);
            // 强制切换回人脸识别模式
            g_system_state.current_mode = SYSTEM_MODE_FACE_DETECTION;
            break;
    }
    
    // 每5秒打印一次状态日志（用于调试）
    if (current_time - last_log_time > 5000) {
        if (g_system_state.current_mode != SYSTEM_MODE_FACE_DETECTION) {
            ESP_LOGD(TAG, "System mode: %d, Photo requested: %d, Face detection paused: %d", 
                     g_system_state.current_mode, 
                     g_system_state.photo_upload_requested,
                     g_system_state.face_detection_paused);
        }
        last_log_time = current_time;
    }
}
