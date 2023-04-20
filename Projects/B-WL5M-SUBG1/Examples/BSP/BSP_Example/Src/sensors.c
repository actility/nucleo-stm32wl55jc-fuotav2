/**
  ******************************************************************************
  * @file    sensors.c
  * @author  MCD Application Team
  * @brief   This example code shows how to use the sensors supported by the
  *          B-WL5M-SUBG board
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/** @addtogroup STM32WLxx_HAL_Examples
  * @{
  */

/** @addtogroup BSP
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define CELSIUS_ASCII_CODE 248

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Demo of STTS22H temperature sensor.
  */
void Temperature_demo(void)
{
#if defined(USE_ENV_SENSOR_STTS22H_0)
  float temperature;
  temperature = 0;
  
  BSP_LED_Init(LED_GREEN);

  uint32_t ret = BSP_ENV_SENSOR_Init(ENV_SENSOR_STTS22H_0, ENV_TEMPERATURE);
  if (ret != BSP_ERROR_NONE)
  {
    Error_Handler();
  }
  
  ret = BSP_ENV_SENSOR_Enable(ENV_SENSOR_STTS22H_0, ENV_TEMPERATURE);
  if (ret != BSP_ERROR_NONE)
  {
    Error_Handler();
  }
  
  for(uint8_t i = 0; i < 10; i++)
  {
    ret = BSP_ENV_SENSOR_GetValue(ENV_SENSOR_STTS22H_0, ENV_TEMPERATURE, &temperature);
    if (ret != BSP_ERROR_NONE)
    {
      Error_Handler();
    }
    
    printf("\tTemperature = %f\n", temperature);
    HAL_Delay(1000);
    BSP_LED_Toggle(LED_GREEN);
  }
  
  ret = BSP_ENV_SENSOR_Disable(ENV_SENSOR_STTS22H_0, ENV_TEMPERATURE);
  if (ret != BSP_ERROR_NONE)
  {
    Error_Handler();
  }
  
  ret = BSP_ENV_SENSOR_DeInit(ENV_SENSOR_STTS22H_0);
  if (ret != BSP_ERROR_NONE)
  {
    Error_Handler();
  }
  
  BSP_LED_DeInit(LED_GREEN);
#endif /* defined(USE_ENV_SENSOR_STTS22H_0) */
}

/**
  * @brief  Demo of ILPS22QS pressure sensor.
  */
void Pressure_demo(void)
{
#if defined(USE_ENV_SENSOR_ILPS22QS_0)
  float pressure;
  pressure = 0;

  uint32_t ret = BSP_ENV_SENSOR_Init(ENV_SENSOR_ILPS22QS_0, ENV_PRESSURE);
  if (ret != BSP_ERROR_NONE)
  {
    Error_Handler();
  }
  
  ret = BSP_ENV_SENSOR_Enable(ENV_SENSOR_ILPS22QS_0, ENV_PRESSURE);
  if (ret != BSP_ERROR_NONE)
  {
    Error_Handler();
  }
  
  for(uint8_t i = 0; i < 10; i++)
  {
    ret = BSP_ENV_SENSOR_GetValue(ENV_SENSOR_ILPS22QS_0, ENV_PRESSURE, &pressure);
    if (ret != BSP_ERROR_NONE)
    {
      Error_Handler();
    }
    printf("\tPressure = %f\n", pressure);
    HAL_Delay(1000);
  }
  
  ret = BSP_ENV_SENSOR_Disable(ENV_SENSOR_ILPS22QS_0, ENV_PRESSURE);
  if (ret != BSP_ERROR_NONE)
  {
    Error_Handler();
  }
  
  ret = BSP_ENV_SENSOR_DeInit(ENV_SENSOR_ILPS22QS_0);
  if (ret != BSP_ERROR_NONE)
  {
    Error_Handler();
  }
#endif /* defined(USE_ENV_SENSOR_ILPS22QS_0) */
}

/**
  * @brief  Demo of ISM330DHCX accelerometer sensor.
  */
