#ifndef __UI_H__
#define __UI_H__

#include "log.h"
#include "spi.h"
#include "st7301.h"
#include "stdint.h"

/** UI Pages */
typedef enum
{
    UI_PAGE_OFF = 0,
    UI_PAGE_MAIN,
    UI_PAGE_RECALIBRATING,
    UI_PAGE_COLOR_PLATE,
    UI_PAGE_PASSKEY,
    UI_PAGE_BAT_LOW,
    UI_PAGE_SENSOR_ERROR,
    UI_PAGE_POWER_ON,
    UI_PAGE_FACTORY_TEST
} ui_page_t;

/** Drawing Area */
typedef struct
{
    uint8_t x; // x-coordinate
    uint8_t y; // y-coordinate
    uint8_t w; // Width
    uint8_t h; // Height
} ui_rect_t;

// Define the full screen area
static ui_rect_t rect_full_screen = {
    .y = 0,
    .h = LCD_HIGHT,
};

/** Boolean Values for UI */
typedef enum
{
    UI_BOOL_FALSE = 0,
    UI_BOOL_TRUE,
} ui_bool_t;

/** Blink Types */
typedef enum
{
    UI_BLINK_NONE = 0,
    UI_BLINK_CO2_FREQUENCY,
    UI_BLINK_ALARM,
    UI_BLINK_VIBRATOR,
    UI_BLINK_HOUR,
    UI_BLINK_MINUTE,
    UI_BLINK_BLE,
    UI_BLINK_CO2_PPM_LABEL,
    UI_BLINK_CO2_VALUE,
    UI_BLINK_FIRST = UI_BLINK_CO2_FREQUENCY,
    UI_BLINK_LAST  = UI_BLINK_CO2_PPM_LABEL,
} ui_blink_t;

/** Check if (_x, _y) is within _rect */
#define INRECT(_x, _y, _rect) ((_x) >= (_rect)->x && (_x) < (_rect)->x + (_rect)->w && (_y) >= (_rect)->y && (_y) < (_rect)->y + (_rect)->h)

/** UI Element */
typedef struct
{
    ui_rect_t rect;
    uint8_t  *pix; // Pixels (1 bit per pixel, row-major order)
} ui_elem_t;

/** Get pixel state at specified position */
#define PIX_IS_SET(_elem, _row, _col) ((*((_elem)->pix + ((_elem)->rect.w + 7) / 8 * (_row) + (_col) / 8)) & (1 << ((_col) % 8)))

/** Colors */
#define COLOR_WHITE      63
#define COLOR_BLACK      0
#define COLOR_GRAY       5
#define COLOR_GRAY_BG    3
#define COLOR_GREEN      12
#define COLOR_YELLOW     15
#define COLOR_RED        3
#define COLOR_BLUE       48
#define COLOR_BLE_CONN   36
#define COLOR_BLE_DISCON COLOR_GRAY
#define COLOR_BLE_ADV    COLOR_WHITE
#define COLOR_BKG        COLOR_BLACK
#define COLOR_BAR_BG     8 // Very subtle dark color for bar backgrounds

#define FONT_HEIGHT 20
#define FONT_WIDTH  12

#define GRAPH_MODE_Y 88
#define BAR_MODE_Y   100
#define PLAIN_MODE_Y 156

#define GRAPH_MODE_PPM_Y 32
#define BAR_MODE_PPM_Y   44
#define PLAIN_MODE_PPM_Y 66

/** Bluetooth connection state, determines the color of the Bluetooth icon */
extern uint8_t ble_state;

/** Time value array */
extern const uint8_t pix_time_num[10][36];

/** Time elements: hour tens, hour units, separator, minute tens, minute units */
extern ui_elem_t elem_time_h10, elem_time_h0, elem_time_split, elem_time_m10, elem_time_m0;

/** Battery elements */
extern ui_elem_t     elem_battery;
extern ui_rect_t     rect_batter_levell;
extern const uint8_t pixdash[36];
extern const uint8_t pix_error_code[42];

/** CO2 PPM elements */
extern const uint8_t pix_co2_ppm_num[12][220];
extern ui_elem_t     elem_ppm_1000x, elem_ppm_100x, elem_ppm_10x, elem_ppm_1x;

/** Passkey elements */
extern const uint8_t pix_passkey_num[10][132];
extern ui_elem_t     passkey_1x, passkey_10x, passkey_100x, passkey_1000x, passkey_10000x, passkey_100000x;

/** Bluetooth icon area */
extern ui_elem_t     elem_ble;
extern const uint8_t pix_ble[60];
extern const uint8_t pix_ble_off[60];

/** Alarm icon */
extern ui_elem_t     elem_alarm;
extern const uint8_t pix_alarm[40];
extern const uint8_t pix_alarm_off[40];

/** Vibrator icon */
extern ui_elem_t     elem_vibrator;
extern const uint8_t pix_vibrator[40];
extern const uint8_t pix_vibrator_off[40];

/** Flight mode icon */
extern ui_elem_t     elem_flight_mode;
extern const uint8_t pix_flight_mode[36];

/** DND icon */
extern ui_elem_t     elem_dnd, elem_dnd_off;
extern const uint8_t pix_dnd[54];

/** Power on */
extern ui_elem_t     elem_power_on_text, elem_down_arrow, elem_airspot_text;
extern const uint8_t pix_power_on_text[1060], pix_down_arrow[136], pix_airspot_text[500];
extern ui_rect_t     logo_g, logo_y, logo_r;
/** POWER MODE */
extern ui_elem_t     elem_power_mode;
extern const uint8_t pix_power_mode[4][90];

/** CO2 PPM label */
extern ui_elem_t     elem_co2_ppm_label;
extern const uint8_t pix_co2_ppm_label[72];

