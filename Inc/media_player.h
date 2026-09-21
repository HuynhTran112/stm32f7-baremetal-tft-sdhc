/**
 * @file media_player.h
 * @brief Giao diện trình phát Video FAT32, Menu chọn tệp và Demo đồ họa 60 FPS
 */

#ifndef MEDIA_PLAYER_H
#define MEDIA_PLAYER_H

#include <stdint.h>
#include "ff.h"

#define MAX_VIDEO_FILES     8U
#define SECTORS_PER_FRAME   510U

typedef struct {
    char     filename[16];
    uint32_t size_bytes;
    uint32_t total_frames;
} VideoFileInfo_t;

uint8_t MediaPlayer_Init_FAT(void);
uint8_t MediaPlayer_ScanVideos(VideoFileInfo_t *file_list, uint8_t max_files);
uint8_t MediaPlayer_SelectVideoUI(VideoFileInfo_t *files, uint8_t file_count);
void MediaPlayer_PlayFile(const char *filename);
void MediaPlayer_RunGraphicsDemo(void);

void LCD_DrawChar(uint32_t fb_addr, uint16_t x, uint16_t y, char c, uint16_t fg, uint16_t bg);
void LCD_DrawString(uint32_t fb_addr, uint16_t x, uint16_t y, const char *str, uint16_t fg, uint16_t bg);

#endif /* MEDIA_PLAYER_H */
