#include "ui.h"

// 内存画布,1bit为1像素，写入显存时再扩展颜色也减小内存消耗
static uint8_t gram[LCD_HIGHT][LCD_WIDTH / 8] = {0};

// 像素设为前景色
#define GRAM_SET_PIX(_row, _col)                         \
    do                                                   \
    {                                                    \
        gram[(_row)][(_col) / 8] |= (1 << ((_col) % 8)); \
    } while (0);
// 像素是否以及设置为前景色
#define GRAM_GET_PIX(_row, _col) (gram[(_row)][(_col) / 8] & (1 << ((_col) % 8)))

// 像素清除为背景
#define GRAM_CLR_PIX(_row, _col)                          \
    do                                                    \
    {                                                     \
        gram[(_row)][(_col) / 8] &= ~(1 << ((_col) % 8)); \
    } while (0);

// SPI发送缓存, 每次写入LCD显存时，最少写入一行
static uint8_t gram_write_buf[LCD_WIDTH] = {0};

// ui状态
// static uint8_t ui_busy = 0;

// 当前绘制/刷新的区域
static ui_rect_t refresh_rect;

// 当前页面 0首页，1校准页
static ui_page_t disp_page = UI_PAGE_MAIN;

/**
 * @brief 返回ui繁忙状态，只有空闲时可以绘制
 *
 * @return uint8_t 0繁忙，1空闲
 */
uint8_t ui_get_idle_state(void)
{
    // return ui_busy;
    return spi_get_usage() == SPI_NOT_USE;
}

/**
 * @brief 给出需要跟新的窗口，实际上会更新屏幕整行
 *
 * @param rect 区域
 */
void ui_set_window(ui_rect_t *rect)
{
    set_window_addr(0, rect->y, LCD_WIDTH / 4, rect->h);
}

// Get color based on CO2 PPM value
static uint8_t get_bar_color(uint16_t ppm)
{
    if (ppm < cfg_fstorage_get_graph_yellow_start())
    {
        return COLOR_GREEN;
    }
    else if (ppm < cfg_fstorage_get_graph_red_start())
    {
        return COLOR_YELLOW;
    }
    else
    {
        return COLOR_RED;
    }
}

/**
 * @brief 根据像素所在位置，获取它的前景色,屏幕颜色简单，仅有少数不规则的前景色，用本函数给出
 *
 * @param x 像素的x坐标
 * @param y 像素的y坐标
 * @return uint8_t 像素的前景色
 */
