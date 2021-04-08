//
// Created by curty on 4/6/2021.
//

#ifndef EE5450_MODULE2_HW0_ARDUCAM_H
#define EE5450_MODULE2_HW0_ARDUCAM_H

#include "main.h"
#include "stm32l4xx.h"

void arducam_init(SPI_HandleTypeDef* p_hspi, GPIO_TypeDef* ardu_cs_port, uint16_t ardu_cs_pin);
void arducam_start_capture();
size_t arducam_read_image(uint32_t framebuffer_size, uint8_t* framebuffer);
uint8_t arducam_get_version();
uint8_t arducam_get_fifo_status();
uint8_t arducam_get_version_year();

#endif //EE5450_MODULE2_HW0_ARDUCAM_H
