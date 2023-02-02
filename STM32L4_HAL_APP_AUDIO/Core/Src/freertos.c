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
#include "sai.h"
#include "cs43l22.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
#define AUDIO_BUFFER_SIZE     (1024 * 8)    //8K
#define WAV_FILE_HEADER_SIZE  44
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
typedef enum {
  AUDIOPLAYER_STATE_STOPPED = 0,
  AUDIOPLAYER_STATE_RUNNING,
  AUDIOPLAYER_STATE_PAUSED
} AudioPlayer_StateTypeDef;
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
FIL file;
FATFS fatFS;
uint8_t audioBuffer[AUDIO_BUFFER_SIZE];
uint32_t actualPosition;
AudioPlayer_StateTypeDef  AppState = AUDIOPLAYER_STATE_STOPPED;

/* USER CODE END Variables */
osThreadId appTaskHandle;
osThreadId usbTaskHandle;
osThreadId audioTaskHandle;
osMessageQId joystickInputQueueHandle;
osMutexId qspiMutexHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
static void audioPlayerStart(void);
static void audioPlayerStop(void);
static void audioPlayerPause(void);
static void audioPlayerResume(void);
static void handleJoystickEvent(osEvent event);
/* USER CODE END FunctionPrototypes */

void appTaskBody(void const * argument);
void usbTaskBody(void const * argument);
void audioTaskBody(void const * argument);

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

  /* Create the queue(s) */
  /* definition and creation of joystickInputQueue */
  osMessageQDef(joystickInputQueue, 1, uint16_t);
  joystickInputQueueHandle = osMessageCreate(osMessageQ(joystickInputQueue), NULL);

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

  /* definition and creation of audioTask */
  osThreadDef(audioTask, audioTaskBody, osPriorityNormal, 0, 1024);
  audioTaskHandle = osThreadCreate(osThread(audioTask), NULL);

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
  //MX_USB_DEVICE_Init();
  /* USER CODE BEGIN appTaskBody */

  // get mutex first before access flash memory
  osMutexWait(qspiMutexHandle, osWaitForever);

  // mount file system
  if(f_mount(&fatFS, (TCHAR const*) USERPath, 1) != FR_OK)
  {
    uint8_t work[_MIN_SS];
    if(f_mkfs((TCHAR const*) USERPath, FM_ANY, 0, work, sizeof(work)) != FR_OK)
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

  // release mutex
  osMutexRelease(qspiMutexHandle);

  // check CS43L22 is available
  if(CS43L22_ID != AUDIO_ReadID(AUDIO_I2C_ADDRESS))
  {
    Error_Handler();
  }

  // initialize audio
  if(AUDIO_Init(AUDIO_I2C_ADDRESS, OUTPUT_DEVICE_HEADPHONE, 60, AUDIO_FREQUENCY_44K) != 0)
  {
    Error_Handler();
  }

  /* Infinite loop */
  for(;;)
  {
    osEvent event = osMessageGet(joystickInputQueueHandle, osWaitForever);

    if(event.status == osEventMessage)
    {
      handleJoystickEvent(event);
    }
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
      // check VBUS HIGH 5 times
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
      // stop audio
      audioPlayerStop();

      // get mutex
      osMutexWait(qspiMutexHandle, osWaitForever);

      // initialize USB
      MX_USB_DEVICE_Init();

      // release mutex
      osMutexRelease(qspiMutexHandle);

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

      // get mutex
      osMutexWait(qspiMutexHandle, osWaitForever);

      // de-initialize USB
      USBD_DeInit(&hUsbDeviceFS);

      // release mutex
      osMutexRelease(qspiMutexHandle);

      // Red led off
      HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_RESET);
    }

    osDelay(1000);
  }
  /* USER CODE END usbTaskBody */
}

/* USER CODE BEGIN Header_audioTaskBody */
/**
 * @brief Function implementing the audioTask thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_audioTaskBody */
