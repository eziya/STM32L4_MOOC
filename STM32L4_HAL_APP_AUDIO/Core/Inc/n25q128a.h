/**
 ******************************************************************************
 * @file    n25q128a.h
 * @author  MCD Application Team
 * @brief   This file contains all the description of the N25Q128A QSPI memory.
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2015 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under BSD 3-Clause license,
 * the "License"; You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                        opensource.org/licenses/BSD-3-Clause
 *
 ******************************************************************************
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __N25Q128A_H
#define __N25Q128A_H

#ifdef __cplusplus
 extern "C" {
#endif 

#include "main.h"

/* N25Q128A Configuration */
#define N25Q128A_FLASH_SIZE                  0x1000000 /* 128 MBits => 16MBytes */
#define N25Q128A_SECTOR_SIZE                 0x10000   /* 256 sectors of 64KBytes */
#define N25Q128A_SUBSECTOR_SIZE              0x1000    /* 4096 subsectors of 4kBytes */
#define N25Q128A_PAGE_SIZE                   0x100     /* 65536 pages of 256 bytes */

#define N25Q128A_DUMMY_CYCLES_READ           8
#define N25Q128A_DUMMY_CYCLES_READ_QUAD      10

#define N25Q128A_BULK_ERASE_MAX_TIME         250000
#define N25Q128A_SECTOR_ERASE_MAX_TIME       3000
#define N25Q128A_SUBSECTOR_ERASE_MAX_TIME    800

/* N25Q128A Commands */
/* Reset Operations */
#define RESET_ENABLE_CMD                     0x66
#define RESET_MEMORY_CMD                     0x99

/* Identification Operations */
#define READ_ID_CMD                          0x9E
#define READ_ID_CMD2                         0x9F
#define MULTIPLE_IO_READ_ID_CMD              0xAF
#define READ_SERIAL_FLASH_DISCO_PARAM_CMD    0x5A

/* Read Operations */
#define READ_CMD                             0x03
#define FAST_READ_CMD                        0x0B
#define DUAL_OUT_FAST_READ_CMD               0x3B
#define DUAL_INOUT_FAST_READ_CMD             0xBB
#define QUAD_OUT_FAST_READ_CMD               0x6B
#define QUAD_INOUT_FAST_READ_CMD             0xEB

/* Write Operations */
#define WRITE_ENABLE_CMD                     0x06
#define WRITE_DISABLE_CMD                    0x04

/* Register Operations */
#define READ_STATUS_REG_CMD                  0x05
#define WRITE_STATUS_REG_CMD                 0x01

#define READ_LOCK_REG_CMD                    0xE8
#define WRITE_LOCK_REG_CMD                   0xE5

#define READ_FLAG_STATUS_REG_CMD             0x70
#define CLEAR_FLAG_STATUS_REG_CMD            0x50

#define READ_NONVOL_CFG_REG_CMD              0xB5
#define WRITE_NONVOL_CFG_REG_CMD             0xB1

#define READ_VOL_CFG_REG_CMD                 0x85
#define WRITE_VOL_CFG_REG_CMD                0x81

#define READ_ENHANCED_VOL_CFG_REG_CMD        0x65
#define WRITE_ENHANCED_VOL_CFG_REG_CMD       0x61

/* Program Operations */
#define PAGE_PROG_CMD                        0x02
#define DUAL_IN_FAST_PROG_CMD                0xA2
#define EXT_DUAL_IN_FAST_PROG_CMD            0xD2
#define QUAD_IN_FAST_PROG_CMD                0x32
#define EXT_QUAD_IN_FAST_PROG_CMD            0x12

/* Erase Operations */
#define SUBSECTOR_ERASE_CMD                  0x20
#define SECTOR_ERASE_CMD                     0xD8
#define BULK_ERASE_CMD                       0xC7

#define PROG_ERASE_RESUME_CMD                0x7A
#define PROG_ERASE_SUSPEND_CMD               0x75

/* One-Time Programmable Operations */
#define READ_OTP_ARRAY_CMD                   0x4B
#define PROG_OTP_ARRAY_CMD                   0x42

/* N25Q128A Registers */
/* Status Register */
#define N25Q128A_SR_WIP                      ((uint8_t)0x01)    /*!< Write in progress */
#define N25Q128A_SR_WREN                     ((uint8_t)0x02)    /*!< Write enable latch */
#define N25Q128A_SR_BLOCKPR                  ((uint8_t)0x5C)    /*!< Block protected against program and erase operations */
#define N25Q128A_SR_PRBOTTOM                 ((uint8_t)0x20)    /*!< Protected memory area defined by BLOCKPR starts from top or bottom */
#define N25Q128A_SR_SRWREN                   ((uint8_t)0x80)    /*!< Status register write enable/disable */

