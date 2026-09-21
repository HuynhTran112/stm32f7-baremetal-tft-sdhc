/**
 * @file diskio.c
 * @brief Tầng cầu nối phần cứng (Hardware Glue Layer) giữa ChaN FatFs và Bare-Metal SDMMC1
 */

#include "diskio.h"
#include "sdmmc.h"

DSTATUS disk_initialize(BYTE pdrv)
{
    if (pdrv != 0) return STA_NOINIT;
    
    if (SDMMC_Init() == SD_OK)
    {
        return 0; /* Khởi tạo thành công, xóa cờ STA_NOINIT */
    }
    return STA_NOINIT;
}

DSTATUS disk_status(BYTE pdrv)
{
    if (pdrv != 0) return STA_NOINIT;
    return 0; /* Ổ đĩa luôn sẵn sàng */
}

DRESULT disk_read(BYTE pdrv, BYTE *buff, DWORD sector, UINT count)
{
    if (pdrv != 0 || count == 0) return RES_PARERR;

    if (SDMMC_ReadMultiBlocks((uint32_t)sector, buff, (uint32_t)count) == SD_OK)
    {
        return RES_OK;
    }
    return RES_ERROR;
}

DRESULT disk_write(BYTE pdrv, const BYTE *buff, DWORD sector, UINT count)
{
    (void)pdrv;
    (void)buff;
    (void)sector;
    (void)count;
    return RES_WRPRT; /* Chế độ chỉ đọc video */
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
    if (pdrv != 0) return RES_PARERR;

    switch (cmd)
    {
        case CTRL_SYNC:
            return RES_OK;

        case GET_SECTOR_COUNT:
            *(DWORD *)buff = 15500000UL; /* ~8 GB */
            return RES_OK;

        case GET_SECTOR_SIZE:
            *(WORD *)buff = 512;
            return RES_OK;

        case GET_BLOCK_SIZE:
            *(DWORD *)buff = 1;
            return RES_OK;

        default:
            return RES_PARERR;
    }
}

DWORD get_fattime(void)
{
    /* Trả về timestamp mặc định: 2026-09-12 12:00:00 */
    return ((DWORD)(2026 - 1980) << 25) |
           ((DWORD)9 << 21) |
           ((DWORD)12 << 16) |
           ((DWORD)12 << 11) |
           ((DWORD)0 << 5) |
           ((DWORD)0 >> 1);
}
