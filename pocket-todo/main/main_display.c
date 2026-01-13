/*
Created by Nicholas West
1/12/2026
*/

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/i2c_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"  // for SSD1306 driver
#include "esp_err.h"
#include "driver/uart.h"
#include "driver/gpio.h"


static const char *TAG = "ssd1306_example";

/************** Configuration **************/
#define I2C_BUS_PORT        0
#define EXAMPLE_PIN_NUM_SDA 3
#define EXAMPLE_PIN_NUM_SCL 4
#define EXAMPLE_PIN_NUM_RST -1      // Set to -1 if no reset pin wired
#define EXAMPLE_I2C_HW_ADDR 0x3C
#define EXAMPLE_I2C_CLOCK_HZ (400 * 1000)
#define LCD_H_RES 128
#define LCD_V_RES 64

// Monochrome framebuffer (1 bit per pixel)
static uint8_t oled_buffer[LCD_H_RES * LCD_V_RES / 8];

/************** Helper functions *************/

// Set a single pixel (0 = on, 1 = off for SSD1306)
static void set_pixel(int x, int y, bool on)
{
    if (x < 0 || x >= LCD_H_RES || y < 0 || y >= LCD_V_RES){
        ESP_LOGI(TAG, "Incorrect pixel set");
    }

    uint16_t byte_index = x + (y / 8) * LCD_H_RES;
    uint8_t bit_mask = 1 << (y % 8);

    if (on)
        oled_buffer[byte_index] &= ~bit_mask;
    else
        oled_buffer[byte_index] |= bit_mask;
}

// Draw a horizontal line
static void x_out(int y)
{
    for (int x = 10; x < 115; x++)
        set_pixel(x, y, true);
}

static const uint8_t font5x7[][5] = {
    // kinda have to tilt your head to the left and squint at it to see the letters, lol
    { // A
        0b0111111,
        0b1001000,
        0b1001000,
        0b1001000,
        0b0111111
    },

    { // B
        0b00110110,
        0b01001001,
        0b01001001,
        0b01001001,
        0b01111111
    },

    { // C
        0b01000001,
        0b01000001,
        0b01000001,
        0b01000001,
        0b00111110
    },

    { // D
        0b00011100,
        0b00100010,
        0b01000001,
        0b01000001,
        0b01111111
    },

    { // E
        0b01000001,
        0b01001001,
        0b01001001,
        0b01001001,
        0b01111111
    },

    { // F
        0b01000000,
        0b01001000,
        0b01001000,
        0b01001000,
        0b01111111
    },

    { // G
        0b00110110,
        0b00101001,
        0b01001001,
        0b01000001,
        0b00111110
    },

    { // H
        0b01111111,
        0b00001000,
        0b00001000,
        0b00001000,
        0b01111111
    },

    { // I
        0b00000000,
        0b01000001,
        0b01111111,
        0b01000001,
        0b00000000
    },

    { // J
        0b01000000,
        0b01111111,
        0b01000001,
        0b01000001,
        0b00000010
    },

    { // K
        0b01000001,
        0b00100010,
        0b00010100,
        0b00001000,
        0b01111111
    },

    { // L
        0b00000001,
        0b00000001,
        0b00000001,
        0b00000001,
        0b01111111
    },

    { // M
        0b01111111,
        0b00010000,
        0b00001000,
        0b00010000,
        0b01111111
    },

    { // N
        0b01111111,
        0b00001000,
        0b00010000,
        0b0010000,
        0b01111111
    },

    { // O
        0b00111110,
        0b01000001,
        0b01000001,
        0b01000001,
        0b00111110
    },

    { // P
        0b00110000,
        0b01001000,
        0b01001000,
        0b01001000,
        0b01111111
    },

    { // Q
        0b00111101,
        0b01000101,
        0b01000001,
        0b01000001,
        0b00111110
    },

    { // R
        0b00110011,
        0b01001100,
        0b01001000,
        0b01001000,
        0b01111111
    },

    { // S
        0b0100110,
        0b1001001,
        0b1001001,
        0b1001001,
        0b0110001
    },

    { // T
        0b1000000,
        0b1000000,
        0b1111110,
        0b1000000,
        0b1000000
    },

    { // U
        0b1111110,
        0b0000001,
        0b0000001,
        0b0000001,
        0b1111110
    },

    { // V
        0b00011111,
        0b00100000,
        0b01000000,
        0b00100000,
        0b00011111
    },

    { // W
        0b01111111,
        0b00010000,
        0b00100000,
        0b00010000,
        0b01111111
    },

    { // X
        0b11000110,
        0b00101000,
        0b00010000,
        0b00101000,
        0b11000110
    },

    { // Y
        0b11000000,
        0b10010000,
        0b10000111,
        0b10010000,
        0b11000000
    },

    { // Z
        0b11000011,
        0b10100101,
        0b10010010,
        0b10100101,
        0b11000001
    },

    { // [, or a box in our code
        0b1111111,
        0b1000001,
        0b1000001,
        0b1000001,
        0b1111111
    }
};

