/**
 * @file sdmmc.h
 * @brief Giao diện giao tiếp thẻ nhớ MicroSD qua khối SDMMC1 chuẩn SDHC (4GB - 32GB)
 */

#ifndef SDMMC_H
#define SDMMC_H

#include <stdint.h>

#define SD_BLOCK_SIZE       512U
#define SD_OK               0U
#define SD_ERROR            1U
#define SD_TIMEOUT          2U

/* Định danh phân loại thẻ theo chuẩn SD Physical Layer Spec */
#define SD_TYPE_UNKNOWN     0U
#define SD_TYPE_SDSC        1U  /* Standard Capacity (<= 2GB, Byte Addressing) */
#define SD_TYPE_SDHC        2U  /* High Capacity (4GB - 32GB, Block Addressing LBA) */

uint8_t SDMMC_Init(void);
uint8_t SDMMC_GetCardType(void);
const char *SDMMC_GetCardTypeName(void);
uint8_t SDMMC_ReadSingleBlock(uint32_t block_addr, uint8_t *pBuffer);
uint8_t SDMMC_ReadMultiBlocks(uint32_t block_addr, uint8_t *pBuffer, uint32_t num_blocks);

#endif /* SDMMC_H */