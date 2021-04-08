# ee5450-module2-camera: ArduCam 2MP Mini Plus demo with ThreadX

This repo builds upon the previous repos, but adds the camera driver and modifies the global
data structure to include a framebuffer to hold the current image to transfer via MQTT.

## Project file structure
| File name | Purpose |
| ------------------- | ------- |
| `Core/Inc/main.h` | System data structures |
| `Core/Src/main.c` | Thread setup and entry functions |
| `Core/Inc/sensor_telemetry_setup.h` | Constants for MQTT telemetry setup |
| `Core/Src/sensor_telemetry_setup.c` | Functions to setup NetX Duo and MQTT client for sensor telemetry |
| `Core/Inc/sensor_telemetry.h` | Declarations for sensor telemetry functions (MQTT message publishing) |
| `Core/Inc/sensor_telemetry.h` | Definitions for sensor telemetry functions (MQTT message publishing) |
| `mosquitto.conf` | Mosquitto configuration file (run `mosquitto.exe` with `-c` switch) |
| **NEW:** `Drivers/Camera/arducam.h` | Declarations for ArduCam/ArduChip SPI functions |
| **NEW:** `Drivers/Camera/arducam.c` | Definitions for ArduCam/ArduChip SPI functions |
| **NEW:** `Drivers/Camera/ov2640.h` | Declarations for OV2640 image sensor I2C functions |
| **NEW:** `Drivers/Camera/ov2640.c` | Definitions for OV2640 image sensor I2C functions |

## System data structures
### `main.h`: `struct global_data_t`: Global data structure
There is a `struct` type named `global_data_t`.  
This data structure replaces our global data to allow us to use 
test harnesses in the future.  Rather than having the
test harness attempt to modify globals, the test harness can generate
dummy `struct global_data_t` objects instead. 

This data structure contains members to ensure the operation of the system
successfully, including the ThreadX primitives and structures, NetX primitives
and structures, and the thread input parameters. This also allows for
threads and functions that require multiple inputs to be used with the
ThreadX `tx_thread_create()` interface, which only accepts one `ULONG` 
input (which will be the pointer to the global data structure).

Note the one caveat, that this type definition is available globally
in `main.c`, but the actual pointer is only allocated inside
`tx_application_define` to take advantage of ThreadX's byte allocation
feature.

### `main.c`: `extern TX_EVENT_FLAGS_GROUP global_event_flags`: system-wide software interrupts
This global event flag group is to provide system-wide software interrupts. 
The event `#define`s are public in `main.h` and begin with `EVT_`.


## Task to complete this assignment
Your tasks are to modify `main.h`, `main.c`, `sensor_telemetry.h`, and `sensor_telemetry.c`
accordingly to achieve the following aim.  Two examples have been provided, 
as shown in the table. 

### System aim (purpose)
Previous aim: Use the sensors available on your B-L4S5I-IOT01A board
and MQTT to stream the following sensor data at roughly 1 Hz:

| sensor type | Sensor | uC Conn. | MQTT topic | Example? |
| ----------- | ------ | -------- | ---------- | -------- |
| temperature | HTS221 | `I2C2` | `board_test/temperature` | Yes |
| acceleration (x, y, z) | LSM6DSL | `I2C2` | `board_test/accelerometer` | Yes |
| relative humidity | HTS221 | `I2C2` | `board_test/humidity` | No |
| angular velocity (x, y, z) | LSM6DSL | `I2C2` | `board_test/gyroscope` | No |
| magnetic direction | LIS3MDL | `I2C2` | `board_test/magnetometer` | No |
| air pressure | LPS22HB | `I2C2` | `board_test/air_pressure` | No |

**New Aim:** The image sensor data should be streamed at 0.5 Hz (every two seconds):
| sensor type | Sensor | uC Conn. | MQTT topic | Example? |
| ----------- | ------ | -------- | ---------- | -------- |
| image | ArduCam 2MP Mini Plus | `SPI1/I2C1` | `board_test/camera` | No |

For the messages sent to this topic, they should be of the following format:
`img_id: <image id: uint8_t>, chunk_id: <chunk id: size_t>, num_chunks: <chunk count: size_t>, chunk_size: <chunk number of bytes: size_t>, data: <chunk bytes: uint8_t[chunk_size]> `
You will be sending your image data in chunks so that it is easier to handle.  When you implement the Python server
next week, you will be able to concatenate your image data into a single file.  Note that JPEG data starts with the
header `0xff 0xd8` and ends with the footer `0xff 0xd9`, which you can check exists inside the framebuffer.

### Tasks
1. Begin by modifying the `sensor_telemetry_setup.h` `#define`s for your own networking
   setup.  If you need to find your own IP address, run `ipconfig /all` in PowerShell
   or the Command Prompt.  Note that you may need to check this every time you turn 
   your computer on/off unless you have a reserved IP at your router end.
2. If you haven't already, install [MQTT Explorer](http://mqtt-explorer.com/) 
   so you can more easily view the sensor data that is streamed.
3. Run mosquitto on your computer, using the `mosquitto.conf` configuration that 
   allows for unauthenticated access.
4. I recommend first making sure that you can grab the camera data. As you may have noticed,
   the network setup and other sensor threads have been commented out.  Make sure you implement
   two threads: the camera setup thread (which is partially implemented), and the camera 
   streaming thread (which is not at all implemented).  
   
   Start with the camera setup thread,
   and add the setting of the camera setup event (defined as EVT_CAMERA_READY) in the global 
   event flags, so that the camera streaming thread knows to wait.
   
   Then, for the camera streaming thread, simply get the framebuffer from the ArduChip (there
   is a function inside `sensor_telemetry.c` that you'll find helpful).  Do not yet try to 
   chunk it out to send via MQTT.  You can put a breakpoint in your function on a call similar to
   `printf("framebuffer length: %d", num_bytes);` so that you can inspect the framebuffer and look
   for the JPEG header/footer.
5. Once the framebuffer is working, add the other threads back in, and get the timing correct. Make sure
   that the other sensors are still streaming properly (modify the global data struct as necessary) 
   before adding the chunking/MQTT logic to the camera streaming thread.  Since the image size can often
   be around 10 KiB, I recommend to use 1 KiB chunks (1024 bytes) to stream the image.  As there is a mutex
   on the `send_nx_mqtt_message()` function, you don't need to use a mutex here, similar to the other threads.
6. You may notice some lag in your timing now.  On the Teams page, please discuss what you think you could
   do to mitigate the lag with the MQTT messages.
