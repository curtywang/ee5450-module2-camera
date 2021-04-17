/**
 * @brief: This file contains function definitions for sensor telemetry on the B-L4S5I-IOT01A board.
 */

#include "sensor_telemetry.h"


/**
 * @brief sends a message with the topic given to the MQTT client
 * @param global_data: pointer to the global data structure
 * @param topic: pointer to the topic buffer (should be null-terminated)
 * @param message: pointer to the message buffer (may be null-terminated)
 * @param message_length: message buffer length if buffer not null-terminated (use 0 otherwise)
 * @return status of the client
 */
UINT send_nx_mqtt_message(struct global_data_t* global_data,
                          char* topic, char* message, size_t message_length) {
    UINT status;
    tx_mutex_get(&global_data->mutex_mqtt, TX_WAIT_FOREVER);
    if (message_length == 0)
        message_length = strlen(message);
    status = nxd_mqtt_client_publish(&global_data->mqtt_client,
                                     topic, strlen(topic),
                                     message, message_length,
                                     0, 1, NX_WAIT_FOREVER);
    tx_mutex_put(&global_data->mutex_mqtt);
    return status;
}


/**
 * @brief format temperature message for sending via some protocol (such as mqtt)
 * @param global_data: pointer to the global data structure
 * @param message: message buffer
 * @param message_size: size of message buffer
 */
void get_temperature_message(struct global_data_t* global_data, char* message, size_t message_size) {
    tx_mutex_get(&global_data->mutex_i2c2, TX_WAIT_FOREVER);
    float temperature = BSP_TSENSOR_ReadTemp();
    uint32_t current_tick = tx_time_get();
    tx_mutex_put(&global_data->mutex_i2c2);
    snprintf(message, message_size, "time: %lu, temp: %.2f degC", current_tick, temperature);
}


/**
 * @brief format accelerometer message for sending via some protocol (such as mqtt)
 * @param global_data: pointer to the global data structure
 * @param message: message buffer
 * @param message_size: size of message buffer
 */
void get_accelerometer_message(struct global_data_t* global_data, char* message, size_t message_size) {
    int16_t current_xyz[3];
    tx_mutex_get(&global_data->mutex_i2c2, TX_WAIT_FOREVER);
    BSP_ACCELERO_AccGetXYZ(current_xyz);
    uint32_t current_tick = tx_time_get();
    tx_mutex_put(&global_data->mutex_i2c2);
    snprintf(message, message_size, "time: %lu, xl: %.2hd %.2hd %.2hd",
             current_tick, current_xyz[0], current_xyz[1], current_xyz[2]);
}


/**
 * @brief fill the framebuffer with a capture and get number of bytes of image
 * @param global_data: poitner to the global data structure
 * @return size of the image in number of bytes
 */
size_t get_framebuffer_from_camera(struct global_data_t* global_data) {
    size_t num_bytes = arducam_read_image(FRAMEBUFFER_SIZE, global_data->framebuffer);
    return num_bytes;
}


/* TODO: put your function definitions for the other sensors here here */
