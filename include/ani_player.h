#ifndef ANI_PLAYER_H
#define ANI_PLAYER_H

#include "lvgl/lvgl.h"
#include <stdbool.h> // Wichtig für 'bool'

typedef struct {
    const uint8_t ** data; // Pointer auf ein Array von Byte-Pointern
    uint16_t frames;
    uint16_t width;
    uint16_t height;
    uint32_t interval_ms;
    bool has_alpha;        // Das hat in deiner Datei gefehlt!
} ani_resource_t;


lv_obj_t * ani_player_create(lv_obj_t * parent, const void * res);

#endif