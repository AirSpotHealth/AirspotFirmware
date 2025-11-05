#ifndef __SCD4X_H__
#define __SCD4X_H__

#include "stdint.h"

#define SC4DX_S_START_PERIODIC_MEASUREMENT             0x21b1
#define SC4DX_R_READ_MEASUREMENT                       0xec05
#define SC4DX_S_STOP_PERIODIC_MEASUREMENT              0x3f86
#define SC4DX_W_SET_TEMPERATURE_OFFSET                 0x241d
#define SC4DX_R_GET_TEMPERATURE_OFFSET                 0x2318
#define SC4DX_W_SET_SENSOR_ALITUTUDE                   0x2427
#define SC4DX_R_GET_SENSOR_ALITUTUDE                   0x2322
#define SC4DX_S_SET_AMBIENT_PRESSURE                   0xe000
#define SC4DX_RW_PERFORM_FORCED_RECALIBRATION          0x362f
#define SC4DX_W_SET_AUTOMATIC_SELF_CALIBRATION_ENABLED 0x2416
#define SC4DX_R_GET_AUTOMATIC_SELF_CALIBRATION_ENABLED 0x2313
#define SC4DX_S_START_LOW_POWER_PERIODIC_MEASUREMENT   0x21ac
#define SC4DX_R_GET_DATA_READY_STATUS                  0xe4b8
#define SC4DX_S_PERSIST_SETTINGS                       0x3615
#define SC4DX_R_GET_SERIAL_NUMBER                      0x3682
#define SC4DX_R_PERFORM_SELF_TEST                      0x3639
#define SC4DX_S_PERFORM_FACTORY_RESET                  0x3632
#define SC4DX_S_REINIT                                 0x3646
#define SC4DX_S_MEASURE_SINGLE_SHOT                    0x219d
#define SC4DX_S_MEASURE_SINGLE_SHOT_RHT_ONLY           0x2196

#endif
