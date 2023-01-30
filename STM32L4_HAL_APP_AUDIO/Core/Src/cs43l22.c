#include "cs43l22.h"
#include "i2c.h"
#include "gpio.h"

static uint8_t Is_AUDIO_Stop = 1;
volatile uint8_t OutputDev = 0;

static uint8_t CODEC_IO_Write(uint8_t Addr, uint8_t Reg, uint8_t Value);
static uint8_t CODEC_IO_Read(uint8_t Addr, uint8_t Reg);
static HAL_StatusTypeDef I2C1_WriteBuffer(uint16_t Addr, uint16_t Reg, uint16_t RegSize, uint8_t *pBuffer, uint16_t Length);
static HAL_StatusTypeDef I2C1_ReadBuffer(uint16_t Addr, uint16_t Reg, uint16_t RegSize, uint8_t *pBuffer, uint16_t Length);
static void I2C1_Error(void);

uint32_t AUDIO_Init(uint16_t DeviceAddr, uint16_t OutputDevice, uint8_t Volume, uint32_t AudioFreq)
{
  uint32_t counter = 0;
  uint8_t regVal;

  //(1): Reset IC
  AUDIO_IO_Init();

  //(2): Power down
  regVal = 0x01;
  counter += CODEC_IO_Write(DeviceAddr, CS43L22_REG_POWER_CTL1, regVal);

  //(3): Select Output Device
  switch(OutputDevice)
  {
  case OUTPUT_DEVICE_SPEAKER:
    OutputDev = 0xFA;
    break;
  case OUTPUT_DEVICE_HEADPHONE:
    OutputDev = 0xAF;
    break;
  case OUTPUT_DEVICE_BOTH:
    OutputDev = 0xAA;
    break;
  case OUTPUT_DEVICE_AUTO:
    OutputDev = 0x05;
    break;
  default:
    OutputDev = 0x05;
    break;
  }
  counter += CODEC_IO_Write(DeviceAddr, CS43L22_REG_POWER_CTL2, OutputDev);

  //(4): Automatic clock detection
  regVal = (1 << 7);
  counter += CODEC_IO_Write(DeviceAddr, CS43L22_REG_CLOCKING_CTL, regVal);

  //(5): Interface control 1
  regVal = CODEC_IO_Read(DeviceAddr, CS43L22_REG_INTERFACE_CTL1);
  regVal &= ~(1 << 7);
  regVal &= (1 << 5);
  regVal &= ~(1 << 6);
  regVal &= ~(1 << 4);
  regVal &= ~(1 << 2);
  regVal |= (1 << 2);
  regVal |= (3 << 0);
  counter += CODEC_IO_Write(DeviceAddr, CS43L22_REG_INTERFACE_CTL1, regVal);

  //(6): Passthrough A settings
  regVal = CODEC_IO_Read(DeviceAddr, CS43L22_REG_PASSTHR_A_SELECT);
  regVal &= 0xF0;
  regVal |= (1 << 0);
  counter += CODEC_IO_Write(DeviceAddr, CS43L22_REG_PASSTHR_A_SELECT, regVal);

  //(7): Passthrough B settings
  regVal = CODEC_IO_Read(DeviceAddr, CS43L22_REG_PASSTHR_B_SELECT);
  regVal &= 0xF0;
  regVal |= (1 << 0);
  counter += CODEC_IO_Write(DeviceAddr, CS43L22_REG_PASSTHR_B_SELECT, regVal);

  //(8): Set master volume
  counter += AUDIO_SetVolume(DeviceAddr, Volume);

  /* If the Speaker is enabled, set the Mono mode and volume attenuation level */
  if(OutputDevice != OUTPUT_DEVICE_HEADPHONE)
  {
    /* Set the Speaker Mono mode */
    counter += CODEC_IO_Write(DeviceAddr, CS43L22_REG_PLAYBACK_CTL1, 0x06);

    /* Set the Speaker attenuation level */
    counter += CODEC_IO_Write(DeviceAddr, CS43L22_REG_SPEAKER_A_VOL, 0x00);
    counter += CODEC_IO_Write(DeviceAddr, CS43L22_REG_SPEAKER_B_VOL, 0x00);
  }

  /* Low power configuration
   Additional configuration for the CODEC. These configurations are done to reduce
   the time needed for the Codec to power off. If these configurations are removed,
   then a long delay should be added between powering off the Codec and switching
   off the I2S peripheral MCLK clock (which is the operating clock for Codec).
   If this delay is not inserted, then the codec will not shut down properly and
   it results in high noise after shut down. */

  /* Disable the analog soft ramp */
  counter += CODEC_IO_Write(DeviceAddr, CS43L22_REG_ANALOG_ZC_SR_SETT, 0x00);
  /* Disable the digital soft ramp */
  counter += CODEC_IO_Write(DeviceAddr, CS43L22_REG_MISC_CTL, 0x04);
  /* Disable the limiter attack level */
  counter += CODEC_IO_Write(DeviceAddr, CS43L22_REG_LIMIT_CTL1, 0x00);
  /* Adjust Bass and Treble levels */
  counter += CODEC_IO_Write(DeviceAddr, CS43L22_REG_TONE_CTL, 0x0F);
  /* Adjust PCM volume level */
  counter += CODEC_IO_Write(DeviceAddr, CS43L22_REG_PCMA_VOL, 0x0A);
  counter += CODEC_IO_Write(DeviceAddr, CS43L22_REG_PCMB_VOL, 0x0A);

  /* Return communication control value */
  return counter;
}

