/**
 * Copyright (c) 2019 - 2021, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#ifndef CUSTOM_BOARD_H
#define CUSTOM_BOARD_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "nrf_gpio.h"

    /************************************************************************************************* */

#define PIN_VIBRATOR   NRF_GPIO_PIN_MAP(0, 14)
#define PIN_CO2_SDA    NRF_GPIO_PIN_MAP(0, 22)
#define PIN_CO2_SCL    NRF_GPIO_PIN_MAP(0, 23)
#define PIN_BUZZER     NRF_GPIO_PIN_MAP(0, 24)
#define PIN_CHRG_STA   NRF_GPIO_PIN_MAP(0, 25)
#define PIN_CO2_PWR    NRF_GPIO_PIN_MAP(0, 26)
#define PIN_BAT_ADC    NRF_GPIO_PIN_MAP(0, 28)
#define PIN_KEY        NRF_GPIO_PIN_MAP(0, 13)
#define PIN_FLASH_CS   NRF_GPIO_PIN_MAP(0, 31)
#define PIN_FLASH_MISO NRF_GPIO_PIN_MAP(0, 30)
#define PIN_FLASH_MOSI NRF_GPIO_PIN_MAP(0, 27)
#define PIN_FLASH_CLK  NRF_GPIO_PIN_MAP(0, 29)
#define PIN_USB_IN     NRF_GPIO_PIN_MAP(0, 12)
#define PIN_LCD_PWR    NRF_GPIO_PIN_MAP(0, 11) // lcd power enable
#define PIN_LCD_CS     NRF_GPIO_PIN_MAP(0, 2)  // chip select
#define PIN_LCD_CD     NRF_GPIO_PIN_MAP(0, 3)  // data or command
#define PIN_LCD_RST    NRF_GPIO_PIN_MAP(0, 4)  // hard reset
#define PIN_LCD_CLK    NRF_GPIO_PIN_MAP(0, 5)  // clock
#define PIN_LCD_SDA    NRF_GPIO_PIN_MAP(0, 7)  // data in/out
#define PIN_LCD_TE     NRF_GPIO_PIN_MAP(0, 8)  // tearing effect singal pin
#define PIN_LCD_BL     NRF_GPIO_PIN_MAP(0, 10) // backlight

    /************************************************************************************************* */

#define BUTTONS_NUMBER 1

#define BUTTON_0 PIN_KEY

#define BUTTON_PULL NRF_GPIO_PIN_PULLUP

#define BUTTONS_ACTIVE_STATE 0

#define BUTTONS_LIST \
    {                \
        BUTTON_0     \
    }

#define BSP_BUTTON_0 BUTTON_0

#define BSP_BUTTON_RIGHT BUTTON_0

    /************************************************************************************************* */

#ifdef __cplusplus
}
#endif

#endif // PCA10100_H
