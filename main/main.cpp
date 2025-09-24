/*
 * SPDX-FileCopyrightText: 2022-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdio.h>
#include <unistd.h>
#include <string.h>

extern "C" {
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "esp_camera.h"
#include "tusb.h"
#include "class/video/video.h"
#include "esp_heap_caps.h"
}
#include "usb_descriptors.h"

static const char *TAG = "USB_UVC_CAMERA";

// TinyUSB definitions
#define BOARD_TUD_RHPORT   0

// OV2640 GPIO configuration for DVP interface
#define PWDN_GPIO_NUM -1
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 15
#define SIOD_GPIO_NUM 4
#define SIOC_GPIO_NUM 5

#define Y2_GPIO_NUM 11
#define Y3_GPIO_NUM 9
#define Y4_GPIO_NUM 8
#define Y5_GPIO_NUM 10
#define Y6_GPIO_NUM 12
#define Y7_GPIO_NUM 18
#define Y8_GPIO_NUM 17
#define Y9_GPIO_NUM 16

#define VSYNC_GPIO_NUM 6
#define HREF_GPIO_NUM 7
#define PCLK_GPIO_NUM 13

// Camera configuration
static camera_config_t camera_config = {
    .pin_pwdn = PWDN_GPIO_NUM,
    .pin_reset = RESET_GPIO_NUM,
    .pin_xclk = XCLK_GPIO_NUM,
    .pin_sccb_sda = SIOD_GPIO_NUM,
    .pin_sccb_scl = SIOC_GPIO_NUM,

    .pin_d7 = Y9_GPIO_NUM,
    .pin_d6 = Y8_GPIO_NUM,
    .pin_d5 = Y7_GPIO_NUM,
    .pin_d4 = Y6_GPIO_NUM,
    .pin_d3 = Y5_GPIO_NUM,
    .pin_d2 = Y4_GPIO_NUM,
    .pin_d1 = Y3_GPIO_NUM,
    .pin_d0 = Y2_GPIO_NUM,
    .pin_vsync = VSYNC_GPIO_NUM,
    .pin_href = HREF_GPIO_NUM,
    .pin_pclk = PCLK_GPIO_NUM,

    .xclk_freq_hz = 20000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_JPEG,
    .frame_size = FRAMESIZE_VGA,
    .jpeg_quality = 10, // Better quality to ensure proper JPEG headers
    .fb_count = 1,      // Reduce to single buffer
    .fb_location = CAMERA_FB_IN_PSRAM,
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
    .sccb_i2c_port = 1, // Default I2C port
};

// UVC related variables
static bool uvc_streaming = false;
static camera_fb_t *current_fb = NULL;
static SemaphoreHandle_t frame_ready_sem = NULL;

// JPEG validation function
static bool is_valid_jpeg(const uint8_t *data, size_t len)
{
    if (len < 4)
        return false;

    // Check for JPEG SOI marker (0xFFD8)
    if (data[0] != 0xFF || data[1] != 0xD8)
    {
        return false;
    }

    // Check for EOI marker (0xFFD9) at the end
    if (data[len - 2] != 0xFF || data[len - 1] != 0xD9)
    {
        return false;
    }

    return true;
}

// UVC format descriptors
static const uint8_t desc_uvc_format[] = {
    // Format descriptor (MJPEG)
    TUD_VIDEO_DESC_CS_VS_FMT_MJPEG(1, 1, 1, 1, 0, 0, 0, 0),
    // Frame descriptor (VGA: 640x480)
    TUD_VIDEO_DESC_CS_VS_FRM_MJPEG_CONT(1, 0, 640, 480, 640*480*16, 640*480*16*30, 640*480*2, 333333, 333333, 1000000, 333333),
};

// Initialize camera
static esp_err_t init_camera(void)
{
    ESP_LOGI(TAG, "Initializing camera...");
    
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Camera init failed with error 0x%x", err);
        return err;
    }

    // Let camera settle
    vTaskDelay(pdMS_TO_TICKS(1000));

    sensor_t *s = esp_camera_sensor_get();
    if (s != NULL) {
        // Set initial settings for better JPEG output
        s->set_brightness(s, 0);     // -2 to 2
        s->set_contrast(s, 0);       // -2 to 2
        s->set_saturation(s, 0);     // -2 to 2
        s->set_special_effect(s, 0); // 0 to 6 (0 - No Effect, 1 - Negative, 2 - Grayscale, 3 - Red Tint, 4 - Green Tint, 5 - Blue Tint, 6 - Sepia)
        s->set_whitebal(s, 1);       // 0 = disable , 1 = enable
        s->set_awb_gain(s, 1);       // 0 = disable , 1 = enable
        s->set_wb_mode(s, 0);        // 0 to 4 - if awb_gain enabled (0 - Auto, 1 - Sunny, 2 - Cloudy, 3 - Office, 4 - Home)
        s->set_exposure_ctrl(s, 1);  // 0 = disable , 1 = enable
        s->set_aec2(s, 0);           // 0 = disable , 1 = enable
        s->set_ae_level(s, 0);       // -2 to 2
        s->set_aec_value(s, 300);    // 0 to 1200
        s->set_gain_ctrl(s, 1);      // 0 = disable , 1 = enable
        s->set_agc_gain(s, 0);       // 0 to 30
        s->set_gainceiling(s, (gainceiling_t)0);  // 0 to 6
        s->set_bpc(s, 0);            // 0 = disable , 1 = enable
        s->set_wpc(s, 1);            // 0 = disable , 1 = enable
        s->set_raw_gma(s, 1);        // 0 = disable , 1 = enable
        s->set_lenc(s, 1);           // 0 = disable , 1 = enable
        s->set_hmirror(s, 0);        // 0 = disable , 1 = enable
        s->set_vflip(s, 0);          // 0 = disable , 1 = enable
        s->set_dcw(s, 1);            // 0 = disable , 1 = enable
        s->set_colorbar(s, 0);       // 0 = disable , 1 = enable

        // Try a few test captures to warm up the sensor
        for (int i = 0; i < 3; i++)
        {
            camera_fb_t *fb = esp_camera_fb_get();
            if (fb)
            {
                ESP_LOGI(TAG, "Warmup capture %d: len=%zu, format=%d", i + 1, fb->len, fb->format);
                esp_camera_fb_return(fb);
            }
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }
    
    ESP_LOGI(TAG, "Camera initialized successfully");
    return ESP_OK;
}

// Camera capture task
static void camera_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Camera task started");

    int consecutive_errors = 0;
    const int MAX_CONSECUTIVE_ERRORS = 10;
    int frame_count = 0;

    while (1) {
        if (uvc_streaming) {
            ESP_LOGI(TAG, "Attempting to capture frame %d", frame_count++);
            camera_fb_t *fb = esp_camera_fb_get();
            if (fb) {
                consecutive_errors = 0; // Reset error counter on success
                ESP_LOGI(TAG, "Frame captured successfully: len=%zu, format=%d, width=%d, height=%d",
                         fb->len, fb->format, fb->width, fb->height);

                // Validate the frame
                if (fb->len > 0 && fb->format == PIXFORMAT_JPEG)
                {
                    // Additional JPEG validation
                    if (is_valid_jpeg(fb->buf, fb->len))
                    {
                        ESP_LOGI(TAG, "JPEG validation passed, setting current frame");
                        if (current_fb)
                        {
                            esp_camera_fb_return(current_fb);
                        }
                        current_fb = fb;
                        xSemaphoreGive(frame_ready_sem);
                        ESP_LOGI(TAG, "Frame ready semaphore given");
                    }
                    else
                    {
                        ESP_LOGW(TAG, "Camera provided invalid JPEG data (len=%zu)", fb->len);
                        if (fb->len >= 4)
                        {
                            ESP_LOGW(TAG, "Frame header: 0x%02X 0x%02X 0x%02X 0x%02X",
                                     fb->buf[0], fb->buf[1], fb->buf[2], fb->buf[3]);
                        }
                        esp_camera_fb_return(fb);
                    }
                }
                else
                {
                    ESP_LOGW(TAG, "Camera frame invalid: len=%zu, format=%d", fb->len, fb->format);
                    esp_camera_fb_return(fb);
                }
            }
            else
            {
                consecutive_errors++;
                ESP_LOGW(TAG, "Failed to capture frame (error %d/%d)", consecutive_errors, MAX_CONSECUTIVE_ERRORS);

                if (consecutive_errors >= MAX_CONSECUTIVE_ERRORS)
                {
                    ESP_LOGE(TAG, "Too many consecutive camera errors, restarting camera");
                    esp_camera_deinit();
                    vTaskDelay(pdMS_TO_TICKS(1000));

                    if (esp_camera_init(&camera_config) != ESP_OK)
                    {
                        ESP_LOGE(TAG, "Camera restart failed!");
                        vTaskDelay(pdMS_TO_TICKS(5000));
                    }
                    else
                    {
                        ESP_LOGI(TAG, "Camera restarted successfully");
                        consecutive_errors = 0;
                    }
                }
                vTaskDelay(pdMS_TO_TICKS(100));
            }
        }
        else
        {
            ESP_LOGD(TAG, "UVC not streaming, camera task waiting...");
            vTaskDelay(pdMS_TO_TICKS(1000)); // Longer delay when not streaming
        }
    }
}

// TinyUSB Video Class callbacks
extern "C" void tud_video_frame_complete_cb(uint_fast8_t ctl_idx)
{
    (void)ctl_idx;
    ESP_LOGD(TAG, "Video frame transfer complete");
}

extern "C" int tud_video_commit_cb(uint_fast8_t ctl_idx, uint_fast8_t stm_idx, video_probe_and_commit_control_t const *parameters)
{
    (void)ctl_idx;
    (void)stm_idx;
    (void)parameters;

    ESP_LOGI(TAG, "UVC stream commit - Host requesting video stream start");
    uvc_streaming = true;
    return VIDEO_ERROR_NONE;
}

extern "C" int tud_video_uncomit_cb(uint_fast8_t ctl_idx)
{
    (void)ctl_idx;

    ESP_LOGI(TAG, "UVC stream uncomit - Host stopped video stream");
    uvc_streaming = false;
    return VIDEO_ERROR_NONE;
}

extern "C" void tud_mount_cb(void)
{
    ESP_LOGI(TAG, "USB Device mounted and connected!");
}

extern "C" void tud_umount_cb(void)
{
    ESP_LOGI(TAG, "USB Device unmounted");
}

// TinyUSB descriptor callbacks
extern "C" uint8_t const* tud_descriptor_device_cb(void)
{
    return (uint8_t const*)&desc_device;
}

extern "C" uint8_t const* tud_descriptor_configuration_cb(uint8_t index)
{
    (void)index;
    return desc_configuration;
}

extern "C" uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
    (void)langid;

    static uint16_t desc_str[32 + 1]; // Static buffer for string descriptor
    uint8_t chr_count;
    
    if (index == 0) {
        // Language ID descriptor
        desc_str[0] = (TUSB_DESC_STRING << 8) | (2 + 2);
        desc_str[1] = 0x0409; // English (United States)
        return desc_str;
    } else {
        if (!(index < sizeof(string_desc_arr)/sizeof(string_desc_arr[0]))) return NULL;
        
        const char* str = string_desc_arr[index];
        chr_count = strlen(str);
        if (chr_count > 31) chr_count = 31;

        // First byte is length (in bytes) and type
        desc_str[0] = (TUSB_DESC_STRING << 8) | (2 * chr_count + 2);

        // Convert ASCII to UTF-16
        for (uint8_t i = 0; i < chr_count; i++) {
            desc_str[1 + i] = str[i];
        }

        return desc_str;
    }
}

// UVC streaming task
static void uvc_task(void *pvParameters)
{
    ESP_LOGI(TAG, "UVC task started");
    
    while (1) {
        ESP_LOGD(TAG, "UVC task loop: streaming=%d, tud_video_n_streaming=%d",
                 uvc_streaming, tud_video_n_streaming(0, 0));

        if (uvc_streaming && tud_video_n_streaming(0, 0)) {
            ESP_LOGI(TAG, "UVC streaming active, waiting for frame...");
            if (xSemaphoreTake(frame_ready_sem, pdMS_TO_TICKS(100)) == pdTRUE) {
                if (current_fb && current_fb->len > 0) {
                    static unsigned frame_num = 0;

                    ESP_LOGI(TAG, "Frame received for streaming: len=%zu", current_fb->len);

                    // Validate JPEG data before sending
                    if (is_valid_jpeg(current_fb->buf, current_fb->len))
                    {
                        ESP_LOGI(TAG, "Sending frame %u to USB (len=%zu)", frame_num, current_fb->len);
                        tud_video_n_frame_xfer(0, 0, (void *)(uintptr_t)current_fb->buf, current_fb->len);
                        frame_num++;

                        if (frame_num % 10 == 0)
                        {
                            ESP_LOGI(TAG, "Streamed %u frames successfully", frame_num);
                        }
                    }
                    else
                    {
                        ESP_LOGW(TAG, "Invalid JPEG frame detected, skipping (len=%zu)", current_fb->len);
                        if (current_fb->len >= 4)
                        {
                            ESP_LOGW(TAG, "Frame header: 0x%02X 0x%02X 0x%02X 0x%02X",
                                     current_fb->buf[0], current_fb->buf[1],
                                     current_fb->buf[2], current_fb->buf[3]);
                        }
                    }
                }
                else
                {
                    ESP_LOGW(TAG, "Frame ready but current_fb is null or empty");
                }
            }
            else
            {
                ESP_LOGD(TAG, "No frame available, timeout waiting for semaphore");
            }
        }
        else
        {
            if (!uvc_streaming)
            {
                ESP_LOGD(TAG, "UVC streaming not active");
            }
            if (!tud_video_n_streaming(0, 0))
            {
                ESP_LOGD(TAG, "TinyUSB video not streaming");
            }
            vTaskDelay(pdMS_TO_TICKS(100));
        }

        tud_task();
    }
}

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "USB UVC Camera starting...");
    
    // Create semaphore for frame synchronization
    frame_ready_sem = xSemaphoreCreateBinary();
    if (!frame_ready_sem) {
        ESP_LOGE(TAG, "Failed to create semaphore");
        return;
    }
    
    // Initialize camera
    esp_err_t ret = init_camera();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize camera");
        return;
    }
    
    // Initialize TinyUSB
    ESP_LOGI(TAG, "Initializing USB...");
    
    // Initialize TinyUSB device stack
    ret = ESP_OK;
    if (!tud_init(BOARD_TUD_RHPORT)) {
        ESP_LOGE(TAG, "Failed to initialize TinyUSB device");
        ret = ESP_FAIL;
    }
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install TinyUSB driver");
        return;
    }
    
    // Create camera capture task
    xTaskCreatePinnedToCore(
        camera_task,
        "camera_task",
        4096,
        NULL,
        5,
        NULL,
        1
    );
    
    // Create UVC streaming task
    xTaskCreatePinnedToCore(
        uvc_task,
        "uvc_task",
        4096,
        NULL,
        5,
        NULL,
        0
    );
    
    ESP_LOGI(TAG, "USB UVC Camera initialized successfully");

    int status_counter = 0;
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        status_counter++;

        if (status_counter % 5 == 0)
        {
            ESP_LOGI(TAG, "=== System Status ===");
            ESP_LOGI(TAG, "Free heap: %lu bytes", esp_get_free_heap_size());
            ESP_LOGI(TAG, "USB mounted: %s", tud_mounted() ? "YES" : "NO");
            ESP_LOGI(TAG, "USB ready: %s", tud_ready() ? "YES" : "NO");
            ESP_LOGI(TAG, "UVC streaming: %s", uvc_streaming ? "YES" : "NO");
            ESP_LOGI(TAG, "TinyUSB video streaming: %s", tud_video_n_streaming(0, 0) ? "YES" : "NO");
            ESP_LOGI(TAG, "Current frame buffer: %s", current_fb ? "Available" : "NULL");
            ESP_LOGI(TAG, "==================");
        }
        else
        {
            ESP_LOGD(TAG, "System running... USB: %s, UVC: %s",
                     tud_mounted() ? "Connected" : "Disconnected",
                     uvc_streaming ? "Streaming" : "Idle");
        }
    }
}