void AUDIO_DeInit(void)
{
  AUDIO_IO_DeInit();
}

uint32_t AUDIO_ReadID(uint16_t DeviceAddr)
{
  uint8_t Value;

  AUDIO_IO_Init();
  Value = AUDIO_IO_Read(DeviceAddr, CS43L22_REG_ID);
  Value = (Value & CS43L22_ID_MASK);

  return ((uint32_t) Value);
}

uint32_t AUDIO_Play(uint16_t DeviceAddr, uint16_t *pBuffer, uint16_t Size)
{
  uint32_t counter = 0;

  if(Is_AUDIO_Stop == 1)
  {
    counter += AUDIO_SetMute(DeviceAddr, AUDIO_MUTE_OFF);
    /* Power on the Codec */
    counter += CODEC_IO_Write(DeviceAddr, CS43L22_REG_POWER_CTL1, 0x9E);
    Is_AUDIO_Stop = 0;
  }

  return counter;
}

uint32_t AUDIO_Pause(uint16_t DeviceAddr)
{
  uint32_t counter = 0;

  counter += AUDIO_SetMute(DeviceAddr, AUDIO_MUTE_ON);
  /* Put the Codec in Power save mode */
  counter += CODEC_IO_Write(DeviceAddr, CS43L22_REG_POWER_CTL1, 0x01);

  return counter;
}

uint32_t AUDIO_Resume(uint16_t DeviceAddr)
{
  uint32_t counter = 0;
  volatile uint32_t index = 0x00;
  counter += AUDIO_SetMute(DeviceAddr, AUDIO_MUTE_OFF);

  for(index = 0x00;index < 0xFF;index++)
  {
    ;
  }

  /* Configure output device */
  counter += CODEC_IO_Write(DeviceAddr, CS43L22_REG_POWER_CTL2, OutputDev);

  /* Exit the Power save mode */
  counter += CODEC_IO_Write(DeviceAddr, CS43L22_REG_POWER_CTL1, 0x9E);

  return counter;
}

uint32_t AUDIO_Stop(uint16_t DeviceAddr, uint32_t CodecPdwnMode)
{
  uint32_t counter = 0;

  counter += AUDIO_SetMute(DeviceAddr, AUDIO_MUTE_ON);

  /* Power down the DAC and the speaker (PMDAC and PMSPK bits)*/
  counter += CODEC_IO_Write(DeviceAddr, CS43L22_REG_POWER_CTL1, 0x9F);

  Is_AUDIO_Stop = 1;
  return counter;
}

uint32_t AUDIO_SetVolume(uint16_t DeviceAddr, uint8_t Volume)
{
  uint32_t counter = 0;
  uint8_t convertedvol = VOLUME_CONVERT(Volume);

  if(Volume > 0xE6)
  {
    counter += CODEC_IO_Write(DeviceAddr, CS43L22_REG_MASTER_A_VOL, convertedvol - 0xE7);
    counter += CODEC_IO_Write(DeviceAddr, CS43L22_REG_MASTER_B_VOL, convertedvol - 0xE7);
  }
  else
  {
    counter += CODEC_IO_Write(DeviceAddr, CS43L22_REG_MASTER_A_VOL, convertedvol + 0x19);
    counter += CODEC_IO_Write(DeviceAddr, CS43L22_REG_MASTER_B_VOL, convertedvol + 0x19);
  }

  return counter;
}

uint32_t AUDIO_SetFrequency(uint16_t DeviceAddr, uint32_t AudioFreq)
{
  return 0;
}