uint8_t ui_get_pix_forcolor(uint8_t x, uint8_t y)
{
    extern uint8_t  get_ble_state(void);
    extern uint16_t co2_history[BAR_COUNT];

    if (disp_page == UI_PAGE_MAIN)
    {
        if (cfg_fstorage_get_ui_mode() == 0)
        {
            // Check if pixel is in graph area
            if (x >= BAR_START_X &&
                x < LCD_WIDTH &&
                y >= BAR_START_Y &&
                y <= LCD_HIGHT)
            {
                // Find which bar this pixel belongs to
                uint8_t  bar_index = (x - BAR_START_X) / (BAR_WIDTH + BAR_SPACING);
                uint16_t ppm       = co2_history[BAR_COUNT - 1 - bar_index];

                // If CO2 value is 0, don't draw anything (transparent)
                if (ppm == 0)
                {
                    return COLOR_BKG; // Return background color
                }

                // Get the height of the bar
                uint8_t bar_height = ui_get_bar_height_from_ppm(ppm);

                // Calculate pixel position from bottom of graph
                uint8_t pixel_height_from_bottom = BAR_START_Y + BAR_HEIGHT - y;

                // Calculate bar's top position from bottom
                uint8_t bar_top_from_bottom = bar_height;

                // If pixel is within the active part of the bar
                if (pixel_height_from_bottom <= bar_top_from_bottom)
                {
                    return get_bar_color(ppm);
                }
                else
                {
                    // Create a very subtle dotted pattern for the background
                    // Only show a dot every few pixels to create a subtle effect
                    if ((x + y) % 4 == 0)
                    {
                        return COLOR_GRAY; // Show a dot
                    }
                    else
                    {
                        return COLOR_BKG; // Most pixels are background color
                    }
                }
            }
        }
        else if (cfg_fstorage_get_ui_mode() == 1)
        {
            if (INRECT(x, y, &elem_rect_g))
            {
                return COLOR_GREEN;
            }
            if (INRECT(x, y, &elem_rect_y))
            {
                return COLOR_YELLOW;
            }
            if (INRECT(x, y, &elem_rect_r))
            {
                return COLOR_RED;
            }
        }

        if (INRECT(x, y, &elem_ble.rect))
        {
            if (get_ble_state() == 1)
            {
                return COLOR_BLE_CONN;
            }
            else if (get_ble_state() == 2)
            {
                return COLOR_BLE_ADV;
            }
            else
            {
                return COLOR_BLE_DISCON;
            }
        }

        if (INRECT(x, y, &elem_ppm_1000x.rect) || INRECT(x, y, &elem_ppm_100x.rect) || INRECT(x, y, &elem_ppm_10x.rect) || INRECT(x, y, &elem_ppm_1x.rect))
        {
            int16_t ppm = get_co2_value();
            if (ppm == 0) return COLOR_WHITE;

            if (ppm < cfg_fstorage_get_graph_yellow_start())
            {
                return COLOR_GREEN;
            }
            else if (ppm < cfg_fstorage_get_graph_red_start())
            {
                return COLOR_YELLOW;
            }
            else
            {
                return COLOR_RED;
            }
        }
    }
    else if (disp_page == UI_PAGE_COLOR_PLATE) // 调色盘
    {
        return (y / 22) * 8 + x / 22;
    }
    else if (disp_page == UI_PAGE_BAT_LOW)
    {
        if (INRECT(x, y, &elem_bat_low_red.rect))
        {
            // 圆角部分
            // if(x == 17)
            // {
            //     if(y == 68 || y == 69 || y == 105 || y == 106) return COLOR_WHITE;
            // }
            // else if(x == 18)
            // {
            //     if(y == 68 || y == 106) return COLOR_WHITE;
            // }
            return COLOR_RED;
        }
    }
    else if (disp_page == UI_PAGE_POWER_ON)
    {
        if (INRECT(x, y, &logo_g))
        {
            return COLOR_GREEN;
        }
        if (INRECT(x, y, &logo_y))
        {
            return COLOR_YELLOW;
        }
        if (INRECT(x, y, &logo_r))
        {
            return COLOR_RED;
        }
    }

    return COLOR_WHITE;
}

/**将需要跟新的画布部分发送到显存 */
static void ui_refresh_row(void)
{
    if (0 == refresh_rect.h) // 结束
    {
        // ui_busy = 0;
        // spi_config(SPI_NOT_USE);
        // print("ui refresh done\n");
        return;
    }

    for (uint8_t col = 0; col < LCD_WIDTH; col++)
    {
        if (GRAM_GET_PIX(refresh_rect.y, col))
        {
            gram_write_buf[col] = ST7301_COLOR(ui_get_pix_forcolor(col, refresh_rect.y));
        }
        else
        {
            gram_write_buf[col] = ST7301_COLOR(COLOR_BKG);
        }
    }

    // print("lcd write row %d\n", refresh_rect.y);
    refresh_rect.y += 1;
    refresh_rect.h -= 1;
    // print("lcd row reamin %d\n", refresh_rect.h);
    spi_write(gram_write_buf, LCD_WIDTH, ui_refresh_row, 10);
    // spi_write(gram_write_buf, 64, ui_refresh_row, 10);
}

/**
 * @brief 刷新屏幕指定的区域,实际上每次总是刷新屏幕的整行
 *
 * @param rect 刷新区域
 * @return busy 1错误， 0成功
 */
