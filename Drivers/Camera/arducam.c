//
// Created by curty on 4/6/2021.
//

#include "arducam.h"

const uint8_t ARDU_WRITE = 0x80;
const uint8_t ARDU_REG_FIFO_CONTROL = 0x04;
const uint8_t ARDU_REG_SENSOR_CONTROL = 0x06;
const uint8_t ARDU_REG_VERSION = 0x40;
const uint8_t ARDU_REG_FIFO_BURST = 0x3C;
const uint8_t ARDU_REG_FIFO_SINGLE = 0x3D;
const uint8_t ARDU_REG_FIFO_STATUS = 0x41;
const uint8_t ARDU_REG_FIFO_SIZE_LSB = 0x42;
const uint8_t ARDU_REG_FIFO_SIZE_ISB = 0x43;
const uint8_t ARDU_REG_FIFO_SIZE_MSB = 0x44;

const uint8_t ARDU_FIFO_CLEAR = 0x01;
const uint8_t ARDU_FIFO_START = 0x02;
const uint8_t ARDU_FIFO_READY = 0x08;

SPI_HandleTypeDef* lib_phspi;
GPIO_TypeDef* lib_ardu_cs_port;
uint16_t lib_ardu_cs_pin;
bool lib_waiting = false;

/**
 * @brief write to a register on the arduchip
 * @param ardu_reg: register on the arduchip to write to
 * @param data: data to write to the register
 */
void arduchip_reg_write(uint8_t ardu_reg, uint8_t data) {
    uint8_t command = ardu_reg | ARDU_WRITE;
    HAL_GPIO_WritePin(lib_ardu_cs_port, lib_ardu_cs_pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(lib_phspi, &command, 1, 0xFFFF);
    HAL_SPI_Transmit(lib_phspi, &data, 1, 0xFFFF);
    HAL_GPIO_WritePin(lib_ardu_cs_port, lib_ardu_cs_pin, GPIO_PIN_SET);
}


/**
 * @brief read from a register on the arduchip
 * @param ardu_reg: register on the arduchip to read from
 * @return value of read on register
 */
uint8_t arduchip_reg_read(uint8_t ardu_reg) {
    uint8_t data_out = 0x00;
    uint8_t data_reg = ardu_reg;
    volatile HAL_StatusTypeDef status;
    HAL_GPIO_WritePin(lib_ardu_cs_port, lib_ardu_cs_pin, GPIO_PIN_RESET);
    HAL_SPI_TransmitReceive(lib_phspi, &data_reg, &data_out, 1, 100);
    data_out = 0x00;
    HAL_SPI_TransmitReceive(lib_phspi, &data_out, &data_out, 1, 100);
    HAL_GPIO_WritePin(lib_ardu_cs_port, lib_ardu_cs_pin, GPIO_PIN_SET);
    return data_out;
}


/**
 * @brief read the FIFO on the arduchip
 * @param framebuffer_size: framebuffer size
 * @param framebuffer: pointer to the framebuffer (make sure at least 384K)
 * @return number of bytes written to the framebuffer
 */
size_t arduchip_fifo_read(uint32_t framebuffer_size, uint8_t* framebuffer) {
    size_t fifo_size = 0;
    uint8_t command_buffer = ARDU_REG_FIFO_BURST;
    fifo_size = arduchip_reg_read(ARDU_REG_FIFO_SIZE_MSB) << 16;
    fifo_size |= arduchip_reg_read(ARDU_REG_FIFO_SIZE_ISB) << 8;
    fifo_size |= arduchip_reg_read(ARDU_REG_FIFO_SIZE_LSB);
    if (fifo_size > framebuffer_size)
        fifo_size = framebuffer_size;  // truncate to framebuffer size
    HAL_GPIO_WritePin(lib_ardu_cs_port, lib_ardu_cs_pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(lib_phspi, &command_buffer, 1, 0xFFFF);
    for (size_t i = 0; i < fifo_size; i += 1024) {  // 1K-sized chunks
        HAL_SPI_Receive(lib_phspi, &framebuffer[i], 1024, 0xFFFF);
    }
    HAL_GPIO_WritePin(lib_ardu_cs_port, lib_ardu_cs_pin, GPIO_PIN_SET);
    return fifo_size;
}


/**
 * @brief initialize the arduchip data structures and register
 * @param p_hspi: pointer to the SPI bus handle
 * @param ardu_cs_port: chip select GPIO port
 * @param ardu_cs_pin: chip select GPIO pin
 */
void arducam_init(SPI_HandleTypeDef* p_hspi, GPIO_TypeDef* ardu_cs_port, uint16_t ardu_cs_pin) {
    lib_phspi = p_hspi;
    lib_ardu_cs_port = ardu_cs_port;
    lib_ardu_cs_pin = ardu_cs_pin;
    arduchip_reg_read(0x00);
}


/**
 * @brief clear the arduchip's fifo
 */
void arducam_clear_capture() {
    arduchip_reg_write(ARDU_REG_FIFO_CONTROL, ARDU_FIFO_CLEAR);
    lib_waiting = false;
}


/**
 * @brief start an image capture
 */
void arducam_start_capture() {
    arducam_clear_capture();
    arduchip_reg_write(ARDU_REG_FIFO_CONTROL, ARDU_FIFO_START);
    lib_waiting = true;
}


/**
 * @brief get the arduchip version
 * @return the value of the arduchip version
 */
uint8_t arducam_get_version() {
    return arduchip_reg_read(ARDU_REG_VERSION);
}


/**
 * @brief get the current fifo status
 * @return the fifo status
 */
uint8_t arducam_get_fifo_status() {
    return arduchip_reg_read(ARDU_REG_FIFO_STATUS);
}


/**
 * @brief get the arduchip's mfg year
 * @return the arduchip's mfg year from 2000
 */
uint8_t arducam_get_version_year() {
    return arduchip_reg_read(0x46);
}


/**
 * @brief read the captured image into a local framebuffer,
 *        will start capture automatically; size checking implemented
 * @param framebuffer_size: maximum size of the framebuffer
 * @param framebuffer: pointer to the framebuffer
 * @return the number of bytes written to the framebuffer
 */
size_t arducam_read_image(uint32_t framebuffer_size, uint8_t* framebuffer) {
    uint8_t status = 0;
    size_t num_bytes = 0;
    if (!lib_waiting)
        arducam_start_capture();
    while ((status & ARDU_FIFO_READY) != ARDU_FIFO_READY) {
        status = arduchip_reg_read(ARDU_REG_FIFO_STATUS);
    }
    num_bytes = arduchip_fifo_read(framebuffer_size, framebuffer);
    arducam_clear_capture();
    return num_bytes;
}
