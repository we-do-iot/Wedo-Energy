/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    adc_if.h
  * @author  MCD Application Team
  * @brief   Header for ADC interface configuration
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __ADC_IF_H__
#define __ADC_IF_H__

#ifdef __cplusplus
extern "C" {
#endif
/* Includes ------------------------------------------------------------------*/
#include "adc.h"
#include "platform.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/**
  * @brief Battery level in mV
  */
#define BAT_CR2032                  ((uint32_t) 3000)
/**
  * @brief Maximum battery level in mV
  */
#define VDD_BAT                     BAT_CR2032
/**
  * @brief Minimum battery level in mV
  */
#define VDD_MIN                     1800

/* USER CODE BEGIN EC */

/**
  * IMPORTANTE: CubeMX genera automáticamente defines para batería CR2032 arriba.
  * Los redefinimos aquí para usar valores de batería Li-ion.
  * Usamos #undef para evitar warnings de redefinición.
  */

/* Primero eliminamos las definiciones generadas por CubeMX */
#undef VDD_BAT
#undef VDD_MIN

/**
  * @brief Typical Li-ion battery nominal values (mV)
  * We measure the battery through a resistor divider and then reconstruct the
  * real battery voltage in software. The battery maximum is ~4.2V for Li-ion.
  */
#define BAT_LI_ION                  ((uint32_t) 4200)

/**
  * @brief Maximum battery level in mV (Li-ion)
  * REDEFINIDO: CubeMX genera VDD_BAT = BAT_CR2032, nosotros usamos BAT_LI_ION
  */
#define VDD_BAT                     BAT_LI_ION

/**
  * @brief Minimum battery level in mV for reporting (below this is considered empty)
  * REDEFINIDO: CubeMX genera VDD_MIN = 1800, nosotros usamos 3000 para Li-ion
  */
#define VDD_MIN                     3000

/* USER CODE END EC */

/* External variables --------------------------------------------------------*/
/* USER CODE BEGIN EV */

/* USER CODE END EV */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/

/**
  * @brief  Initializes the ADC input
  */
void SYS_InitMeasurement(void);

/**
  * @brief DeInitializes the ADC
  */
void SYS_DeInitMeasurement(void);

/**
  * @brief  Get the current temperature
  * @return value temperature in degree Celsius( q7.8 )
  */
int16_t SYS_GetTemperatureLevel(void);

/**
  * @brief Get the current battery level
  * @return value battery level in linear scale
  */
uint16_t SYS_GetBatteryLevel(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

#ifdef __cplusplus
}
#endif

#endif /* __ADC_IF_H__ */
