#ifndef W25Q64JV_H_
#define W25Q64JV_H_

#include "main.h"
#include <stdint.h>

typedef struct {
    void* comms_handle;
    uint8_t comms_mode;
    GPIO_TypeDef* cs_port;
    uint16_t cs_pin;
} w25q64jv_cfg_t;


#endif /* W25Q64JV_H_ */