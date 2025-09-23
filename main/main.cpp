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
    .jpeg_quality = 12,
    .fb_count = 2,
    .fb_location = CAMERA_FB_IN_PSRAM,
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
};

// UVC related variables
static bool uvc_streaming = false;
static camera_fb_t *current_fb = NULL;
static SemaphoreHandle_t frame_ready_sem = NULL;

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
    
    sensor_t *s = esp_camera_sensor_get();
    if (s != NULL) {
        // Set initial settings
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
    }
    
    ESP_LOGI(TAG, "Camera initialized successfully");
    return ESP_OK;
}

// Camera capture task
static void camera_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Camera task started");
    
    while (1) {
        if (uvc_streaming) {
            camera_fb_t *fb = esp_camera_fb_get();
            if (fb) {
                if (current_fb) {
                    esp_camera_fb_return(current_fb);
                }
                current_fb = fb;
                xSemaphoreGive(frame_ready_sem);
            } else {
                ESP_LOGW(TAG, "Failed to capture frame");
                vTaskDelay(pdMS_TO_TICKS(10));
            }
        } else {
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}

// TinyUSB Video Class callbacks
extern "C" void tud_video_frame_complete_cb(uint_fast8_t ctl_idx)
{
    (void)ctl_idx;
}

extern "C" int tud_video_commit_cb(uint_fast8_t ctl_idx, uint_fast8_t stm_idx, video_probe_and_commit_control_t const *parameters)
{
    (void)ctl_idx;
    (void)stm_idx;
    (void)parameters;
    
    ESP_LOGI(TAG, "UVC stream commit");
    uvc_streaming = true;
    return VIDEO_ERROR_NONE;
}

extern "C" int tud_video_uncomit_cb(uint_fast8_t ctl_idx)
{
    (void)ctl_idx;
    
    ESP_LOGI(TAG, "UVC stream stop");
    uvc_streaming = false;
    return VIDEO_ERROR_NONE;
}

extern "C" void tud_mount_cb(void)
{
    ESP_LOGI(TAG, "USB Device mounted");
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
    
    uint8_t chr_count;
    
    if (index == 0) {
        memcpy(&chr_count, string_desc_arr[0], 1);
        chr_count = 1;
    } else {
        if (!(index < sizeof(string_desc_arr)/sizeof(string_desc_arr[0]))) return NULL;
        
        const char* str = string_desc_arr[index];
        chr_count = strlen(str);
        if (chr_count > 31) chr_count = 31;
        
        for (uint8_t i = 0; i < chr_count; i++) {
            ((uint16_t*)&chr_count)[1 + i] = str[i];
        }
    }
    
    return (uint16_t const*)&chr_count;
}

// UVC streaming task
static void uvc_task(void *pvParameters)
{
    ESP_LOGI(TAG, "UVC task started");
    
    while (1) {
        if (uvc_streaming && tud_video_n_streaming(0, 0)) {
            if (xSemaphoreTake(frame_ready_sem, pdMS_TO_TICKS(100)) == pdTRUE) {
                if (current_fb && current_fb->len > 0) {
                    static unsigned frame_num = 0;
                    
                    tud_video_n_frame_xfer(0, 0, (void*)(uintptr_t)current_fb->buf, current_fb->len);
                    frame_num++;
                    
                    if (frame_num % 30 == 0) {
                        ESP_LOGI(TAG, "Streamed %u frames", frame_num);
                    }
                }
            }
        } else {
            vTaskDelay(pdMS_TO_TICKS(10));
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
    
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        ESP_LOGI(TAG, "System running... Free heap: %lu bytes", esp_get_free_heap_size());
    }
}