uint8_t ui_refresh(ui_rect_t *rect)
{
    // print("ui refresh\n");

    refresh_rect.y = rect->y;
    refresh_rect.h = rect->h;

    ui_set_window(&refresh_rect);
    ui_refresh_row();

    return 0;
}

/**检查是否刷新完成 */
uint8_t ui_refresh_done(void)
{
    return refresh_rect.h == 0;
}

/**
 * @brief 内存画布清除为背景色
 *
 * @param color 颜色
 */
void ui_draw_bkg(void)
{
    memset(gram, 0, LCD_HIGHT * LCD_WIDTH / 8);
}

/**
 * @brief 绘制元素,仅仅绘制到画布，颜色在写入显存时才赋值
 *
 * @param elem
 */
void ui_draw_elem(ui_elem_t *elem)
{
    for (uint8_t row = 0; row < elem->rect.h; row++)
    {
        for (uint8_t col = 0; col < elem->rect.w; col++)
        {
            if (PIX_IS_SET(elem, row, col))
            {
                GRAM_SET_PIX(row + elem->rect.y, col + elem->rect.x);
            }
            else
            {
                GRAM_CLR_PIX(row + elem->rect.y, col + elem->rect.x);
            }
        }
    }
}

/**
 * @brief 以反色绘制元素
 *
 * @param elem
 */
void ui_draw_elem_inverted(ui_elem_t *elem)
{
    for (uint8_t row = 0; row < elem->rect.h; row++)
    {
        for (uint8_t col = 0; col < elem->rect.w; col++)
        {
            if (PIX_IS_SET(elem, row, col))
            {
                if (GRAM_GET_PIX(row + elem->rect.y, col + elem->rect.x))
                {
                    GRAM_CLR_PIX(row + elem->rect.y, col + elem->rect.x);
                }
                else
                {
                    GRAM_SET_PIX(row + elem->rect.y, col + elem->rect.x);
                }
            }
        }
    }
}

/**
 * @brief 以背景色清除区域
 *
 * @param rect
 */
void ui_undraw_rect(ui_rect_t *rect)
{
    for (uint8_t row = 0; row < rect->h; row++)
    {
        for (uint8_t col = 0; col < rect->w; col++)
        {
            GRAM_CLR_PIX(row + rect->y, col + rect->x);
        }
    }
}

/**
 * @brief 填充区域
 *
 * @param rect
 */
void ui_draw_fill_rect(ui_rect_t *rect)
{
    for (uint8_t row = 0; row < rect->h; row++)
    {
        for (uint8_t col = 0; col < rect->w; col++)
        {
            GRAM_SET_PIX(row + rect->y, col + rect->x);
        }
    }
}

/**
 * @brief 绘制警报图标
 *
 * @param visible 是否显示
 */
void ui_draw_buzzer(ui_bool_t state, ui_bool_t visible)
{
    if (visible)
    {
        if (state)
        {
            elem_alarm.pix = (uint8_t *)pix_alarm;
        }
        else
        {
            elem_alarm.pix = (uint8_t *)pix_alarm_off;
        }
        ui_draw_elem(&elem_alarm);
    }
    else
    {
        ui_undraw_rect(&elem_alarm.rect);
    }
}

/**
 * @brief 绘制震动图标
 *
 * @param visible 是否显示
 */
void ui_draw_vibrator(ui_bool_t state, ui_bool_t visible)
{
    if (visible)
    {
        if (state)
        {
            elem_vibrator.pix = (uint8_t *)pix_vibrator;
        }
        else
        {
            elem_vibrator.pix = (uint8_t *)pix_vibrator_off;
        }
        ui_draw_elem(&elem_vibrator);
    }
    else
    {
        ui_undraw_rect(&elem_vibrator.rect);
    }
}

/**
 * @brief 绘制时间
 *
 * @param hour 小时数
 * @param minute 分钟数
 * @param visible 是否显示
 */
