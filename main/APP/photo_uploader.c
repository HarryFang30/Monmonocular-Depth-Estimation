#include "photo_uploader.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "esp_camera.h"
#include "camera.h"

static const char *TAG = "PhotoUploader";

/* WiFi连接状态 */
static bool wifi_connected = false;
static EventGroupHandle_t wifi_event_group;
const int WIFI_CONNECTED_BIT = BIT0;

/**
 * @brief WiFi事件处理函数
 */
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                              int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_connected = false;
        ESP_LOGI(TAG, "WiFi disconnected, retry connecting...");
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP:" IPSTR, IP2STR(&event->ip_info.ip));
        wifi_connected = true;
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

/**
 * @brief 初始化WiFi连接
 */
esp_err_t wifi_init(void)
{
    wifi_event_group = xEventGroupCreate();
    
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi initialization finished.");
    
    /* 等待连接 */
    EventBits_t bits = xEventGroupWaitBits(wifi_event_group,
                                          WIFI_CONNECTED_BIT,
                                          pdFALSE,
                                          pdFALSE,
                                          portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Connected to AP SSID:%s", WIFI_SSID);
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "Failed to connect to WiFi");
        return ESP_FAIL;
    }
}

/**
 * @brief 检查WiFi连接状态
 */
bool wifi_is_connected(void)
{
    return wifi_connected;
}

/**
 * @brief HTTP事件处理函数
 */
esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGI(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGI(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
        case HTTP_EVENT_REDIRECT:
            ESP_LOGI(TAG, "HTTP_EVENT_REDIRECT");
            break;
    }
    return ESP_OK;
}

/**
 * @brief 拍照并上传到服务器
 */
