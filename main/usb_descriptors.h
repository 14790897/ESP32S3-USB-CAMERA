#pragma once

#include "tusb.h"
#include "class/video/video.h"

#ifdef __cplusplus
extern "C" {
#endif

// Video streaming endpoint
#define EPNUM_VIDEO_IN    0x81

// Video Interface numbers
#define ITF_NUM_VIDEO_CONTROL     0
#define ITF_NUM_VIDEO_STREAMING   1
#define ITF_NUM_TOTAL             2

// UVC descriptor for USB Video Class
#define TUD_VIDEO_CAPTURE_DESC_UNCOMPR_LEN (\
  TUD_VIDEO_DESC_IAD_LEN +\
  TUD_VIDEO_DESC_STD_VC_LEN +\
  (TUD_VIDEO_DESC_CS_VC_LEN + 1) +\
  TUD_VIDEO_DESC_CAMERA_TERM_LEN +\
  TUD_VIDEO_DESC_OUTPUT_TERM_LEN +\
  TUD_VIDEO_DESC_STD_VS_LEN +\
  (TUD_VIDEO_DESC_CS_VS_IN_LEN + 1) +\
  TUD_VIDEO_DESC_CS_FMT_MJPEG_LEN +\
  TUD_VIDEO_DESC_CS_FRM_MJPEG_CONT_LEN +\
  TUD_VIDEO_DESC_CS_COLORFORMAT_LEN +\
  TUD_VIDEO_DESC_EP_BULK_IN_LEN\
)

#define TUD_VIDEO_CAPTURE_DESC_UNCOMPR(itfnum, stridx, epin, epsize) \
  TUD_VIDEO_DESC_IAD(itfnum, 2, stridx),\
  TUD_VIDEO_DESC_STD_VC(itfnum, 0, stridx),\
  TUD_VIDEO_DESC_CS_VC(1),\
  TUD_VIDEO_DESC_CAMERA_TERM(1, 0, 0, epin),\
  TUD_VIDEO_DESC_OUTPUT_TERM(2, VIDEO_TT_STREAMING, 1, 1, 0),\
  TUD_VIDEO_DESC_STD_VS(itfnum + 1, 0, 0, stridx),\
  TUD_VIDEO_DESC_CS_VS_IN(1, 1, 1),\
  TUD_VIDEO_DESC_CS_FMT_MJPEG(1, 1),\
  TUD_VIDEO_DESC_CS_FRM_MJPEG_CONT(1, 640, 480, 666667, 5000000, 333333),\
  TUD_VIDEO_DESC_CS_COLORFORMAT,\
  TUD_VIDEO_DESC_EP_BULK_IN(epin, epsize, 1)

// USB Device Descriptor
static const tusb_desc_device_t desc_device = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = TUSB_CLASS_MISC,
    .bDeviceSubClass    = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol    = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor           = 0x303A, // Espressif VID
    .idProduct          = 0x4002,
    .bcdDevice          = 0x0100,
    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,
    .bNumConfigurations = 0x01
};

// Configuration Descriptor
static const uint8_t desc_configuration[] = {
    // Configuration number, interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESC(1, ITF_NUM_TOTAL, 0, TUD_VIDEO_CAPTURE_DESC_UNCOMPR_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 500),

    // UVC Descriptor
    TUD_VIDEO_CAPTURE_DESC_UNCOMPR(ITF_NUM_VIDEO_CONTROL, 4, EPNUM_VIDEO_IN, 64)
};

// String Descriptors
static const char* string_desc_arr [] = {
    (const char[]) { 0x09, 0x04 }, // 0: is supported language is English (0x0409)
    "Espressif",                   // 1: Manufacturer
    "ESP32-S3 UVC Camera",         // 2: Product
    "123456",                      // 3: Serials
    "UVC",                         // 4: UVC Interface
};

#ifdef __cplusplus
}
#endif