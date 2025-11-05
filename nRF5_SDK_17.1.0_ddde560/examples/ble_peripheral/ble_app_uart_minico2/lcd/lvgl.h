/***
 * @Author: tai && tai0105@163.com
 * @Date: 2023-02-08 19:10:30
 * @LastEditors: tai && tai0105@163.com
 * @LastEditTime: 2023-02-09 13:40:32
 * @FilePath: \LTB_2023_1_29_2_8\img\lvgl.h
 * @Description:
 */
/***
 * @Author: tai && tai0105@163.com
 * @Date: 2023-01-30 15:35:34
 * @LastEditors: tai && tai0105@163.com
 * @LastEditTime: 2023-02-01 11:19:44
 * @FilePath: \LTB_2023_1_29\img\lvgl.h
 * @Description: 系统并不使用lvgl，但是图像是用lvgl转换器转换的，因此构建本文件以适配之
 */
#ifndef __LVGL_H__
#define __LVGL_H__

#include <stdint.h>

#include "app_section.h"

// 指定数组在flash运行, LV_ATTRIBUTE_LARGE_CONST为转换器定义但已经不使用的标签
#define LV_ATTRIBUTE_LARGE_CONST APP_FLASH_RODATA_SECTION
#define LV_ATTRIBUTE_MEM_ALIGN APP_FLASH_RODATA_SECTION

// 16比特
#define LV_COLOR_DEPTH 16
// 大小端交换
#define LV_COLOR_16_SWAP 1

#define LV_IMG_CF_TRUE_COLOR 0
#define LV_COLOR_SIZE 16
//100 33 55
#if LV_COLOR_16_SWAP == 1
#define RGB24_TO_RGB565(r, g, b) (((((g) << 3) & 0xE0) | (((b) >> 3) & 0x1f)) << 8) | (((r)&0xF8) | (((g) >> 5) & 0x07))
#define RGB565_TO_R(rgb565) ((rgb565) & 0xF8)
#define RGB565_TO_G(rgb565) ((((rgb565) << 5) & 0xE0) | (((rgb565) >> 11) & 0x1C))
#define RGB565_TO_B(rgb565) (((rgb565) >> 5) & 0xF8)
#else
#define RGB24_TO_RGB565(r, g, b) (((((r)&0xF8) | (((g) >> 5) & 0x07))) << 8) | ((((g) << 3) & 0xE0) | (((b) >> 3) & 0x1F))
#define RGB565_TO_R(rgb565) (((rgb565) >> 8) & 0xF8)
#define RGB565_TO_G(rgb565) (((rgb565) >> 3) & 0xFC)
#define RGB565_TO_B(rgb565) (((rgb565)) & 0x1F)
#endif

typedef struct
{
    uint16_t w;
    uint16_t h;
    uint8_t cf;
    uint8_t always_zero;
    uint8_t reserved;
} lv_img_head_t;

typedef struct
{
    lv_img_head_t header;
    const uint8_t *data;
    uint16_t data_size;
} lv_img_dsc_t;

#endif