void Accelero_demo(void)
{
#if defined(USE_MOTION_SENSOR_ISM330DHCX_0)
  MOTION_SENSOR_Axes_t acceleration;

  uint32_t ret = BSP_MOTION_SENSOR_Init(MOTION_SENSOR_ISM330DHCX_0, MOTION_ACCELERO);
  if (ret != BSP_ERROR_NONE)
  {
    Error_Handler();
  }
  
  ret = BSP_MOTION_SENSOR_Enable(MOTION_SENSOR_ISM330DHCX_0, MOTION_ACCELERO);
  if (ret != BSP_ERROR_NONE)
  {
    Error_Handler();
  }

  for(uint8_t i = 0; i < 10; i++)
  {
    memset(&acceleration, 0, sizeof(MOTION_SENSOR_Axes_t));
    ret = BSP_MOTION_SENSOR_GetAxes(MOTION_SENSOR_ISM330DHCX_0, MOTION_ACCELERO, &acceleration);
    if (ret != BSP_ERROR_NONE)
    {
      Error_Handler();
    }
    printf( "\tAccelero (x = %ld, y = %ld, z = %ld)\n", acceleration.x, acceleration.y, acceleration.z);

    HAL_Delay(1000);
  }
  
  ret = BSP_MOTION_SENSOR_Disable(MOTION_SENSOR_ISM330DHCX_0, MOTION_ACCELERO);
  if (ret != BSP_ERROR_NONE)
  {
    Error_Handler();
  }
  
  ret = BSP_MOTION_SENSOR_DeInit(MOTION_SENSOR_ISM330DHCX_0);
  if (ret != BSP_ERROR_NONE)
  {
    Error_Handler();
  }
#endif /* USE_MOTION_SENSOR_ISM330DHCX_0 */
}

/**
  * @brief  Demo of ISM330DHCX gyroscope sensor.
  */
void Gyro_demo(void)
{
#if defined(USE_MOTION_SENSOR_ISM330DHCX_0)
  MOTION_SENSOR_Axes_t angular_velocity;

  uint32_t ret = BSP_MOTION_SENSOR_Init(MOTION_SENSOR_ISM330DHCX_0, MOTION_GYRO);
  if (ret != BSP_ERROR_NONE)
  {
    Error_Handler();
  }
  
  ret = BSP_MOTION_SENSOR_Enable(MOTION_SENSOR_ISM330DHCX_0, MOTION_GYRO);
  if (ret != BSP_ERROR_NONE)
  {
    Error_Handler();
  }

  for(uint8_t i = 0; i < 10; i++)
  {
    memset(&angular_velocity, 0, sizeof(MOTION_SENSOR_Axes_t));
    ret = BSP_MOTION_SENSOR_GetAxes(MOTION_SENSOR_ISM330DHCX_0, MOTION_GYRO, &angular_velocity);
    if (ret != BSP_ERROR_NONE)
    {
      Error_Handler();
    }
    printf("\tGyro (x = %ld, y = %ld, z = %ld)\n", angular_velocity.x, angular_velocity.y, angular_velocity.z);

    HAL_Delay(1000);
  }
  
  ret = BSP_MOTION_SENSOR_Disable(MOTION_SENSOR_ISM330DHCX_0, MOTION_GYRO);
  if (ret != BSP_ERROR_NONE)
  {
    Error_Handler();
  }
 
  ret = BSP_MOTION_SENSOR_DeInit(MOTION_SENSOR_ISM330DHCX_0);
  if (ret != BSP_ERROR_NONE)
  {
    Error_Handler();
  }
#endif /* USE_MOTION_SENSOR_ISM330DHCX_0 */
}

/**
  * @brief  Demo of ISM330DHCX gyroscope sensor.
  */
void Magneto_demo(void)
{
#if defined(USE_MOTION_SENSOR_IIS2MDC_0)
  MOTION_SENSOR_Axes_t angular_velocity;

  uint32_t ret = BSP_MOTION_SENSOR_Init(MOTION_SENSOR_IIS2MDC_0, MOTION_MAGNETO);
  if (ret != BSP_ERROR_NONE)
  {
    Error_Handler();
  }
  
  ret = BSP_MOTION_SENSOR_Enable(MOTION_SENSOR_IIS2MDC_0, MOTION_MAGNETO);
  if (ret != BSP_ERROR_NONE)
  {
    Error_Handler();
  }

  for(uint8_t i = 0; i < 10; i++)
  {
    memset(&angular_velocity, 0, sizeof(MOTION_SENSOR_Axes_t));
    ret = BSP_MOTION_SENSOR_GetAxes(MOTION_SENSOR_IIS2MDC_0, MOTION_MAGNETO, &angular_velocity);
    if (ret != BSP_ERROR_NONE)
    {
      Error_Handler();
    }
    printf("\tMAGNETO (x = %ld, y = %ld, z = %ld)\n", angular_velocity.x, angular_velocity.y, angular_velocity.z);

    HAL_Delay(1000);
  }
  
  ret = BSP_MOTION_SENSOR_Disable(MOTION_SENSOR_IIS2MDC_0, MOTION_MAGNETO);
  if (ret != BSP_ERROR_NONE)
  {
    Error_Handler();
  }
  
  ret = BSP_MOTION_SENSOR_DeInit(MOTION_SENSOR_IIS2MDC_0);
  if (ret != BSP_ERROR_NONE)
  {
    Error_Handler();
  }
#endif /* USE_MOTION_SENSOR_ISM330DHCX_0 */
}

/**
  * @}
  */

/**
  * @}
  */
