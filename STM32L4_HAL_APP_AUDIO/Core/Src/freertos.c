/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * File Name          : freertos.c
 * Description        : Code for freertos applications
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "usbd_core.h"
#include "usb_device.h"
#include "fatfs.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
FIL file;
FATFS fatFS;
/* USER CODE END Variables */
osThreadId appTaskHandle;
osThreadId usbTaskHandle;
osMutexId qspiMutexHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void appTaskBody(void const * argument);
void usbTaskBody(void const * argument);

extern void MX_USB_DEVICE_Init(void);
void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize)
{
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
  /* place for user code */
}
/* USER CODE END GET_IDLE_TASK_MEMORY */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */
  /* Create the mutex(es) */
  /* definition and creation of qspiMutex */
  osMutexDef(qspiMutex);
  qspiMutexHandle = osMutexCreate(osMutex(qspiMutex));

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of appTask */
  osThreadDef(appTask, appTaskBody, osPriorityNormal, 0, 4096);
  appTaskHandle = osThreadCreate(osThread(appTask), NULL);

  /* definition and creation of usbTask */
  osThreadDef(usbTask, usbTaskBody, osPriorityHigh, 0, 256);
  usbTaskHandle = osThreadCreate(osThread(usbTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_appTaskBody */
/**
 * @brief  Function implementing the appTask thread.
 * @param  argument: Not used
 * @retval None
 */
/* USER CODE END Header_appTaskBody */
void appTaskBody(void const * argument)
{
  /* init code for USB_DEVICE */
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN appTaskBody */

  // get mutex first before access flash memory
  osMutexWait(qspiMutexHandle, osWaitForever);

  // mount file system
  if(f_mount(&fatFS, (TCHAR const*) USERPath, 1) != FR_OK)
  {
    uint8_t work[_MIN_SS];
    if(f_mkfs((TCHAR const*)USERPath, FM_ANY, 0, work, sizeof(work)) != FR_OK)
    {
      Error_Handler();
    }
    else
    {
      if(f_mount(&fatFS, (TCHAR const*) USERPath, 1) != FR_OK)
      {
        Error_Handler();
      }
    }
  }

  // file open test
  if(f_open(&file, "FATFSOK", FA_CREATE_NEW | FA_WRITE) == FR_OK)
  {
    f_printf(&file, "FatFS is working properly.\n");
    f_close(&file);
  }

  f_mount(NULL, "", 0);

  // release mutex
  osMutexRelease(qspiMutexHandle);

  /* Infinite loop */
  for(;;)
  {
    osDelay(100);

    // green led on
    HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_SET);

    // get mutex before access flash memory
    osMutexWait(qspiMutexHandle, osWaitForever);

    // add your code here
    osDelay(100);

    // release mutex
    osMutexRelease(qspiMutexHandle);

    // green led off
    HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_RESET);
  }
  /* USER CODE END appTaskBody */
}

/* USER CODE BEGIN Header_usbTaskBody */
/**
 * @brief Function implementing the usbTask thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_usbTaskBody */
void usbTaskBody(void const * argument)
{
  /* USER CODE BEGIN usbTaskBody */
  uint32_t USB_VBUS_counter = 0;
  /* Infinite loop */
  for(;;)
  {
    USB_VBUS_counter = 0;

    // as long as VBUS is low, it never exit the loop
    while(USB_VBUS_counter < 5)
    {
      osDelay(10);
      //check VBUS HIGH 5 times
      if(HAL_GPIO_ReadPin(USB_VBUS_GPIO_Port, USB_VBUS_Pin) != GPIO_PIN_RESET)
      {
        USB_VBUS_counter++;
      }
      else
      {
        break;
      }
    }

    // if VBUS is HIGH, initialize USB Device
    if(USB_VBUS_counter >= 5)
    {
      MX_USB_DEVICE_Init();

      // Red led on
      HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_SET);

      // if VBUS is LOW, then deinitialize USB device
      // as long as VBUS is HIGH, it never exit the loop
      while(USB_VBUS_counter)
      {
        osDelay(100);

        if(HAL_GPIO_ReadPin(USB_VBUS_GPIO_Port, USB_VBUS_Pin) == GPIO_PIN_RESET)
        {
          USB_VBUS_counter--;
        }
        else
        {
          USB_VBUS_counter = 5;
        }
      }

      USBD_DeInit(&hUsbDeviceFS);

      // Red led off
      HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_RESET);
    }

    osDelay(1000);
  }
  /* USER CODE END usbTaskBody */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */
