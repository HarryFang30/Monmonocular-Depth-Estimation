/**
 ****************************************************************************************************
 * @file        fvoid handle_distance_detection_c(void* results)
{
    printf("=== Distance detection started ===\r\n");
    
    // 重置no_face标志，因为现在检测到了人脸
    no_face_logged = false;
    
    if (!g_distance_detector_handle) {
        printf("Distance detector not initialized!\r\n");
        return;
    }ance_c_interface.cpp
 * @author      AI Assistant
 * @version     V1.0
 * @date        2025-07-24
 * @brief       人脸距离检测系统C接口实现
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 */

#include "face_distance_c_interface.h"
#include "face_distance_detector.hpp"
#include "dl_detect_define.hpp"
#include "esp_log.h"
#include "buzzer.h"
#include "photo_uploader.h"
#include "system_state_manager.h"

static const char *TAG = "FaceDistanceC";

/* 全局距离检测器指针 */
void* g_distance_detector_handle = nullptr;

// 标定相关变量
static bool calibration_requested = false;
static int calibration_frames_collected = 0;

// 状态跟踪变量 - 在函数间共享
static face_distance_state_t last_alarm_state = FACE_DISTANCE_SAFE;
static bool no_face_logged = false;

/**
 * @brief 初始化距离检测系统
 */
esp_err_t init_distance_detection_system(void)
{
    if (g_distance_detector_handle == nullptr) {
        FaceDistanceDetector* detector = new FaceDistanceDetector();
        if (detector->init() != ESP_OK) {
            ESP_LOGE(TAG, "Failed to initialize distance detector");
            delete detector;
            return ESP_FAIL;
        } else {
            g_distance_detector_handle = detector;
            ESP_LOGI(TAG, "Distance detector initialized successfully");
            return ESP_OK;
        }
    }
    return ESP_OK;
}

/**
 * @brief 销毁距离检测系统
 */
void deinit_distance_detection_system(void)
{
    if (g_distance_detector_handle != nullptr) {
        FaceDistanceDetector* detector = static_cast<FaceDistanceDetector*>(g_distance_detector_handle);
        delete detector;
        g_distance_detector_handle = nullptr;
    }
}

/**
 * @brief 检查是否已标定
 */
bool is_distance_calibrated(void)
{
    if (g_distance_detector_handle == nullptr) {
        return false;
    }
    FaceDistanceDetector* detector = static_cast<FaceDistanceDetector*>(g_distance_detector_handle);
    return detector->isCalibrated();
}

/**
 * @brief 开始距离标定
 */
void start_distance_calibration(void)
{
    if (g_distance_detector_handle == nullptr) {
        ESP_LOGW(TAG, "Distance detector not initialized");
        return;
    }
    
    FaceDistanceDetector* detector = static_cast<FaceDistanceDetector*>(g_distance_detector_handle);
    
    ESP_LOGI(TAG, "\r\n=== STARTING DISTANCE CALIBRATION ===");
    ESP_LOGI(TAG, "Please follow these steps:");
    ESP_LOGI(TAG, "1. Sit directly in front of the camera");
    ESP_LOGI(TAG, "2. Keep your face unobstructed");
    ESP_LOGI(TAG, "3. Maintain exactly 50 cm distance from camera");
    ESP_LOGI(TAG, "4. Stay still for calibration (20 frames needed)");
    ESP_LOGI(TAG, "=====================================\r\n");
    
    if (detector->startCalibration() == ESP_OK) {
        calibration_requested = true;
        calibration_frames_collected = 0;
    } else {
        ESP_LOGE(TAG, "Failed to start calibration");
    }
}

/**
 * @brief 重置距离标定
 */
void reset_distance_calibration(void)
{
    if (g_distance_detector_handle == nullptr) {
        ESP_LOGW(TAG, "Distance detector not initialized");
        return;
    }
    
    FaceDistanceDetector* detector = static_cast<FaceDistanceDetector*>(g_distance_detector_handle);
    
    if (detector->resetCalibration() == ESP_OK) {
        ESP_LOGI(TAG, "Distance calibration reset successfully");
        calibration_requested = false;
        calibration_frames_collected = 0;
    } else {
        ESP_LOGE(TAG, "Failed to reset calibration");
    }
}

/**
 * @brief 处理人脸检测结果进行距离检测
 */