/* Nonvolatile Configuration Register */
#define N25Q128A_NVCR_LOCK                   ((uint16_t)0x0001) /*!< Lock nonvolatile configuration register */
#define N25Q128A_NVCR_DUAL                   ((uint16_t)0x0004) /*!< Dual I/O protocol */
#define N25Q128A_NVCR_QUAB                   ((uint16_t)0x0008) /*!< Quad I/O protocol */
#define N25Q128A_NVCR_RH                     ((uint16_t)0x0010) /*!< Reset/hold */
#define N25Q128A_NVCR_ODS                    ((uint16_t)0x01C0) /*!< Output driver strength */
#define N25Q128A_NVCR_XIP                    ((uint16_t)0x0E00) /*!< XIP mode at power-on reset */
#define N25Q128A_NVCR_NB_DUMMY               ((uint16_t)0xF000) /*!< Number of dummy clock cycles */

/* Volatile Configuration Register */
#define N25Q128A_VCR_WRAP                    ((uint8_t)0x03)    /*!< Wrap */
#define N25Q128A_VCR_XIP                     ((uint8_t)0x08)    /*!< XIP */
#define N25Q128A_VCR_NB_DUMMY                ((uint8_t)0xF0)    /*!< Number of dummy clock cycles */

/* Enhanced Volatile Configuration Register */
#define N25Q128A_EVCR_ODS                    ((uint8_t)0x07)    /*!< Output driver strength */
#define N25Q128A_EVCR_VPPA                   ((uint8_t)0x08)    /*!< Vpp accelerator */
#define N25Q128A_EVCR_RH                     ((uint8_t)0x10)    /*!< Reset/hold */
#define N25Q128A_EVCR_DUAL                   ((uint8_t)0x40)    /*!< Dual I/O protocol */
#define N25Q128A_EVCR_QUAD                   ((uint8_t)0x80)    /*!< Quad I/O protocol */

/* Flag Status Register */
#define N25Q128A_FSR_PRERR                   ((uint8_t)0x02)    /*!< Protection error */
#define N25Q128A_FSR_PGSUS                   ((uint8_t)0x04)    /*!< Program operation suspended */
#define N25Q128A_FSR_VPPERR                  ((uint8_t)0x08)    /*!< Invalid voltage during program or erase */
#define N25Q128A_FSR_PGERR                   ((uint8_t)0x10)    /*!< Program error */
#define N25Q128A_FSR_ERERR                   ((uint8_t)0x20)    /*!< Erase error */
#define N25Q128A_FSR_ERSUS                   ((uint8_t)0x40)    /*!< Erase operation suspended */
#define N25Q128A_FSR_READY                   ((uint8_t)0x80)    /*!< Ready or command in progress */

/* N25Q Error codes */
#define N25Q_OK            ((uint8_t)0x00)
#define N25Q_ERROR         ((uint8_t)0x01)
#define N25Q_BUSY          ((uint8_t)0x02)
#define N25Q_NOT_SUPPORTED ((uint8_t)0x04)
#define N25Q_SUSPENDED     ((uint8_t)0x08)

/* N25Q Info */
typedef struct
{
  uint32_t FlashSize; /*!< Size of the flash */
  uint32_t EraseSectorSize; /*!< Size of sectors for the erase operation */
  uint32_t EraseSectorsNumber; /*!< Number of sectors for the erase operation */
  uint32_t ProgPageSize; /*!< Size of pages for the program operation */
  uint32_t ProgPagesNumber; /*!< Number of pages for the program operation */
} N25Q_Info;

/* Functions */
uint8_t N25Q_Init(void);
uint8_t N25Q_DeInit(void);
uint8_t N25Q_Read(uint8_t *pData, uint32_t ReadAddr, uint32_t Size);
uint8_t N25Q_Write(uint8_t *pData, uint32_t WriteAddr, uint32_t Size);
uint8_t N25Q_Erase_Block(uint32_t BlockAddress);
uint8_t N25Q_Erase_Sector(uint32_t Sector);
uint8_t N25Q_Erase_Chip(void);
uint8_t N25Q_GetStatus(void);
uint8_t N25Q_GetInfo(N25Q_Info *pInfo);

#ifdef __cplusplus
}
#endif

#endif /* __N25Q128A_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
