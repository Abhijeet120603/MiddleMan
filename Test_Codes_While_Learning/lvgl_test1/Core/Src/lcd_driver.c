/*
 * lcd_driver.c
 *
 *  Created on: Aug 18, 2025
 *      Author: abhij
 */


#include "lvgl.h"
#include "stm32f4xx_hal.h"
#include "main.h"

extern LTDC_HandleTypeDef hltdc;
extern DMA2D_HandleTypeDef hdma2d;

#define LCD_FRAME_BUFFER   0xD0000000  // SDRAM start (check linker script)

static lv_disp_drv_t disp_drv;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf1[LV_HOR_RES_MAX * 10];  // Small buffer
static lv_color_t buf2[LV_HOR_RES_MAX * 10];

void lvgl_init_display(void) {
    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, LV_HOR_RES_MAX * 10);

    lv_disp_drv_init(&disp_drv);
    disp_drv.draw_buf = &draw_buf;
    disp_drv.flush_cb = my_flush_cb;
    disp_drv.hor_res = 240;
    disp_drv.ver_res = 320;
    lv_disp_drv_register(&disp_drv);
}

void my_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t x1 = area->x1;
    uint32_t y1 = area->y1;
    uint32_t x2 = area->x2;
    uint32_t y2 = area->y2;

    // Copy to LCD frame buffer using DMA2D
    for(uint32_t y = y1; y <= y2; y++) {
        uint32_t *dst = (uint32_t*)(LCD_FRAME_BUFFER + 4*(y * drv->hor_res + x1));
        for(uint32_t x = x1; x <= x2; x++) {
            *dst++ = color_p->full;
            color_p++;
        }
    }
    lv_disp_flush_ready(drv);
}