esp_err_t capture_and_upload_photo(void)
{
    // 检查WiFi连接状态
    if (!wifi_connected) {
        ESP_LOGW(TAG, "WiFi not connected, cannot upload photo");
        return ESP_FAIL;
    }
    
    // 双重检查WiFi状态
    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) != ESP_OK) {
        ESP_LOGW(TAG, "WiFi connection lost, cannot upload photo");
        wifi_connected = false;
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Capturing photo for upload...");
    
    // 临时切换到JPEG格式 - 使用重新初始化方法确保格式正确
    ESP_LOGI(TAG, "Deinitializing camera for format switch...");
    esp_camera_deinit();
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // 重新初始化摄像头为JPEG格式
    // 从BSP配置获取基础设置
    extern camera_config_t camera_config; // 从camera.c引用配置
    camera_config_t jpeg_config = camera_config;
    jpeg_config.pixel_format = PIXFORMAT_JPEG;
    jpeg_config.frame_size = FRAMESIZE_VGA;
    jpeg_config.jpeg_quality = 6;
    
    esp_err_t err = esp_camera_init(&jpeg_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to reinitialize camera for JPEG: %s", esp_err_to_name(err));
        // 尝试恢复原始配置
        esp_camera_init(&camera_config);
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Camera reinitialized with JPEG format (quality: 6, framesize: VGA)");
    
    // 等待摄像头稳定 - 增加更长的等待时间
    ESP_LOGI(TAG, "Waiting for camera to stabilize...");
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // 清空多个帧缓冲，确保获取稳定的帧
    for (int i = 0; i < 3; i++) {
        camera_fb_t *temp_fb = esp_camera_fb_get();
        if (temp_fb) {
            ESP_LOGI(TAG, "Discarding frame %d: size=%zu, format=%d", i+1, temp_fb->len, temp_fb->format);
            esp_camera_fb_return(temp_fb);
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
    
    // 获取摄像头帧，增加重试机制和调试信息
    camera_fb_t *fb = NULL;
    int retry_count = 0;
    const int MAX_RETRIES = 5; // 增加重试次数
    
    ESP_LOGI(TAG, "Starting photo capture...");
    while (retry_count < MAX_RETRIES && fb == NULL) {
        fb = esp_camera_fb_get();
        if (!fb) {
            ESP_LOGW(TAG, "Camera capture failed, retry %d/%d", retry_count + 1, MAX_RETRIES);
            vTaskDelay(pdMS_TO_TICKS(200)); // 增加重试间隔
            retry_count++;
        } else {
            ESP_LOGI(TAG, "Photo captured on attempt %d, size: %zu bytes", retry_count + 1, fb->len);
        }
    }
    
    // 恢复原始摄像头配置
    ESP_LOGI(TAG, "Restoring original camera configuration...");
    esp_camera_deinit();
    vTaskDelay(pdMS_TO_TICKS(100));
    esp_camera_init(&camera_config);
    ESP_LOGI(TAG, "Original camera configuration restored");
    
    if (!fb) {
        ESP_LOGE(TAG, "Camera capture failed after %d retries", MAX_RETRIES);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Photo captured, size: %zu bytes, format: %d, width: %d, height: %d", 
             fb->len, fb->format, fb->width, fb->height);
    
    // 验证JPEG格式
    if (fb->format != PIXFORMAT_JPEG) {
        ESP_LOGW(TAG, "Warning: Expected JPEG format but got format %d", fb->format);
    } else {
        ESP_LOGI(TAG, "Confirmed JPEG format");
    }
        
        // 验证JPEG头部
        if (fb->len > 2 && fb->buf[0] == 0xFF && fb->buf[1] == 0xD8) {
            ESP_LOGI(TAG, "Valid JPEG header detected");
        } else {
            ESP_LOGW(TAG, "Invalid JPEG header: 0x%02X 0x%02X", fb->buf[0], fb->buf[1]);
        }
        
        // 检查图片格式和大小
        if (fb->len == 0 || fb->len > 2000000) { // 限制最大2MB，适应高质量照片
            ESP_LOGE(TAG, "Invalid photo size: %zu bytes", fb->len);
            esp_camera_fb_return(fb);
            return ESP_FAIL;
        }

        // 配置HTTP客户端，增加超时时间
        esp_http_client_config_t config = {
            .url = SERVER_URL,
            .event_handler = http_event_handler,
            .method = HTTP_METHOD_POST,
            .timeout_ms = 20000,  // 增加到20秒
            .buffer_size = 4096,
            .buffer_size_tx = 4096,
        };

        esp_http_client_handle_t client = esp_http_client_init(&config);
        if (!client) {
            ESP_LOGE(TAG, "Failed to initialize HTTP client");
            esp_camera_fb_return(fb);
            return ESP_FAIL;
        }

        err = ESP_OK; // 重用之前定义的err变量
        
        // 设置HTTP头
        esp_http_client_set_header(client, "Content-Type", "image/jpeg");
        
        // 正确设置Content-Length头
        char content_length_str[32];
        snprintf(content_length_str, sizeof(content_length_str), "%zu", fb->len);
        esp_http_client_set_header(client, "Content-Length", content_length_str);
        
        // 发送POST请求
        err = esp_http_client_open(client, fb->len);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
            goto cleanup;
        }

        // 分块发送图片数据，避免内存问题
        size_t total_written = 0;
        const size_t chunk_size = 1024; // 1KB chunks
        
        while (total_written < fb->len) {
            size_t write_len = (fb->len - total_written) > chunk_size ? 
                              chunk_size : (fb->len - total_written);
            
            int wlen = esp_http_client_write(client, 
                                            (const char*)(fb->buf + total_written), 
                                            write_len);
            if (wlen < 0) {
                ESP_LOGE(TAG, "Failed to write HTTP data at offset %zu", total_written);
                err = ESP_FAIL;
                goto cleanup;
            }
            
            total_written += wlen;
            
            // 给其他任务一些时间
            if (total_written % (chunk_size * 10) == 0) {
                vTaskDelay(pdMS_TO_TICKS(1));
            }
        }
        
        ESP_LOGI(TAG, "Sent %zu bytes to server", total_written);

        // 完成请求并获取响应
        int content_length = esp_http_client_fetch_headers(client);
        int status_code = esp_http_client_get_status_code(client);
        
        ESP_LOGI(TAG, "HTTP Status: %d, Content-Length: %d", status_code, content_length);

        if (status_code == 200) {
            ESP_LOGI(TAG, "Photo uploaded successfully!");
            err = ESP_OK;
        } else {
            ESP_LOGE(TAG, "Photo upload failed with status: %d", status_code);
            err = ESP_FAIL;
        }

cleanup:
    // 清理资源
    if (client) {
        esp_http_client_cleanup(client);
    }
    if (fb) {
        esp_camera_fb_return(fb);
    }

    return err;
}

/**
 * @brief 初始化照片上传系统
 */
esp_err_t photo_uploader_init(void)
{
    ESP_LOGI(TAG, "Initializing photo uploader system...");
    
    // 初始化WiFi
    esp_err_t ret = wifi_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "WiFi initialization failed");
        return ret;
    }

    ESP_LOGI(TAG, "Photo uploader system initialized successfully");
    return ESP_OK;
}
