/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    lptim.c
  * @brief   This file provides code for the configuration
  *          of the LPTIM instances.
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
/* Includes ------------------------------------------------------------------*/
#include "lptim.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

LPTIM_HandleTypeDef hlptim2;

/* LPTIM2 init function */
void MX_LPTIM2_Init(void)
{

  /* USER CODE BEGIN LPTIM2_Init 0 */

  /* USER CODE END LPTIM2_Init 0 */

  /* USER CODE BEGIN LPTIM2_Init 1 */

  /* USER CODE END LPTIM2_Init 1 */
  hlptim2.Instance = LPTIM2;
  hlptim2.Init.Clock.Source = LPTIM_CLOCKSOURCE_APBCLOCK_LPOSC;
  hlptim2.Init.Clock.Prescaler = LPTIM_PRESCALER_DIV1;
  hlptim2.Init.Trigger.Source = LPTIM_TRIGSOURCE_0;
  hlptim2.Init.Trigger.ActiveEdge = LPTIM_ACTIVEEDGE_FALLING;
  hlptim2.Init.Trigger.SampleTime = LPTIM_TRIGSAMPLETIME_DIRECTTRANSITION;
  hlptim2.Init.OutputPolarity = LPTIM_OUTPUTPOLARITY_HIGH;
  hlptim2.Init.UpdateMode = LPTIM_UPDATE_IMMEDIATE;
  hlptim2.Init.CounterSource = LPTIM_COUNTERSOURCE_INTERNAL;
  hlptim2.Init.Input1Source = LPTIM_INPUT1SOURCE_GPIO;
  hlptim2.Init.Input2Source = LPTIM_INPUT2SOURCE_GPIO;
  if (HAL_LPTIM_Init(&hlptim2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN LPTIM2_Init 2 */

  /* USER CODE END LPTIM2_Init 2 */

}

void HAL_LPTIM_MspInit(LPTIM_HandleTypeDef* lptimHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
  if(lptimHandle->Instance==LPTIM2)
  {
  /* USER CODE BEGIN LPTIM2_MspInit 0 */

  /* USER CODE END LPTIM2_MspInit 0 */

  /** Initializes the peripherals clock
  */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_LPTIM2;
    PeriphClkInit.Lptim2ClockSelection = RCC_LPTIM2CLKSOURCE_LSE;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
      Error_Handler();
    }

    /* LPTIM2 clock enable */
    __HAL_RCC_LPTIM2_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**LPTIM2 GPIO Configuration
    PA5     ------> LPTIM2_ETR
    */
    GPIO_InitStruct.Pin = BTN_DOWN_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF14_LPTIM2;
    HAL_GPIO_Init(BTN_DOWN_GPIO_Port, &GPIO_InitStruct);

    /* LPTIM2 interrupt Init */
    HAL_NVIC_SetPriority(LPTIM2_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(LPTIM2_IRQn);
  /* USER CODE BEGIN LPTIM2_MspInit 1 */

  /* USER CODE END LPTIM2_MspInit 1 */
  }
}

void HAL_LPTIM_MspDeInit(LPTIM_HandleTypeDef* lptimHandle)
{

  if(lptimHandle->Instance==LPTIM2)
  {
  /* USER CODE BEGIN LPTIM2_MspDeInit 0 */

  /* USER CODE END LPTIM2_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_LPTIM2_CLK_DISABLE();

    /**LPTIM2 GPIO Configuration
    PA5     ------> LPTIM2_ETR
    */
    HAL_GPIO_DeInit(BTN_DOWN_GPIO_Port, BTN_DOWN_Pin);

    /* LPTIM2 interrupt Deinit */
    HAL_NVIC_DisableIRQ(LPTIM2_IRQn);
  /* USER CODE BEGIN LPTIM2_MspDeInit 1 */

  /* USER CODE END LPTIM2_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
