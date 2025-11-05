#include "st7301.h"

#define delay_ms nrf_delay_ms

void lcd_gpio_init(void)
{
    nrf_gpio_cfg_output(PIN_LCD_PWR);
    nrf_gpio_cfg_output(PIN_LCD_CD);
    nrf_gpio_cfg_output(PIN_LCD_RST);
    nrf_gpio_cfg_output(PIN_LCD_TE);
    nrf_gpio_cfg_output(PIN_LCD_BL);
    nrf_delay_ms(1);
}

void lcd_gpio_uninit(void)
{
    nrf_gpio_cfg_default(PIN_LCD_PWR);
    nrf_gpio_cfg_default(PIN_LCD_CD);
    nrf_gpio_cfg_default(PIN_LCD_RST);
    nrf_gpio_cfg_default(PIN_LCD_TE);
    nrf_gpio_cfg_default(PIN_LCD_BL);
}

/**
 * @brief lcd写入一字节的命令或者数据
 *
 * @param cd 命令/数据选择 COMMAND | PARAMETER
 * @param value 写入的值
 * @return uint8_t 成功返回0，失败返回1
 */
static void write_cd(uint8_t cd, uint8_t value)
{
    static uint8_t val;
    val = value;

    if (cd == COMMAND)
    {
        LCD_SEL_CMD();
    }
    else
    {
        LCD_SEL_PARM();
    }

    if (0 != spi_write(&val, 1, NULL, 10))
    {
        if (cd == COMMAND)
        {
            print("st7301 write command %02x failed\n", value);
        }
        else
        {
            print("st7301 write data %02x failed\n", value);
        }
    }
}

