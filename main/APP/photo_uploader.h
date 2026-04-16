#ifndef __PHOTO_UPLOADER_H__
#define __PHOTO_UPLOADER_H__

#include "esp_camera.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_http_client.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "wifi_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief WiFi配置 - 在wifi_config.h文件中修改
 */
// WiFi和服务器配置在wifi_config.h文件中定义

/**
 * @brief 初始化WiFi连接
 * @retval ESP_OK: 成功, ESP_FAIL: 失败
 */
esp_err_t wifi_init(void);

/**
 * @brief 检查WiFi连接状态
 * @retval true: 已连接, false: 未连接
 */
bool wifi_is_connected(void);

/**
 * @brief 拍照并上传到服务器
 * @retval ESP_OK: 成功, ESP_FAIL: 失败
 */
esp_err_t capture_and_upload_photo(void);

/**
 * @brief 初始化照片上传系统
 * @retval ESP_OK: 成功, ESP_FAIL: 失败
 */
esp_err_t photo_uploader_init(void);

#ifdef __cplusplus
}
#endif

#endif /* __PHOTO_UPLOADER_H__ */
