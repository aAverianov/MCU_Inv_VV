/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
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
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define RESET_IP_Pin GPIO_PIN_10
#define RESET_IP_GPIO_Port GPIOB
#define LED_Pin GPIO_PIN_12
#define LED_GPIO_Port GPIOB
#define SCS_Pin GPIO_PIN_6
#define SCS_GPIO_Port GPIOC
#define RST_Pin GPIO_PIN_7
#define RST_GPIO_Port GPIOC
	
#define SIG_RUN_PIN	GPIO_PIN_2
#define SIG_RUN_PORT GPIOC
#define IND_STOP_PIN GPIO_PIN_8
#define IND_STOP_PORT GPIOB
#define IND_FLT_PIN GPIO_PIN_9
#define IND_FLT_PORT GPIOB
#define IND_PSFLT_PIN GPIO_PIN_7
#define IND_PSFLT_PORT GPIOB
#define IND_LCON_PIN GPIO_PIN_6
#define IND_LCON_PORT GPIOB
#define IND_ARC_PIN GPIO_PIN_1
#define IND_ARC_PORT GPIOC
#define IND_ARC_FLT_PIN GPIO_PIN_0
#define IND_ARC_FLT_PORT GPIOC
	
#define CMD_CONTACTOR_PIN GPIO_PIN_15
#define CMD_CONTACTOR_PORT GPIOC
	
#define CNTRL_CONTACTOR_PIN GPIO_PIN_4
#define CNTRL_CONTACTOR_PORT GPIOC

#define CNTRL_PS_PIN GPIO_PIN_3
#define CNTRL_PS_PORT GPIOC

/* USER CODE BEGIN Private defines */
//Битовые операции
#define BIT_IN_FALSE(reg, bit)      reg &= ~(1UL << bit)
#define BIT_IN_TRUE(reg, bit)       reg |= 1UL << bit
#define BIT_TEST(reg, bit)			reg & 1UL << bit
#define BIT_INVERT(reg, bit)		reg ^= 1UL << bit
// для сброса адреса
#define RESET_IP_READ			HAL_GPIO_ReadPin(RESET_IP_GPIO_Port, RESET_IP_Pin)
	
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