void st7301_config(void)
{
    write_cd(COMMAND, 0xEB);   // Enable OTP
    write_cd(PARAMETER, 0x02); // SCYS[7:6], WRCYC[5:3], DSCYC[2:0]

    write_cd(COMMAND, 0xD7);   // OTP Load Control
    write_cd(PARAMETER, 0x68); //

    write_cd(COMMAND, 0xD1);   // Auto Power Control
    write_cd(PARAMETER, 0x01); //

    write_cd(COMMAND, 0xC0);   // Gate Voltage Setting
    write_cd(PARAMETER, 0xE4); //

    write_cd(COMMAND, 0xC1);
    write_cd(PARAMETER, 0x28);
    write_cd(PARAMETER, 0x28);
    write_cd(PARAMETER, 0x28);
    write_cd(PARAMETER, 0x28);
    write_cd(PARAMETER, 0x14);
    write_cd(PARAMETER, 0x00);

    write_cd(COMMAND, 0xC2);
    write_cd(PARAMETER, 0x0D);
    write_cd(PARAMETER, 0x0D);
    write_cd(PARAMETER, 0x0D);
    write_cd(PARAMETER, 0x0D);

    write_cd(COMMAND, 0xCB);   // VCOMH Setting
    write_cd(PARAMETER, 0x14); // VCOMH

    write_cd(COMMAND, 0xB4);   // Gate EQ
    write_cd(PARAMETER, 0xE5); //
    write_cd(PARAMETER, 0x66); //
    write_cd(PARAMETER, 0xFD); // HPM EQ
    write_cd(PARAMETER, 0xFF); //
    write_cd(PARAMETER, 0xFF); //
    write_cd(PARAMETER, 0x7F); //
    write_cd(PARAMETER, 0xFD); //
    write_cd(PARAMETER, 0xFF); // LPM EQ
    write_cd(PARAMETER, 0xFF); //
    write_cd(PARAMETER, 0x7F); //

    write_cd(COMMAND, 0xB7);   // Disable Source EQ
    write_cd(PARAMETER, 0x04); //

    write_cd(COMMAND, 0x11); // Sleep out

    delay_ms(120);

    write_cd(COMMAND, 0xD6); // Load 51Hz code
    write_cd(PARAMETER, 0x01);

    write_cd(COMMAND, 0xC7); // 32Hz=A6,51Hz=80；
    write_cd(PARAMETER, 0x80);
    write_cd(PARAMETER, 0xE9);

    write_cd(COMMAND, 0xCA); // 68Hz；
    write_cd(PARAMETER, 0xF7);

    write_cd(COMMAND, 0xB2); // Frame rate setting
    write_cd(PARAMETER, 0X01);
    write_cd(PARAMETER, 0X02);

    write_cd(COMMAND, 0xB0);   // Duty Setting
    write_cd(PARAMETER, 0x58); // 212dutyx2/4=106

    write_cd(COMMAND, 0x36); // MY=0, MX=0, MV=0, ML=0
    // write_cd(PARAMETER, 0x48); // MX=1 ; DO=1
    write_cd(PARAMETER, (1 << 7)); // MX=1 ; DO=1

    write_cd(COMMAND, 0x3A);   // 64Color Mode
    write_cd(PARAMETER, 0x10); // 10:4write for 24bit ; 11: 3write for 24bit

    write_cd(COMMAND, 0xB9);   // Source Setting
    write_cd(PARAMETER, 0x23); // Pixel Cutting

    write_cd(COMMAND, 0xB8);   // Source Setting
    write_cd(PARAMETER, 0x00); // 08=Frame inversion

    // write_cd(COMMAND, 0x2A); // Column Address Setting
    // write_cd(PARAMETER, 0X08);
    // write_cd(PARAMETER, 0X33);

    // write_cd(COMMAND, 0x2B); // Row Address Setting
    // write_cd(PARAMETER, 0X00);
    // write_cd(PARAMETER, 0XAF);

    write_cd(COMMAND, 0x35); // Auto power down
    write_cd(PARAMETER, 0x00);

    write_cd(COMMAND, 0xD0);   // Auto power down
    write_cd(PARAMETER, 0x1F); //

    write_cd(COMMAND, 0x29); // DISPLAY ON

    write_cd(COMMAND, 0xC4); //
    write_cd(PARAMETER, 0xB1);

    write_cd(COMMAND, 0xD8); // D8=02 1*VGH，D8=03 1.1*VGH
    write_cd(PARAMETER, 0x02);

    write_cd(COMMAND, 0xE3); //
    write_cd(PARAMETER, 0x02);

    write_cd(COMMAND, 0x38); // HPM
}

/**
 * @brief Set the window addr
 *
 * @param x 起始x,每1个单位代表4个像素列
 * @param y 起始y
 * @param w 宽度,每1个单位代表4个像素列
 * @param h 高度
 * @return uint8_t 成功返回0，否则返回1
 */
uint8_t set_window_addr(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    // if (w == 0 || h == 0) return 1;
    // if (x + w >= 60 || y + h >= 240) return 1;

    write_cd(COMMAND, 0x2A);                       // 列地址
    write_cd(PARAMETER, (x + COL_OFFSET));         // 首列
    write_cd(PARAMETER, (x + COL_OFFSET + w - 1)); // 末列
    write_cd(COMMAND, 0x2B);                       // 行地址
    write_cd(PARAMETER, (y + ROW_OFFSET));         // 首行
    write_cd(PARAMETER, (y + ROW_OFFSET + h - 1)); // 末行
    // print("xs %d xe %d, ys %d ye %d\n",
    //       x + COL_OFFSET,
    //       x + COL_OFFSET + w - 1,
    //       y + ROW_OFFSET,
    //       y + ROW_OFFSET + h - 1);

    write_cd(COMMAND, 0x2C); // 准备写显存
    LCD_SEL_PARM();

    return 0;
}

void lcd_low_power(void)
{
    // write_cd(COMMAND, 0x10); // Sleep in
    write_cd(COMMAND, 0xb2); // frame rate
    write_cd(PARAMETER, 0); // 0.25Hz
    write_cd(COMMAND, 0x39); // low power mode

}
