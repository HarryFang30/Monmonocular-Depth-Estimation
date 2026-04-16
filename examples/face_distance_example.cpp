/**
 ****************************************************************************************************
 * @file        face_distance_example.cpp
 * @author      AI Assistant  
 * @version     V1.0
 * @date        2025-07-24
 * @brief       人脸距离检测系统使用示例
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 */

#include "face_distance_detector.hpp"
#include "esp_log.h"

static const char *TAG = "FaceDistanceExample";

/**
 * @brief 基础使用示例
 */
void basic_usage_example()
{
    ESP_LOGI(TAG, "=== Basic Usage Example ===");
    
    // 1. 创建检测器实例
    FaceDistanceDetector detector;
    
    // 2. 初始化检测器
    if (detector.init() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize detector");
        return;
    }
    
    // 3. 检查是否需要标定
    if (!detector.isCalibrated()) {
        ESP_LOGI(TAG, "Detector needs calibration");
        
        // 开始标定流程
        detector.startCalibration();
        ESP_LOGI(TAG, "Please position face 50cm from camera and stay still...");
        
        // 在实际应用中，这里会在人脸检测循环中收集标定数据
        // detector.addCalibrationFrame(keypoints);
        // detector.finishCalibration();
    }
    
    // 4. 在主循环中使用
    // std::list<dl::detect::result_t> face_results = get_face_detection_results();
    // face_distance_state_t state = detector.processFrame(face_results);
    
    ESP_LOGI(TAG, "Basic usage example completed");
}

/**
 * @brief 高级配置示例
 */
void advanced_configuration_example()
{
    ESP_LOGI(TAG, "=== Advanced Configuration Example ===");
    
    FaceDistanceDetector detector;
    detector.init();
    
    // 自定义阈值
    detector.setThresholds(25.0f, 28.0f);  // 更严格的距离控制
    ESP_LOGI(TAG, "Set custom thresholds: enter=25cm, exit=28cm");
    
    // 重置标定（如果需要重新标定）
    if (detector.isCalibrated()) {
        ESP_LOGI(TAG, "Resetting previous calibration...");
        detector.resetCalibration();
    }
    
    ESP_LOGI(TAG, "Advanced configuration completed");
}

/**
 * @brief 完整应用示例（模拟主循环）
 */
void complete_application_example()
{
    ESP_LOGI(TAG, "=== Complete Application Example ===");
    
    FaceDistanceDetector detector;
    detector.init();
    
    // 模拟的人脸关键点数据（实际应用中来自ESP-DL）
    std::vector<int> mock_keypoints = {
        100, 80,   // 左眼 (x, y)
        110, 85,   // 嘴巴左侧
        120, 90,   // 鼻子
        140, 80,   // 右眼
        130, 85    // 嘴巴右侧
    };
    
    // 如果未标定，进行标定
    if (!detector.isCalibrated()) {
        ESP_LOGI(TAG, "Starting calibration process...");
        detector.startCalibration();
        
        // 模拟收集20帧标定数据
        for (int i = 0; i < 20; i++) {
            if (detector.addCalibrationFrame(mock_keypoints)) {
                ESP_LOGI(TAG, "Calibration frame %d collected", i + 1);
            }
        }
        
        if (detector.finishCalibration() == ESP_OK) {
            ESP_LOGI(TAG, "Calibration completed successfully!");
        }
    }
    
    // 模拟主检测循环
    std::list<dl::detect::result_t> mock_results;
    dl::detect::result_t mock_face;
    mock_face.keypoint = mock_keypoints;
    mock_results.push_back(mock_face);
    
    for (int frame = 0; frame < 100; frame++) {
        // 处理当前帧
        face_distance_state_t state = detector.processFrame(mock_results);
        float current_distance = detector.getCurrentDistance();
        
        // 根据状态执行相应动作
        static face_distance_state_t last_state = FACE_DISTANCE_SAFE;
        if (state != last_state) {
            switch (state) {
                case FACE_DISTANCE_TOO_CLOSE:
                    ESP_LOGW(TAG, "⚠️ WARNING: Face too close! Distance: %.1f cm", current_distance);
                    // 在这里添加警告LED、蜂鸣器等
                    break;
                    
                case FACE_DISTANCE_SAFE:
                    ESP_LOGI(TAG, "✅ Face distance safe. Distance: %.1f cm", current_distance);
                    // 在这里关闭警告设备
                    break;
            }
            last_state = state;
        }
        
        // 模拟帧间延时
        vTaskDelay(pdMS_TO_TICKS(33)); // ~30 FPS
    }
    
    ESP_LOGI(TAG, "Complete application example finished");
}

/**
 * @brief 多用户场景示例
 */
