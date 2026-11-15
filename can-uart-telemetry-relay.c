#include "HAL.h"
#include <stdbool.h>

// Creates a struct to hold the CAN message with a 32 bit id and 8 bytes of data
typedef struct{
    uint32_t id;
    uint8_t data[CAN_LEN];
}CanMessage;

// Creates a circular buffer to store the CAN messages
enum {BufferSize = 32};
static CanMessage message_buffer[BufferSize];
// Creates global variables for the read and write indexes along with the LED state
static volatile uint32_t write_index = 0;
static volatile uint32_t read_index = 0;
static bool led_red = true;

// Runs once on startup
void Init(){
    // Initializes the read and write indexes and sets the initial LED state to red
    write_index = 0;
    read_index = 0;
    HAL_SetLED(255, 0, 0);
    led_red = true;
}

// Runs repeatedly from the main loop: drains the buffer and relays each
// buffered CAN message over UART
void Iter(){
    while (read_index != write_index) {
        // Checks the state of the LED and changes it to green if it is red
        if (led_red) {
            HAL_SetLED(0, 255, 0); // Set LED to green
            led_red = false;
        }
        // Points the message pointer to the correct slot in the buffer
        CanMessage* msg = &message_buffer[read_index % BufferSize];
        // Creates a UART buffer to hold the header, ID, and data
        uint8_t uart_buffer[14];
        // Creates the header to the UART buffer
        uart_buffer[0] = 0xF5;
        uart_buffer[1] = 0xAE;
        /* Adds the ID to the UART buffer in big-endian order (most significant byte first)
           Works by shifting the bits, keeping the last 8 bits, then converting to a single byte */
        uart_buffer[2] = (uint8_t)((msg->id >> 24) & 0xFF);
        uart_buffer[3] = (uint8_t)((msg->id >> 16) & 0xFF);
        uart_buffer[4] = (uint8_t)((msg->id >> 8) & 0xFF);
        uart_buffer[5] = (uint8_t)(msg->id & 0xFF);
        // Copies the 8 data bytes into the UART buffer
        for (int i = 0; i < CAN_LEN; i++) {
            uart_buffer[6 + i] = (uint8_t)(msg->data[i]);
        } 
        // Sends the UART buffer
        HAL_UartSend(uart_buffer);
        // Increments the read index
        read_index++;
        // Sets the LED state to red if the buffer is empty
        if (read_index == write_index && !led_red) {
            HAL_SetLED(255, 0, 0);
            led_red = true;
        }
    }
}

// CAN receive interrupt handler, called once per received frame. It must stay
// short, and HAL_ReadCanMessage is the only HAL function that is safe to call here
void RxCan(){
    // Checks if the buffer is full
    // (the indexes are free-running, so their unsigned difference is the fill level)
    if ((uint32_t)(write_index - read_index) >= BufferSize) {
        // If the buffer is full, the program reads and discards the message
        uint8_t temp_data[CAN_LEN];
        uint32_t temp_id;
        HAL_ReadCanMessage(&temp_id, temp_data);
        return;
    }
    // Points the message pointer to the correct slot in the buffer
    CanMessage* msg = &message_buffer[write_index % BufferSize];

    // Reads the CAN message into the buffer and increments the write index
    HAL_ReadCanMessage(&msg->id, msg->data);
    write_index++;
}
