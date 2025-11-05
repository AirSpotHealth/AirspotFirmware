/*** 
 * @Author: tai && tai0105@163.com
 * @Date: 2022-12-08 13:28:33
 * @LastEditors: tai && tai0105@163.com
 * @LastEditTime: 2023-02-17 10:44:44
 * @FilePath: \nRF5_SDK_17.1.0_ddde560\examples\ble_peripheral\ble_app_uart_eeg_v2\pin_def.h
 * @Description: 
 */
#ifndef __PIN_DEF_H__
#define __PIN_DEF_H__

#define PIN_BAT_ADC 2
#define PIN_PGOOD 3
#define PIN_CHG 4

#define PIN_PWR_CTRL 5
#define PIN_PWR_KEY 6

#define PIN_LED3_G 7
#define PIN_LED3_B 9
#define PIN_LED3_R 8

// pin_eeg_en
#define PIN_ADS_PWR 10

#define PIN_LED2_G 11
#define PIN_LED2_R 13
#define PIN_LED2_B 12

#define PIN_LED1_R 16
#define PIN_LED1_G 15
#define PIN_LED1_B 14

#define PIN_ADS_MOSI 17
#define PIN_ADS_RST 18
#define PIN_ADS_START 19
#define PIN_ADS_CS 20

#define PIN_AB_STATUS 22
#define PIN_1531_LED1 0xff
#define PIN_1531_LED0 23

#define PIN_1531_PWR 24

//tx 与 PIN_PGOOD 复用
#define PIN_UART_TX PIN_PGOOD
#define PIN_UART_RX 25

#define PIN_ADS_SCLK 27
#define PIN_ADS_MISO 28
#define PIN_ADS_DRDY 29

#define PIN_LED4_B 26
#define PIN_LED4_G 31
#define PIN_LED4_R 30


#endif