void multi_user_scenario_example()
{
    ESP_LOGI(TAG, "=== Multi-User Scenario Example ===");
    
    FaceDistanceDetector detector;
    detector.init();
    
    // 用户A的标定
    ESP_LOGI(TAG, "Calibrating for User A...");
    if (!detector.isCalibrated()) {
        // 执行用户A的标定流程
        ESP_LOGI(TAG, "Please User A sit 50cm from camera...");
        // ... 标定代码 ...
    }
    
    // 使用一段时间后，切换到用户B
    ESP_LOGI(TAG, "Switching to User B...");
    detector.resetCalibration();  // 清除用户A的标定数据
    
    ESP_LOGI(TAG, "Calibrating for User B...");
    // 执行用户B的标定流程
    // ... 标定代码 ...
    
    ESP_LOGI(TAG, "Multi-user scenario example completed");
}

/**
 * @brief 错误处理示例
 */
void error_handling_example()
{
    ESP_LOGI(TAG, "=== Error Handling Example ===");
    
    FaceDistanceDetector detector;
    
    // 初始化错误处理
    if (detector.init() != ESP_OK) {
        ESP_LOGE(TAG, "Detector initialization failed!");
        ESP_LOGI(TAG, "Possible solutions:");
        ESP_LOGI(TAG, "1. Check NVS partition");
        ESP_LOGI(TAG, "2. Verify memory availability");
        ESP_LOGI(TAG, "3. Reset device");
        return;
    }
    
    // 标定错误处理
    if (!detector.isCalibrated()) {
        ESP_LOGW(TAG, "Detector not calibrated");
        
        if (detector.startCalibration() != ESP_OK) {
            ESP_LOGE(TAG, "Failed to start calibration!");
            return;
        }
        
        // 模拟标定失败场景
        std::vector<int> invalid_keypoints = {1, 2, 3}; // 不足的关键点数据
        
        bool calibration_complete = false;
        for (int attempt = 0; attempt < 3 && !calibration_complete; attempt++) {
            ESP_LOGI(TAG, "Calibration attempt %d", attempt + 1);
            
            for (int frame = 0; frame < 25; frame++) {
                if (detector.addCalibrationFrame(invalid_keypoints)) {
                    ESP_LOGI(TAG, "Calibration data collected");
                    break;
                }
            }
            
            if (detector.finishCalibration() == ESP_OK) {
                calibration_complete = true;
                ESP_LOGI(TAG, "Calibration successful on attempt %d", attempt + 1);
            } else {
                ESP_LOGW(TAG, "Calibration attempt %d failed, retrying...", attempt + 1);
                detector.resetCalibration();
                detector.startCalibration();
            }
        }
        
        if (!calibration_complete) {
            ESP_LOGE(TAG, "All calibration attempts failed!");
            ESP_LOGI(TAG, "Please check:");
            ESP_LOGI(TAG, "1. Face is clearly visible");
            ESP_LOGI(TAG, "2. Lighting conditions");
            ESP_LOGI(TAG, "3. Camera focus");
        }
    }
    
    ESP_LOGI(TAG, "Error handling example completed");
}

/**
 * @brief 性能测试示例
 */
void performance_test_example()
{
    ESP_LOGI(TAG, "=== Performance Test Example ===");
    
    FaceDistanceDetector detector;
    detector.init();
    
    // 准备测试数据
    std::vector<int> test_keypoints = {100, 80, 110, 85, 120, 90, 140, 80, 130, 85};
    std::list<dl::detect::result_t> test_results;
    dl::detect::result_t test_face;
    test_face.keypoint = test_keypoints;
    test_results.push_back(test_face);
    
    // 性能测试
    const int test_frames = 1000;
    int64_t start_time = esp_timer_get_time();
    
    for (int i = 0; i < test_frames; i++) {
        detector.processFrame(test_results);
    }
    
    int64_t end_time = esp_timer_get_time();
    float avg_time_ms = (end_time - start_time) / 1000.0f / test_frames;
    float fps = 1000.0f / avg_time_ms;
    
    ESP_LOGI(TAG, "Performance Results:");
    ESP_LOGI(TAG, "- Frames processed: %d", test_frames);
    ESP_LOGI(TAG, "- Average time per frame: %.2f ms", avg_time_ms);
    ESP_LOGI(TAG, "- Theoretical FPS: %.1f", fps);
    ESP_LOGI(TAG, "- Memory usage: ~%d KB", heap_caps_get_free_size(MALLOC_CAP_DEFAULT) / 1024);
    
    ESP_LOGI(TAG, "Performance test completed");
}

/**
 * @brief 主示例函数
 */
extern "C" void face_distance_examples_main()
{
    ESP_LOGI(TAG, "Starting Face Distance Detection Examples...");
    
    // 运行各种示例
    basic_usage_example();
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    advanced_configuration_example();
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    complete_application_example();
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    multi_user_scenario_example();
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    error_handling_example();
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    performance_test_example();
    
    ESP_LOGI(TAG, "All examples completed!");
}