void audioTaskBody(void const * argument)
{
  /* USER CODE BEGIN audioTaskBody */
  uint32_t bufferOffset = 0;
  uint32_t bytesRead = 0;
  osEvent event;

  /* pause at first */
  osThreadSuspend(audioTaskHandle);

  /* Infinite loop */
  for(;;)
  {
    // wait DMA transmit signal
    event = osSignalWait(A | B, osWaitForever);

    if(event.status == osEventSignal)
    {
      switch(event.value.v)
      {
      case B:
        // full tx complete
        bufferOffset = AUDIO_BUFFER_SIZE / 2;
        break;
      case A:
        // half tx complete
        bufferOffset = 0;
        break;
      default:
        break;
      }

      // acquire mutex
      osMutexWait(qspiMutexHandle, osWaitForever);

      if(f_read(&file, (audioBuffer + bufferOffset), (AUDIO_BUFFER_SIZE/2), (UINT*)&bytesRead) != FR_OK)
      {
        Error_Handler();
      }

      // update position info
      actualPosition += bytesRead;

      // check EOF and move to the start position of wav data
      if((actualPosition + (AUDIO_BUFFER_SIZE / 2)) > f_size(&file))
      {
        if(f_rewind(&file) != FR_OK)
        {
          Error_Handler();
        }

        if(f_lseek(&file, WAV_FILE_HEADER_SIZE) != FR_OK)
        {
          Error_Handler();
        }

        actualPosition = WAV_FILE_HEADER_SIZE;
      }

      // release mutex
      osMutexRelease(qspiMutexHandle);

      // toggle GREEN LED
      HAL_GPIO_TogglePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin);
    }
  }
  /* USER CODE END audioTaskBody */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
/**
 * @brief Tx Transfer completed callbacks.
 * @param  hsai : pointer to a SAI_HandleTypeDef structure that contains
 *                the configuration information for SAI module.
 * @retval None
 */
void HAL_SAI_TxCpltCallback(SAI_HandleTypeDef *hsai)
{
  osSignalSet(audioTaskHandle, B);
}

/**
 * @brief Tx Transfer Half completed callbacks
 * @param  hsai : pointer to a SAI_HandleTypeDef structure that contains
 *                the configuration information for SAI module.
 * @retval None
 */
void HAL_SAI_TxHalfCpltCallback(SAI_HandleTypeDef *hsai)
{
  osSignalSet(audioTaskHandle, A);
}