/** Indicator triangles */
extern ui_elem_t elem_indicator;

/** CALIB */
extern ui_elem_t     elem_calib_off;
extern ui_elem_t     elem_calib_on;
extern const uint8_t pixel_calib_off[32];
extern const uint8_t pixel_calib_on[32];

/** Alarm areas: green, yellow, red */
extern ui_rect_t elem_rect_g, elem_rect_y, elem_rect_r;

/** Calibration page elements */
extern ui_elem_t elem_sensor_error, elem_sensor_error_code, elem_cali_sign_1000x, elem_cali_time_100x, elem_cali_time_10x, elem_cali_time_1x;

/** Low battery warning elements */
extern ui_elem_t elem_bat_low_frame, elem_bat_low_red, elem_bat_low_text;

/** VGA FONT 8 x 16 */
extern const uint8_t font[96][40];
extern const uint8_t font_widths[96];

/** Get error code */
extern uint8_t get_error_code(void);

/** Get sensor recovery attempt count */
extern uint8_t get_sensor_recovery_attempt(void);

/** UI mode */
extern uint8_t cfg_fstorage_get_ui_mode(void);

/** Set screen always on */
void set_screen_const_on(ui_bool_t state);

/** Get screen always on state */
ui_bool_t get_screen_const_on_state(void);

/** Get UI idle state, only draw when idle */
uint8_t ui_get_idle_state(void);

/** Refresh specified area of the screen, actually refreshes the entire row each time */
uint8_t ui_refresh(ui_rect_t *rect);

/** Check if refresh is done */
uint8_t ui_refresh_done(void);

/** Clear memory canvas to background color */
void ui_draw_bkg(void);

/** Draw alarm icon */
void ui_draw_buzzer(ui_bool_t state, ui_bool_t visible);

/** Draw vibrator icon */
void ui_draw_vibrator(ui_bool_t state, ui_bool_t visible);

/** Draw time */
void ui_draw_time(uint8_t hour, uint8_t minute, ui_bool_t visible);

/** Draw hour */
void ui_draw_hour(uint8_t hour, ui_bool_t visible);

/** Draw minute */
void ui_draw_minute(uint8_t minute, ui_bool_t visible);

/** Draw battery level */
void ui_draw_battery(uint8_t present, ui_bool_t visible);

/** Draw CO2 level */
void ui_draw_co2_ppm(uint16_t ppm, ui_bool_t visible);

/** Draw CO2 frequency */
void ui_draw_co2_frequency(uint8_t power_mode, ui_bool_t visible);

/** Draw CO2 PPM label */
void ui_draw_co2_ppm_label(ui_bool_t visible);

/** Draw Bluetooth icon */
void ui_draw_ble(ui_bool_t state, ui_bool_t visible);

/** Draw indicator triangle */
void ui_draw_indicator(uint16_t ppm);

/** Draw three color blocks */
void ui_draw_indicator_rect(void);

/** Draw recalibration countdown */
void ui_draw_recalibrating_countdown(uint16_t seconds);

/** Draw CC value */
void ui_draw_cc_value(int16_t cc);

/** Draw color plate */
void ui_draw_color_plate(void);

/** Draw DND */
void ui_draw_dnd(uint8_t dnd_time);

/** Draw passkey */
void ui_draw_passkey(uint8_t *key);

/** Draw low battery warning */
void ui_draw_battery_low(void);

/** Draw sensor error */
void ui_draw_sensor_error(void);

/** Set the current display page to output the correct color */
void ui_set_disp_page(ui_page_t page);

/** Get the current display page */
ui_page_t ui_get_disp_page(void);

/** Set passkey */
void ui_set_passkey(const char *key);

/** get co2 value */
extern uint16_t get_co2_value(void);

/** Draw Power on message */
void ui_draw_power_on(void);

/** Draw factory test */
void ui_draw_factory_test(void);

/** Draw text */
// Function to render a text string at (x, y) with dynamic spacing
void ui_draw_text(int16_t x, int16_t y, const char *text, bool draw);

/** Draw airspot text */
void ui_draw_airspot_text(void);

#define LCD_REFRESH(rect)                                   \
    do                                                      \
    {                                                       \
        TaskWait(spi_get_usage() == SPI_NOT_USE, TICK_MAX); \
        spi_config(SPI_LCD);                                \
        ui_refresh(rect);                                   \
    } while (0);

#define LCD_RELEASE()                          \
    do                                         \
    {                                          \
        TaskWait(ui_refresh_done(), TICK_MAX); \
        spi_config(SPI_NOT_USE);               \
    } while (0);

static char *pmode_str[] = {"Now", "3 Min", "1 Min", "5 sec"};

// Bar Graph Constants
#define BAR_HEIGHT  60
#define BAR_WIDTH   5
#define BAR_COUNT   29
#define BAR_SPACING 1
#define BAR_START_X 2
#define BAR_START_Y 115

extern ui_rect_t elem_bar_bg;

// View Mode
typedef enum
{
    VIEW_MODE_NORMAL = 0,
    VIEW_MODE_GRAPH
} view_mode_t;

/** Draw bar graph */
void ui_draw_bar_graph(void);

/**
 * @brief Updates the y-position of all UI elements that depend on power_mode_y
 * @param ui_mode The current UI mode
 */
void update_elements_position(uint8_t ui_mode);

/**
 * @brief Get the height of the bar from the ppm
 * @param ppm The ppm value
 * @return The height of the bar
 */
uint8_t ui_get_bar_height_from_ppm(uint16_t ppm);

/**
 * @brief Draw the background of the bar graph
 */
void ui_draw_bar_bg(void);

#endif // __UI_H__