void ui_draw_time(uint8_t hour, uint8_t minute, ui_bool_t visible)
{
    uint8_t h10, h0, m10, m0;
    if (visible)
    {
        h10 = (hour % 100) / 10;
        h0  = hour % 10;
        m10 = (minute % 100) / 10;
        m0  = minute % 10;

        elem_time_h10.pix = (uint8_t *)pix_time_num[h10];
        elem_time_h0.pix  = (uint8_t *)pix_time_num[h0];
        elem_time_m10.pix = (uint8_t *)pix_time_num[m10];
        elem_time_m0.pix  = (uint8_t *)pix_time_num[m0];

        ui_draw_elem(&elem_time_h10);
        ui_draw_elem(&elem_time_h0);
        ui_draw_elem(&elem_time_split);
        ui_draw_elem(&elem_time_m10);
        ui_draw_elem(&elem_time_m0);
    }
    else
    {
        ui_undraw_rect(&elem_time_h10.rect);
        ui_undraw_rect(&elem_time_h0.rect);
        ui_undraw_rect(&elem_time_split.rect);
        ui_undraw_rect(&elem_time_m10.rect);
        ui_undraw_rect(&elem_time_m0.rect);
    }
}

/**
 * @brief 绘制小时
 *
 * @param minute 分钟数
 * @param visible 是否显示
 */
void ui_draw_hour(uint8_t hour, ui_bool_t visible)
{
    uint8_t h10, h0;
    if (visible)
    {
        if (hour == 99)
        {
            elem_time_h10.pix = (uint8_t *)pixdash;
            elem_time_h0.pix  = (uint8_t *)pixdash;

            ui_draw_elem(&elem_time_h10);
            ui_draw_elem(&elem_time_h0);

            return;
        }

        h10 = (hour % 100) / 10;
        h0  = hour % 10;

        elem_time_h10.pix = (uint8_t *)pix_time_num[h10];
        elem_time_h0.pix  = (uint8_t *)pix_time_num[h0];

        ui_draw_elem(&elem_time_h10);
        ui_draw_elem(&elem_time_h0);
        ui_draw_elem(&elem_time_split);
    }
    else
    {
        ui_undraw_rect(&elem_time_h10.rect);
        ui_undraw_rect(&elem_time_h0.rect);
    }
}

/**
 * @brief 绘制分钟
 *
 * @param minute 分钟数
 * @param visible 是否显示
 */
void ui_draw_minute(uint8_t minute, ui_bool_t visible)
{
    uint8_t m10, m0;
    if (visible)
    {
        if (minute == 99)
        {
            elem_time_m10.pix = (uint8_t *)pixdash;
            elem_time_m0.pix  = (uint8_t *)pixdash;

            ui_draw_elem(&elem_time_m10);
            ui_draw_elem(&elem_time_m0);

            return;
        }

        m10 = (minute % 100) / 10;
        m0  = minute % 10;

        elem_time_m10.pix = (uint8_t *)pix_time_num[m10];
        elem_time_m0.pix  = (uint8_t *)pix_time_num[m0];

        ui_draw_elem(&elem_time_split);
        ui_draw_elem(&elem_time_m10);
        ui_draw_elem(&elem_time_m0);
    }
    else
    {
        ui_undraw_rect(&elem_time_m10.rect);
        ui_undraw_rect(&elem_time_m0.rect);
    }
}

/**
 * @brief 绘制电量
 *
 * @param present 电量百分比 0~100
 * @param pmode 电源模式 low | mid | hi
 * @param visible 是否显示
 */
void ui_draw_battery(uint8_t present, ui_bool_t visible)
{
    ui_rect_t bat_draw_rect;

    ui_draw_elem(&elem_battery);
    // 绘制电量
    memcpy(&bat_draw_rect, &rect_batter_levell, sizeof(rect_batter_levell));
    bat_draw_rect.w = rect_batter_levell.w * present / 100;
    ui_draw_fill_rect(&bat_draw_rect);
}