/**
 * @brief  EXTI line detection callback.
 * @param  GPIO_Pin Specifies the port pin connected to corresponding EXTI line.
 * @retval None
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  switch (GPIO_Pin)
  {
  case JOY_CENTER_Pin:
    osMessagePut(joystickInputQueueHandle, JOY_CENTER_Pin, 0);
    break;
  case JOY_LEFT_Pin:
    osMessagePut(joystickInputQueueHandle, JOY_LEFT_Pin, 0);
    break;
  case JOY_RIGHT_Pin:
    osMessagePut(joystickInputQueueHandle, JOY_RIGHT_Pin, 0);
    break;
  case JOY_UP_Pin:
    osMessagePut(joystickInputQueueHandle, JOY_UP_Pin, 0);
    break;
  case JOY_DOWN_Pin:
    osMessagePut(joystickInputQueueHandle, JOY_DOWN_Pin, 0);
    break;
  default:
    Error_Handler();
    break;
  }
}

// Joystick input handler
static void handleJoystickEvent(osEvent event)
{
  if (event.status != osEventMessage) return;

  switch(AppState)
  {
  case AUDIOPLAYER_STATE_STOPPED:
    if(event.value.v == JOY_RIGHT_Pin)
    {
      audioPlayerStart();
    }
    break;
  case AUDIOPLAYER_STATE_RUNNING:
    if(event.value.v == JOY_CENTER_Pin)
    {
      audioPlayerPause();
    }
    else if(event.value.v == JOY_LEFT_Pin)
    {
      audioPlayerStop();
    }
    break;
  case AUDIOPLAYER_STATE_PAUSED:
    if(event.value.v == JOY_RIGHT_Pin)
    {
      audioPlayerResume();
    }
    else if(event.value.v == JOY_LEFT_Pin)
    {
      audioPlayerStop();
    }
    break;
  default:
    break;
  }
}

/* audio functions */
static void audioPlayerStart(void)
{
  /* Wake-Up external audio Codec and enable output */
  if(0 != AUDIO_Play(AUDIO_I2C_ADDRESS, NULL, 0))
  {
    Error_Handler();
  }

  /* Wait for the qspiMutex */
  osMutexWait(qspiMutexHandle, osWaitForever);

  /* Create and Open a new text file object with write access */
  if(f_open(&file, (TCHAR const*)"audio.wav", FA_READ) != FR_OK)
  {
    Error_Handler();
  }

  /* Skip the WAV header - this we don't want to listen to :) */
  actualPosition = WAV_FILE_HEADER_SIZE;

  /* Move file pointer to appropriate address - keep in mind that the file consists of many blocks in sectors */
  if (f_lseek(&file, WAV_FILE_HEADER_SIZE) != FR_OK)
  {
    Error_Handler();
  }

  /* Release the qspiMutex */
  osMutexRelease(qspiMutexHandle);

  /* Start the SAI DMA transfer from the Buffer */
  if(HAL_OK != HAL_SAI_Transmit_DMA(&hsai_BlockA1, (uint8_t *)audioBuffer, AUDIO_BUFFER_SIZE/2))
  {
    Error_Handler();
  }

  /* update state */
  AppState = AUDIOPLAYER_STATE_RUNNING;

  /* resume task */
  osThreadResume(audioTaskHandle);

}

static void audioPlayerStop(void)
{
  /* Stop the playback */
  if (AUDIO_Stop(AUDIO_I2C_ADDRESS, CODEC_PDWN_SW) != 0)
  {
    Error_Handler();
  }
  /* Stop DMA transfer */
  if (HAL_SAI_DMAStop(&hsai_BlockA1) != HAL_OK)
  {
    Error_Handler();
  }

  /* Wait for the qspiMutex */
  osMutexWait(qspiMutexHandle, osWaitForever);

  /* Pause the audioTask */
  osThreadSuspend(audioTaskHandle);

  /* Close the Audio file */
  if(f_close(&file) != FR_OK)
  {
    Error_Handler();
  }

  /* Release the qspiMutex */
  osMutexRelease(qspiMutexHandle);

  /* Turn OFF the PLAY signaling LED */
  HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_RESET);

  AppState = AUDIOPLAYER_STATE_STOPPED;
}

static void audioPlayerPause(void)
{
  /* Stop the playback */
  if (AUDIO_Pause(AUDIO_I2C_ADDRESS) != 0)
  {
    Error_Handler();
  }
  /* Stop DMA transfer */
  if (HAL_SAI_DMAPause(&hsai_BlockA1) != HAL_OK)
  {
    Error_Handler();
  }

  /* Wait for the qspiMutex */
  osMutexWait(qspiMutexHandle, osWaitForever);

  /* Pause the audioTask */
  osThreadSuspend(audioTaskHandle);

  /* Release the qspiMutex */
  osMutexRelease(qspiMutexHandle);

  /* Turn OFF the PLAY signaling LED */
  HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_RESET);

  AppState = AUDIOPLAYER_STATE_PAUSED;
}

static void audioPlayerResume(void)
{
  /* Resume DMA transfer */
  if (HAL_SAI_DMAResume(&hsai_BlockA1) != HAL_OK)
  {
    Error_Handler();
  }
  /* Resume the playback */
  if (AUDIO_Resume(AUDIO_I2C_ADDRESS) != 0)
  {
    Error_Handler();
  }

  AppState = AUDIOPLAYER_STATE_RUNNING;

  /* Resume audioTask */
  osThreadResume(audioTaskHandle);
}

/* USER CODE END Application */