void draw_char(int x, int y, char c)
{
    const uint8_t *bitmap = font5x7[c - 'A'];
    for (int col = 0; col < 5; col++) {
        for (int row = 0; row < 7; row++) {
            bool pixel_on = bitmap[col] & (1 << row);
            set_pixel(x + col, y + row, pixel_on);
        }
    }
}
void draw_text(int x, int y, const char *text)
{
    int len = strlen(text);
    int cursor_x = x;
    for (int i = 0; i < len; i++) {
        if (text[i] != ' '){
            draw_char(cursor_x, y, text[i]);
            cursor_x -= 6; // move left
        }
        else {
            cursor_x -= 2;
        }
    }
}

// Clear framebuffer
static void clear_buffer(void){
    memset(oled_buffer, 0xFF, sizeof(oled_buffer)); // all pixels OFF
}

// Flush framebuffer to SSD1306
static void flush_buffer(esp_lcd_panel_handle_t panel){
    esp_lcd_panel_draw_bitmap(panel, 0, 0, LCD_H_RES, LCD_V_RES, oled_buffer);
}

/************** Main program ***************/

void app_main(void) {
    ESP_LOGI(TAG, "Initialize I2C bus");
    i2c_master_bus_handle_t i2c_bus = NULL;
    i2c_master_bus_config_t bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .i2c_port = I2C_BUS_PORT,
        .sda_io_num = EXAMPLE_PIN_NUM_SDA,
        .scl_io_num = EXAMPLE_PIN_NUM_SCL,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &i2c_bus));

    ESP_LOGI(TAG, "Install panel IO");
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t io_config = {
        .dev_addr = EXAMPLE_I2C_HW_ADDR,
        .scl_speed_hz = EXAMPLE_I2C_CLOCK_HZ,
        .control_phase_bytes = 1,
        .dc_bit_offset = 6,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(i2c_bus, &io_config, &io_handle));

    ESP_LOGI(TAG, "Install SSD1306 panel driver");
    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .bits_per_pixel = 1,
        .reset_gpio_num = EXAMPLE_PIN_NUM_RST,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_ssd1306(io_handle, &panel_config, &panel_handle));

    ESP_LOGI(TAG, "Reset and initialize panel");
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
    clear_buffer();
    flush_buffer(panel_handle);

    ESP_LOGI(TAG, "initialize text");
    draw_text(120, 35,"[ TAKE OUT THE TRASH");
    draw_text(90, 20,"KAMUSTA RUTHIE");
    draw_text(80, 5,"STU VWX YZ");
    flush_buffer(panel_handle);





    gpio_set_direction(17, GPIO_MODE_INPUT);
    gpio_input_enable(17);
    gpio_set_pull_mode(17, GPIO_PULLUP_ONLY);


    // superloop
    while (1) {
        if (gpio_get_level(17) == 0){
            x_out(37);
            flush_buffer(panel_handle);
        }
        vTaskDelay(pdMS_TO_TICKS(350));
    }
}
