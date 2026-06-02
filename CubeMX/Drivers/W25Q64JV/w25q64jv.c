#include "w25q64jv_registers.h"
#include "w25q64jv.h"

int w25q64jv_config(w25q64jv_cfg_t* hw_cfg, void* comms_handle, GPIO_TypeDef* cs_port, uint16_t cs_pin) {

    hw_cfg->comms_handle = comms_handle;
    hw_cfg->comms_mode = 1;
    hw_cfg->cs_pin = cs_pin;
    hw_cfg->cs_port = cs_port;

    #ifdef QUADSPI
    if (comms_handle == QUADSPI){
        hw_cfg->comms_mode = 4;
    }
    #endif

    return 0;
};

int cs_high(w25q64jv_cfg_t* hw_cfg){
    if (hw_cfg->cs_port == NULL) { return -1; }
    HAL_GPIO_WritePin(hw_cfg->cs_port, hw_cfg->cs_pin, GPIO_PIN_SET);
    return 0;
}

int cs_low(w25q64jv_cfg_t* hw_cfg){
    if (hw_cfg->cs_port == NULL) { return -1; }
    HAL_GPIO_WritePin(hw_cfg->cs_port, hw_cfg->cs_pin, GPIO_PIN_RESET);
    return 0;
}

int write_enable(w25q64jv_cfg_t* hw_cfg){ 
    // HAL_QSPI_Transmit(QSPI_HandleTypeDef *hqspi, uint8_t *pData, uint32_t Timeout)
    return -1; 
}
int volatile_sr_write_enable(w25q64jv_cfg_t* hw_cfg){ 
    return -1; 
}
int write_disable(w25q64jv_cfg_t* hw_cfg){ 
    return -1; 
}
int release_power_down_id(w25q64jv_cfg_t* hw_cfg){ 
    return -1; 
}
int manufacturer_device_id(w25q64jv_cfg_t* hw_cfg){ 
    return -1; 
}
int jedec_id(w25q64jv_cfg_t* hw_cfg){ 
    return -1; 
}
int read_unique_id(w25q64jv_cfg_t* hw_cfg){ 
    return -1; 
}
int read_data(w25q64jv_cfg_t* hw_cfg){ 
    return -1; 
}
int fast_read(w25q64jv_cfg_t* hw_cfg){ 
    return -1; 
}
int page_program(w25q64jv_cfg_t* hw_cfg){ 
    return -1; 
}
int sector_erase_4KB(w25q64jv_cfg_t* hw_cfg){ 
    return -1; 
}
int block_erase_32KB(w25q64jv_cfg_t* hw_cfg){ 
    return -1; 
}
int block_erase_64KB(w25q64jv_cfg_t* hw_cfg){ 
    return -1; 
}
int chip_erase(w25q64jv_cfg_t* hw_cfg){ 
    return -1; 
}
int read_status_register_1(w25q64jv_cfg_t* hw_cfg){ 
    return -1; 
}
int write_status_register_1(w25q64jv_cfg_t* hw_cfg){ 
    return -1; 
}
int read_status_register_2(w25q64jv_cfg_t* hw_cfg){ 
    return -1; 
}
int write_status_register_2(w25q64jv_cfg_t* hw_cfg){ 
    return -1; 
}
int read_status_register_3(w25q64jv_cfg_t* hw_cfg){ 
    return -1; 
}
int write_status_register_3(w25q64jv_cfg_t* hw_cfg){ 
    return -1; 
}
int read_sfdp_register(w25q64jv_cfg_t* hw_cfg){ 
    return -1; 
}
int erase_security_register(w25q64jv_cfg_t* hw_cfg){ 
    return -1; 
}
int program_security_register(w25q64jv_cfg_t* hw_cfg){ 
    return -1; 
}
int read_security_register(w25q64jv_cfg_t* hw_cfg){ 
    return -1; 
}
int global_block_lock(w25q64jv_cfg_t* hw_cfg){ 
    return -1; 
}
int global_block_unlock(w25q64jv_cfg_t* hw_cfg){ 
    return -1; 
}
int read_block_lock(w25q64jv_cfg_t* hw_cfg){ 
    return -1; 
}
int individual_block_lock(w25q64jv_cfg_t* hw_cfg){ 
    return -1; 
}
int individual_block_unlock(w25q64jv_cfg_t* hw_cfg){ 
    return -1; 
}
int erase_program_suspend(w25q64jv_cfg_t* hw_cfg){ 
    return -1; 
}
int erase_program_resume(w25q64jv_cfg_t* hw_cfg){ 
    return -1; 
}
int power_down(w25q64jv_cfg_t* hw_cfg){ 
    return -1; 
}
int enable_reset(w25q64jv_cfg_t* hw_cfg){ 
    return -1; 
}
int reset_device(w25q64jv_cfg_t* hw_cfg){ 
    return -1; 
}

int fast_read_dual_output(w25q64jv_cfg_t* hw_cfg){ 
    return -1; 
}
int fast_read_dual_io(w25q64jv_cfg_t* hw_cfg){ 
    return -1; 
}
int mftr_device_id_dual_io(w25q64jv_cfg_t* hw_cfg){ 
    return -1; 
}
int quad_input_page_program(w25q64jv_cfg_t* hw_cfg){ 
    return -1; 
}
int fast_read_quad_output(w25q64jv_cfg_t* hw_cfg){ 
    return -1; 
}
int mftr_device_id_quad_io(w25q64jv_cfg_t* hw_cfg){ 
    return -1; 
}
int fast_read_quad_io(w25q64jv_cfg_t* hw_cfg){ 
    return -1; 
}
int set_burst_with_wrap(w25q64jv_cfg_t* hw_cfg){ 
    return -1; 
}