uint32_t AUDIO_SetMute(uint16_t DeviceAddr, uint32_t Cmd)
{
  uint32_t counter = 0;

  if(Cmd == AUDIO_MUTE_ON)
  {
    counter += CODEC_IO_Write(DeviceAddr, CS43L22_REG_POWER_CTL2, 0xFF);
  }
  else
  {
    counter += CODEC_IO_Write(DeviceAddr, CS43L22_REG_POWER_CTL2, OutputDev);
  }
  return counter;
}

uint32_t AUDIO_SetOutputMode(uint16_t DeviceAddr, uint8_t Output)
{
  uint32_t counter = 0;

  switch(Output)
  {
  case OUTPUT_DEVICE_SPEAKER:
    /* SPK always ON & HP always OFF */
    counter += CODEC_IO_Write(DeviceAddr, CS43L22_REG_POWER_CTL2, 0xFA);
    OutputDev = 0xFA;
    break;
  case OUTPUT_DEVICE_HEADPHONE:
    /* SPK always OFF & HP always ON */
    counter += CODEC_IO_Write(DeviceAddr, CS43L22_REG_POWER_CTL2, 0xAF);
    OutputDev = 0xAF;
    break;
  case OUTPUT_DEVICE_BOTH:
    /* SPK always ON & HP always ON */
    counter += CODEC_IO_Write(DeviceAddr, CS43L22_REG_POWER_CTL2, 0xAA);
    OutputDev = 0xAA;
    break;
  case OUTPUT_DEVICE_AUTO:
    /* Detect the HP or the SPK automatically */
    counter += CODEC_IO_Write(DeviceAddr, CS43L22_REG_POWER_CTL2, 0x05);
    OutputDev = 0x05;
    break;
  default:
    /* Detect the HP or the SPK automatically */
    counter += CODEC_IO_Write(DeviceAddr, CS43L22_REG_POWER_CTL2, 0x05);
    OutputDev = 0x05;
    break;
  }
  return counter;
}

uint32_t AUDIO_Reset(uint16_t DeviceAddr)
{
  return 0;
}

static uint8_t CODEC_IO_Write(uint8_t Addr, uint8_t Reg, uint8_t Value)
{
  uint32_t result = 0;
  AUDIO_IO_Write(Addr, Reg, Value);
  return result;
}

static uint8_t CODEC_IO_Read(uint8_t Addr, uint8_t Reg)
{
  return AUDIO_IO_Read(Addr, Reg);
}

void AUDIO_IO_Init(void)
{
  /* Power Down the codec */
  HAL_GPIO_WritePin(AUDIO_RESET_GPIO_Port, AUDIO_RESET_Pin, GPIO_PIN_RESET);
  HAL_Delay(5);
  /* Power on the codec */
  HAL_GPIO_WritePin(AUDIO_RESET_GPIO_Port, AUDIO_RESET_Pin, GPIO_PIN_SET);
  HAL_Delay(5);
}

void AUDIO_IO_Write(uint8_t Addr, uint8_t Reg, uint8_t Value)
{
  I2C1_WriteBuffer(Addr, (uint16_t) Reg, I2C_MEMADD_SIZE_8BIT, &Value, 1);
}

uint8_t AUDIO_IO_Read(uint8_t Addr, uint8_t Reg)
{
  uint8_t Read_Value = 0;
  I2C1_ReadBuffer((uint16_t) Addr, (uint16_t) Reg, I2C_MEMADD_SIZE_8BIT, &Read_Value, 1);
  return Read_Value;
}

void AUDIO_IO_Delay(uint32_t Delay)
{
  HAL_Delay(Delay);
}

static HAL_StatusTypeDef I2C1_WriteBuffer(uint16_t Addr, uint16_t Reg, uint16_t RegSize, uint8_t *pBuffer, uint16_t Length)
{
  HAL_StatusTypeDef status = HAL_OK;
  status = HAL_I2C_Mem_Write(&hi2c1, Addr, (uint16_t) Reg, RegSize, pBuffer, Length, 3000);
  if(status != HAL_OK)
  {
    I2C1_Error();
  }

  return status;
}

static HAL_StatusTypeDef I2C1_ReadBuffer(uint16_t Addr, uint16_t Reg, uint16_t RegSize, uint8_t *pBuffer, uint16_t Length)
{
  HAL_StatusTypeDef status = HAL_OK;
  status = HAL_I2C_Mem_Read(&hi2c1, Addr, (uint16_t) Reg, RegSize, pBuffer, Length, 3000);
  if(status != HAL_OK)
  {
    I2C1_Error();
  }

  return status;
}

static void I2C1_Error(void)
{
  HAL_I2C_DeInit(&hi2c1);
  MX_I2C1_Init();
}