/**
 * @brief Draw the CO2 frequency display
 *
 * @param pmode Power mode
 * @param visible Whether to display or not
 */
void ui_draw_co2_frequency(uint8_t pmode, ui_bool_t visible)
{

    if (visible)
    {
        elem_power_mode.pix = (uint8_t *)pix_power_mode[pmode];
        ui_draw_elem(&elem_power_mode);
    }
    else
    {
        ui_undraw_rect(&elem_power_mode.rect);
    }
}

/**
 * @brief 绘制CO2水平
 * @param visible 是否显示
 * @param ppm co2的ppm值
 */
void ui_draw_co2_ppm(uint16_t ppm, ui_bool_t visible)
{
    uint8_t ppm_1000x, ppm_100x, ppm_10x, ppm_1x;
    if (visible)
    {
        ppm_1000x = (ppm % 10000) / 1000;
        ppm_100x  = (ppm % 1000) / 100;
        ppm_10x   = (ppm % 100) / 10;
        ppm_1x    = ppm % 10;

        elem_ppm_1000x.pix = (uint8_t *)pix_co2_ppm_num[ppm_1000x];
        elem_ppm_100x.pix  = (uint8_t *)pix_co2_ppm_num[ppm_100x];
        elem_ppm_10x.pix   = (uint8_t *)pix_co2_ppm_num[ppm_10x];
        elem_ppm_1x.pix    = (uint8_t *)pix_co2_ppm_num[ppm_1x];

        ui_draw_elem(&elem_ppm_1000x);
        ui_draw_elem(&elem_ppm_100x);
        ui_draw_elem(&elem_ppm_10x);
        ui_draw_elem(&elem_ppm_1x);
    }
    else
    {
        ui_undraw_rect(&elem_ppm_1000x.rect);
        ui_undraw_rect(&elem_ppm_100x.rect);
        ui_undraw_rect(&elem_ppm_10x.rect);
        ui_undraw_rect(&elem_ppm_1x.rect);
    }
}

/**
 * @brief 绘制co2 ppm图标
 * @param visible 是否显示
 */
void ui_draw_co2_ppm_label(ui_bool_t visible)
{
    if (visible)
    {
        ui_draw_elem(&elem_co2_ppm_label);
    }
    else
    {
        ui_undraw_rect(&elem_co2_ppm_label.rect);
    }
}

/**
 * @brief 绘制蓝牙图标
 * @param visible 是否显示
 */
void ui_draw_ble(ui_bool_t state, ui_bool_t visible)
{
    if (visible)
    {
        if (state)
        {
            elem_ble.pix = (uint8_t *)pix_ble;
        }
        else
        {
            elem_ble.pix = (uint8_t *)pix_ble_off;
        }
        ui_draw_elem(&elem_ble);
    }
    else
    {
        ui_undraw_rect(&elem_ble.rect);
    }
}

/**
 * @brief   Draw the indicator for the co2 range
 *
 * @param ppm CO2 ppm value
 */
void ui_draw_indicator(uint16_t ppm)
{
    static const uint8_t width        = 54;
    uint16_t             ppm_range    = 5000;
    uint16_t             yellow_start = cfg_fstorage_get_graph_yellow_start();
    uint16_t             red_start    = cfg_fstorage_get_graph_red_start();

    if (ppm < yellow_start)
    {
        elem_indicator.rect.x = width * ppm / yellow_start; // 0ppm~800ppm
    }
    else if (ppm < red_start)
    {
        elem_indicator.rect.x = width + (width * (ppm - yellow_start) / (red_start - yellow_start)); // 800ppm~1000ppm
    }
    else if (ppm < 5000)
    {
        elem_indicator.rect.x = width * 2 + (width * (ppm - red_start) / (ppm_range - red_start)); // 1000ppm~5000ppm
    }
    else
    {
        elem_indicator.rect.x = width * 2 + (width * (ppm_range - red_start) / (ppm_range - red_start)); // 1000ppm~5000ppm
    }

    ui_draw_elem(&elem_indicator);
}

