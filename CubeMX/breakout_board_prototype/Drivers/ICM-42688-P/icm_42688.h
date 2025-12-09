#ifndef ICM_42688_H_
#define ICM_42688_H_

#include "main.h"
#include <stdint.h>

typedef struct {
    void* comms_handle;
    GPIO_TypeDef* gpio_port;
    uint16_t gpio_pin;
    uint8_t packet_no;
} icm_42688_cfg_t;

int reset_device(icm_42688_cfg_t* hw_cfg);

int icm_42688_config(icm_42688_cfg_t* hw_cfg, void* comms_handle, GPIO_TypeDef* gpio_port, uint16_t gpio_pin);

/// unless otherwise noted this default value must be maintained even if the values of other register fields are modified by the user.

#endif