void handle_distance_detection_c(void* results)
{
    printf("=== Distance detection called ===\r\n");
    
    if (g_distance_detector_handle == nullptr) {
        printf("Distance detector not initialized!\r\n");
        return;
    }
    
    FaceDistanceDetector* detector = static_cast<FaceDistanceDetector*>(g_distance_detector_handle);
    std::list<dl::detect::result_t>* detect_results = static_cast<std::list<dl::detect::result_t>*>(results);
    
    printf("Processing %d faces for distance detection\r\n", (int)detect_results->size());
    
    // 处理标定
    if (calibration_requested && !detect_results->empty()) {
        printf("Calibration mode: processing frame\r\n");
        const auto& face = detect_results->front();
        if (face.keypoint.size() >= 10) {
            printf("Face has %d keypoints\r\n", (int)face.keypoint.size());
            if (detector->addCalibrationFrame(face.keypoint)) {
                calibration_frames_collected++;
                ESP_LOGI(TAG, "Calibration frame %d collected", calibration_frames_collected);
                
                if (calibration_frames_collected >= 20) {
                    if (detector->finishCalibration() == ESP_OK) {
                        ESP_LOGI(TAG, "=== CALIBRATION COMPLETED ===");
                        calibration_requested = false;
                        calibration_frames_collected = 0;
                    } else {
                        ESP_LOGE(TAG, "=== CALIBRATION FAILED ===");
                    }
                }
            } else {
                printf("Failed to add calibration frame\r\n");
            }
        } else {
            printf("Face has insufficient keypoints: %d\r\n", (int)face.keypoint.size());
        }
        return;
    }
    
    // 正常距离检测
    if (detector->isCalibrated()) {
        printf("Detector is calibrated, processing distance...\r\n");
        face_distance_state_t state = detector->processFrame(*detect_results);
        float distance = detector->getCurrentDistance();
        
        printf("Current distance: %.1f cm, state: %d\r\n", distance, (int)state);
        
        if (state != last_alarm_state) {
            printf("=== State change: %d -> %d ===\r\n", (int)last_alarm_state, (int)state);
            if (state == FACE_DISTANCE_TOO_CLOSE) {
                printf("\r\n");
                printf("🚨🚨🚨🚨🚨🚨🚨🚨🚨🚨🚨🚨🚨🚨🚨🚨\r\n");
                printf("⚠️  WARNING: FACE TOO CLOSE! ⚠️\r\n");
                printf("Current Distance: %.1f cm\r\n", distance);
                printf("Safe Distance: > 48 cm\r\n");
                printf("Please move back for eye safety!\r\n");
                printf("🚨🚨🚨🚨🚨🚨🚨🚨🚨🚨🚨🚨🚨🚨🚨🚨\r\n");
                printf("\r\n");
                ESP_LOGW(TAG, "⚠️  WARNING: FACE TOO CLOSE! Distance: %.1f cm ⚠️", distance);
                
                // 开启蜂鸣器报警
                printf("🔊 Activating buzzer alarm... 🔊\r\n");
                buzzer_alarm(1);
                printf("🔊 Buzzer alarm activated! 🔊\r\n");
                
                // 启动报警超时计时（拍照期间的安全措施）
                system_start_alarm_timeout(10000); // 10秒超时
                
                // 请求异步拍照上传（不阻塞当前流程）
                printf("📸 Requesting photo upload (async)... 📸\r\n");
                system_request_photo_upload();
                printf("📸 Photo upload requested successfully 📸\r\n");
                
            } else {
                printf("\r\n");
                printf("✅✅✅✅✅✅✅✅✅✅✅✅✅✅✅✅\r\n");
                printf("✅ Face distance is now SAFE ✅\r\n");
                printf("Current Distance: %.1f cm\r\n", distance);
                printf("✅✅✅✅✅✅✅✅✅✅✅✅✅✅✅✅\r\n");
                printf("\r\n");
                ESP_LOGI(TAG, "✅ Face distance is now safe. Distance: %.1f cm", distance);
                
                // 关闭蜂鸣器报警
                printf("🔇 Deactivating buzzer alarm... 🔇\r\n");
                buzzer_alarm(0);
                printf("🔇 Buzzer alarm deactivated 🔇\r\n");
                
                // 停止报警超时计时
                system_stop_alarm_timeout();
            }
            last_alarm_state = state;
        } else if (state == FACE_DISTANCE_TOO_CLOSE) {
            // 持续警告
            static int warning_counter = 0;
            if (++warning_counter % 10 == 0) {  // 每10帧显示一次持续警告
                printf("⚠️  STILL TOO CLOSE: %.1f cm - Move back! ⚠️\r\n", distance);
            }
        }
    } else {
        static int reminder_counter = 0;
        if (++reminder_counter % 100 == 0) { // 每100帧提醒一次
            printf("Distance detector not calibrated (reminder %d)\r\n", reminder_counter / 100);
            ESP_LOGI(TAG, "Distance detector not calibrated. Use start_distance_calibration() to calibrate.");
        }
    }
    
    printf("=== Distance detection finished ===\r\n");
}

/**
 * @brief 处理没有检测到人脸的情况
 * 重置状态并关闭蜂鸣器
 */
void handle_no_face_detected_c(void)
{
    static int no_face_counter = 0;
    
    // 记录没有检测到人脸
    if (!no_face_logged) {
        printf("No face detected - resetting alarm state\r\n");
        ESP_LOGI(TAG, "No face detected - checking alarm state");
        no_face_logged = true;
        no_face_counter = 0;
    }
    
    no_face_counter++;
    
    // 如果之前是警报状态，现在关闭蜂鸣器
    if (last_alarm_state == FACE_DISTANCE_TOO_CLOSE) {
        printf("\r\n");
        printf("🔇🔇🔇🔇🔇🔇🔇🔇🔇🔇🔇🔇🔇🔇🔇🔇\r\n");
        printf("🔇 NO FACE DETECTED - ALARM OFF 🔇\r\n");
        printf("Face left camera view, deactivating alarm\r\n");
        printf("🔇🔇🔇🔇🔇🔇🔇🔇🔇🔇🔇🔇🔇🔇🔇🔇\r\n");
        printf("\r\n");
        ESP_LOGI(TAG, "🔇 Face left camera view - deactivating alarm");
        
        // 关闭蜂鸣器报警
        printf("🔇 Deactivating buzzer alarm (no face)... 🔇\r\n");
        buzzer_alarm(0);
        printf("🔇 Buzzer alarm deactivated (no face) 🔇\r\n");
        
        last_alarm_state = FACE_DISTANCE_SAFE;
    }
    
    // 每50帧提醒一次没有检测到人脸
    if (no_face_counter % 50 == 0) {
        printf("No face detected for %d frames\r\n", no_face_counter);
    }
}