/**
 * @brief 绘制3个色块
 *
 */
void ui_draw_indicator_rect(void)
{
    ui_draw_fill_rect(&elem_rect_g);
    ui_draw_fill_rect(&elem_rect_y);
    ui_draw_fill_rect(&elem_rect_r);
}

/**
 * @brief Draw the sensor error text
 */
void ui_draw_sensor_error(void)
{
    char    error_str[8]; // Buffer for "CODE: X" + null terminator
    uint8_t code = get_error_code();
    snprintf(error_str, sizeof(error_str), "CODE: %d", code);

    ui_draw_text(-1, -1, "SENSOR ERROR", true);
    ui_draw_text(-1, 116, error_str, true);
}

/**
 * @brief 绘制校准倒计时秒数
 *
 * @param seconds
 */
void ui_draw_recalibrating_countdown(uint16_t seconds)
{
    uint8_t sec_100x, sec_10x, sec_1x;
    sec_100x = seconds / 100;
    sec_10x  = (seconds % 100) / 10;
    sec_1x   = seconds % 10;

    elem_cali_time_100x.pix = (uint8_t *)pix_co2_ppm_num[sec_100x];
    elem_cali_time_10x.pix  = (uint8_t *)pix_co2_ppm_num[sec_10x];
    elem_cali_time_1x.pix   = (uint8_t *)pix_co2_ppm_num[sec_1x];

    ui_draw_elem(&elem_cali_time_100x);
    ui_draw_elem(&elem_cali_time_10x);
    ui_draw_elem(&elem_cali_time_1x);
}

/**
 * @brief Draw CC value
 */
void ui_draw_cc_value(int16_t cc)
{
    int16_t cc_abs = (cc > 0) ? cc : -cc;
    if (cc > 0)
    {
        elem_cali_sign_1000x.pix = (uint8_t *)pix_co2_ppm_num[11];
    }
    else
    {
        elem_cali_sign_1000x.pix = (uint8_t *)pix_co2_ppm_num[10];
    }

    uint8_t cc_100x, cc_10x, cc_1x;
    cc_100x = (cc_abs % 1000) / 100;
    cc_10x  = (cc_abs % 100) / 10;
    cc_1x   = cc_abs % 10;

    print("CC: 100: %d 10: %d 1:%d, isPositive: %d ", cc_100x, cc_10x, cc_1x, cc > 0);

    elem_cali_time_100x.pix = (uint8_t *)pix_co2_ppm_num[cc_100x];
    elem_cali_time_10x.pix  = (uint8_t *)pix_co2_ppm_num[cc_10x];
    elem_cali_time_1x.pix   = (uint8_t *)pix_co2_ppm_num[cc_1x];

    if (cc_abs != 0)
    {
        ui_draw_elem(&elem_cali_sign_1000x);
    }

    ui_draw_elem(&elem_cali_time_100x);
    ui_draw_elem(&elem_cali_time_10x);
    ui_draw_elem(&elem_cali_time_1x);
}

/**
 * @brief Draw DND
 * @param dnd_mode
 * @param visible
 */
void ui_draw_dnd(uint8_t dnd_time)
{
    if (dnd_time == 0)
    {
        ui_draw_elem(&elem_dnd_off);
    }
    else
    {
        ui_draw_elem(&elem_dnd);
    }
}

/**
 * @brief 绘制调色盘
 *
 */
void ui_draw_color_plate(void)
{
    memset(gram, 0xFF, LCD_WIDTH / 8 * LCD_HIGHT);
}

/**
 * @brief 绘制passkey
 *
 * @param key
 */
void ui_draw_passkey(uint8_t *key)
{
    passkey_100000x.pix = (uint8_t *)pix_passkey_num[key[0]];
    passkey_10000x.pix  = (uint8_t *)pix_passkey_num[key[1]];
    passkey_1000x.pix   = (uint8_t *)pix_passkey_num[key[2]];
    passkey_100x.pix    = (uint8_t *)pix_passkey_num[key[3]];
    passkey_10x.pix     = (uint8_t *)pix_passkey_num[key[4]];
    passkey_1x.pix      = (uint8_t *)pix_passkey_num[key[5]];

    ui_draw_elem(&passkey_100000x);
    ui_draw_elem(&passkey_10000x);
    ui_draw_elem(&passkey_1000x);
    ui_draw_elem(&passkey_100x);
    ui_draw_elem(&passkey_10x);
    ui_draw_elem(&passkey_1x);
}

/**
 * @brief 绘制电量低充电提示
 *
 */
void ui_draw_battery_low(void)
{
    ui_draw_elem(&elem_bat_low_frame);
    ui_draw_elem(&elem_bat_low_red);
    ui_draw_elem(&elem_bat_low_text);
}

/**
 * @brief Draw power on
 */
void ui_draw_power_on(void)
{
    ui_draw_elem(&elem_airspot_text);

    ui_draw_fill_rect(&logo_g);
    ui_draw_fill_rect(&logo_y);
    ui_draw_fill_rect(&logo_r);

    ui_draw_elem(&elem_power_on_text);

    ui_draw_elem(&elem_down_arrow);
}

/**
 * @brief 设置当前显示的页,以输出正确的颜色
 *
 * @param page 页面
 */
void ui_set_disp_page(ui_page_t page)
{
    disp_page = page;
}

/**
 * @brief 获取当前显示的页
 *
 * @return ui_page_t
 */
ui_page_t ui_get_disp_page(void)
{
    return disp_page;
}

uint8_t get_char_width(char c)
{
    if (c < 32 || c > 127) return 8; // Default width for unsupported chars
    return font_widths[c - 32];      // Precomputed width lookup
}

// Function to get the total width of a text string
uint8_t get_text_width(const char *text)
{
    uint8_t width = 0;
    while (*text)
    {
        width += get_char_width(*text) + 1; // Include spacing
        text++;
    }
    return (width > 0) ? (width - 1) : 0; // Remove extra space at the end
}

// Function to render or clear a text string at (x, y) with dynamic spacing
// If `draw` is true, the text is drawn, otherwise it is cleared
void ui_draw_text(int16_t x, int16_t y, const char *text, bool draw)
{
    if (!text) return;

    uint8_t text_width = get_text_width(text);

    // Center X if x == -1
    if (x == -1)
    {
        x = (LCD_WIDTH - text_width) / 2;
    }

    // Center Y if y == -1
    if (y == -1)
    {
        y = (LCD_HIGHT - FONT_HEIGHT) / 2;
    }

    if (!draw)
    {
        // Define the rectangle covering the text and clear it
        ui_rect_t rect = {
            .x = x,
            .y = y,
            .w = text_width,
            .h = FONT_HEIGHT};
        ui_undraw_rect(&rect);
        return; // Skip drawing if only clearing
    }

    // Draw each character
    while (*text)
    {
        if (*text >= 32 && *text <= 127)
        {
            ui_elem_t elem = {
                .rect = {
                    .x = x,
                    .y = y,
                    .w = FONT_WIDTH, // Use precomputed width
                    .h = FONT_HEIGHT,
                },
                .pix = (uint8_t *)font[*text - 32] // Get font bitmap
            };

            ui_draw_elem(&elem);            // Draw or clear based on `draw`
            x += get_char_width(*text) + 1; // Move to the next character
        }
        text++; // Move to the next character
    }
}

void ui_draw_bar_bg(void)
{
    ui_draw_fill_rect(&elem_bar_bg);
}

void ui_draw_bar_graph(void)
{
    extern uint16_t co2_history[BAR_COUNT];

    for (int i = 0; i < BAR_COUNT; i++)
    {
        // Calculate x position for this bar
        uint8_t bar_x = BAR_START_X + i * (BAR_WIDTH + BAR_SPACING);

        // Draw the full height bar (both active and background parts)
        ui_rect_t bar_rect = {
            .x = bar_x,
            .y = BAR_START_Y,
            .w = BAR_WIDTH,
            .h = BAR_HEIGHT};

        ui_draw_fill_rect(&bar_rect);
    }
}

uint8_t ui_get_bar_height_from_ppm(uint16_t ppm)
{
    uint16_t max_value = cfg_fstorage_get_graph_max_value();
    uint16_t min_value = cfg_fstorage_get_graph_min_value();
    uint8_t  height;

    // Ensure ppm is within the range
    if (ppm < min_value)
    {
        // If value is below minimum, don't show any bar
        return 0;
    }

    // Calculate height based on the range from min to max
    if (max_value > min_value)
    {
        // Calculate the percentage of the range and apply to bar height
        height = ((ppm - min_value) * BAR_HEIGHT) / (max_value - min_value);
    }
    else
    {
        // Prevent division by zero
        height = 0;
    }

    // Ensure height doesn't exceed maximum
    if (height > BAR_HEIGHT)
    {
        height = BAR_HEIGHT;
    }

    return height;
}

void ui_draw_airspot_text(void)
{
    ui_draw_elem(&elem_airspot_text);
}

void update_elements_position(uint8_t ui_mode)
{
    if (ui_mode == 0)
    {
        elem_power_mode.rect.y    = GRAPH_MODE_Y;
        elem_co2_ppm_label.rect.y = GRAPH_MODE_Y + 4;
        elem_ble.rect.y           = GRAPH_MODE_Y;
        elem_calib_off.rect.y     = GRAPH_MODE_Y + 2;
        elem_calib_on.rect.y      = GRAPH_MODE_Y + 2;
        elem_dnd_off.rect.y       = GRAPH_MODE_Y + 1;

        elem_ppm_1000x.rect.y = GRAPH_MODE_PPM_Y;
        elem_ppm_100x.rect.y  = GRAPH_MODE_PPM_Y;
        elem_ppm_10x.rect.y   = GRAPH_MODE_PPM_Y;
        elem_ppm_1x.rect.y    = GRAPH_MODE_PPM_Y;
    }
    else if (ui_mode == 1)
    {
        elem_power_mode.rect.y    = BAR_MODE_Y;
        elem_co2_ppm_label.rect.y = BAR_MODE_Y + 4;
        elem_ble.rect.y           = BAR_MODE_Y;
        elem_calib_off.rect.y     = BAR_MODE_Y + 2;
        elem_calib_on.rect.y      = BAR_MODE_Y + 2;
        elem_dnd_off.rect.y       = BAR_MODE_Y + 1;

        elem_ppm_1000x.rect.y = BAR_MODE_PPM_Y;
        elem_ppm_100x.rect.y  = BAR_MODE_PPM_Y;
        elem_ppm_10x.rect.y   = BAR_MODE_PPM_Y;
        elem_ppm_1x.rect.y    = BAR_MODE_PPM_Y;
    }
    else if (ui_mode == 2)
    {
        elem_power_mode.rect.y    = PLAIN_MODE_Y;
        elem_co2_ppm_label.rect.y = PLAIN_MODE_Y + 4;
        elem_ble.rect.y           = PLAIN_MODE_Y;
        elem_calib_off.rect.y     = PLAIN_MODE_Y + 2;
        elem_calib_on.rect.y      = PLAIN_MODE_Y + 2;
        elem_dnd_off.rect.y       = PLAIN_MODE_Y + 1;

        elem_ppm_1000x.rect.y = PLAIN_MODE_PPM_Y;
        elem_ppm_100x.rect.y  = PLAIN_MODE_PPM_Y;
        elem_ppm_10x.rect.y   = PLAIN_MODE_PPM_Y;
        elem_ppm_1x.rect.y    = PLAIN_MODE_PPM_Y;
    }
}