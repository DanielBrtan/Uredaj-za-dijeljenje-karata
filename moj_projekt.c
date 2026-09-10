#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_rom_sys.h"

static const char *TAG = "bela";

#define OLED_SDA 18
#define OLED_SCL 19
#define OLED_ADDR 0x3C

#define DF_UART UART_NUM_1
#define DF_TX_PIN 21
#define DF_RX_PIN 22

#define IA_PIN 25
#define IB_PIN 33

#define IN1 27
#define IN2 14
#define IN3 12
#define IN4 13

#define BUTTON_PIN 32

#define KORAKA_90  1024
#define KORAKA_270 3072
#define KORAK_DELAY_US 2000

static i2c_master_dev_handle_t oled_dev;

static const uint8_t font5x7[27][5] = {
    {0x00,0x00,0x00,0x00,0x00},
    {0x7E,0x11,0x11,0x11,0x7E},
    {0x7F,0x49,0x49,0x49,0x36},
    {0x3E,0x41,0x41,0x41,0x22},
    {0x7F,0x41,0x41,0x22,0x1C},
    {0x7F,0x49,0x49,0x49,0x41},
    {0x7F,0x09,0x09,0x09,0x01},
    {0x3E,0x41,0x49,0x49,0x7A},
    {0x7F,0x08,0x08,0x08,0x7F},
    {0x00,0x41,0x7F,0x41,0x00},
    {0x20,0x40,0x41,0x3F,0x01},
    {0x7F,0x08,0x14,0x22,0x41},
    {0x7F,0x40,0x40,0x40,0x40},
    {0x7F,0x02,0x0C,0x02,0x7F},
    {0x7F,0x04,0x08,0x10,0x7F},
    {0x3E,0x41,0x41,0x41,0x3E},
    {0x7F,0x09,0x09,0x09,0x06},
    {0x3E,0x41,0x51,0x21,0x5E},
    {0x7F,0x09,0x19,0x29,0x46},
    {0x46,0x49,0x49,0x49,0x31},
    {0x01,0x01,0x7F,0x01,0x01},
    {0x3F,0x40,0x40,0x40,0x3F},
    {0x1F,0x20,0x40,0x20,0x1F},
    {0x3F,0x40,0x38,0x40,0x3F},
    {0x63,0x14,0x08,0x14,0x63},
    {0x07,0x08,0x70,0x08,0x07},
    {0x61,0x51,0x49,0x45,0x43}
};

static const int step_sequence[8][4] = {
    {1,0,0,0},
    {1,1,0,0},
    {0,1,0,0},
    {0,1,1,0},
    {0,0,1,0},
    {0,0,1,1},
    {0,0,0,1},
    {1,0,0,1}
};

static void oled_cmd(uint8_t c) {
    uint8_t buf[2] = {0x00, c};
    i2c_master_transmit(oled_dev, buf, 2, 100);
}

static void oled_data(const uint8_t *d, size_t len) {
    uint8_t buf[64];
    buf[0] = 0x40;
    memcpy(&buf[1], d, len);
    i2c_master_transmit(oled_dev, buf, len + 1, 100);
}

static void oled_init(void) {
    oled_cmd(0xAE);
    oled_cmd(0x20); oled_cmd(0x00);
    oled_cmd(0xB0);
    oled_cmd(0xC8);
    oled_cmd(0x00); oled_cmd(0x10);
    oled_cmd(0x40);
    oled_cmd(0x81); oled_cmd(0x7F);
    oled_cmd(0xA1);
    oled_cmd(0xA6);
    oled_cmd(0xA8); oled_cmd(0x3F);
    oled_cmd(0xA4);
    oled_cmd(0xD3); oled_cmd(0x00);
    oled_cmd(0xD5); oled_cmd(0x80);
    oled_cmd(0xD9); oled_cmd(0xF1);
    oled_cmd(0xDA); oled_cmd(0x12);
    oled_cmd(0xDB); oled_cmd(0x40);
    oled_cmd(0x8D); oled_cmd(0x14);
    oled_cmd(0xAF);
}

static void oled_clear(void) {
    uint8_t zeros[32] = {0};
    for (int page = 0; page < 8; page++) {
        oled_cmd(0xB0 + page);
        oled_cmd(0x00);
        oled_cmd(0x10);
        for (int i = 0; i < 4; i++) {
            oled_data(zeros, 32);
        }
    }
}

static void oled_text(uint8_t page, uint8_t col, const char *text) {
    oled_cmd(0xB0 + page);
    oled_cmd(0x00 | (col & 0x0F));
    oled_cmd(0x10 | (col >> 4));

    for (int i = 0; text[i] != '\0'; i++) {
        char ch = text[i];
        int idx = 0;
        if (ch >= 'A' && ch <= 'Z') idx = ch - 'A' + 1;
        else if (ch >= 'a' && ch <= 'z') idx = ch - 'a' + 1;
        else idx = 0;

        uint8_t glyph[6];
        memcpy(glyph, font5x7[idx], 5);
        glyph[5] = 0x00;
        oled_data(glyph, 6);
    }
}

static void df_send_cmd(uint8_t cmd, uint16_t param) {
    uint8_t buf[10];
    buf[0] = 0x7E;
    buf[1] = 0xFF;
    buf[2] = 0x06;
    buf[3] = cmd;
    buf[4] = 0x00;
    buf[5] = (param >> 8) & 0xFF;
    buf[6] = param & 0xFF;

    uint16_t checksum = 0 - (0xFF + 0x06 + cmd + buf[4] + buf[5] + buf[6]);
    buf[7] = (checksum >> 8) & 0xFF;
    buf[8] = checksum & 0xFF;
    buf[9] = 0xEF;

    uart_write_bytes(DF_UART, (const char*)buf, 10);
}

static void df_play_from_folder(uint8_t folder, uint8_t track) {
    df_send_cmd(0x0F, (folder << 8) | track);
}

static void stepper_off(void) {
    gpio_set_level(IN1, 0);
    gpio_set_level(IN2, 0);
    gpio_set_level(IN3, 0);
    gpio_set_level(IN4, 0);
}

static void stepper_okreni(int broj_koraka, int smjer) {
    static int poz = 0;

    for (int k = 0; k < broj_koraka; k++) {
        poz += smjer;
        if (poz < 0) poz += 8;
        if (poz > 7) poz -= 8;

        gpio_set_level(IN1, step_sequence[poz][0]);
        gpio_set_level(IN2, step_sequence[poz][1]);
        gpio_set_level(IN3, step_sequence[poz][2]);
        gpio_set_level(IN4, step_sequence[poz][3]);

        esp_rom_delay_us(KORAK_DELAY_US);

        if ((k % 100) == 99) {
            vTaskDelay(1);
        }
    }

    stepper_off();
}

static void izbaci_kartu(void) {
    gpio_set_level(IA_PIN, 1);
    gpio_set_level(IB_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(1500));

    gpio_set_level(IA_PIN, 0);
    gpio_set_level(IB_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(200));

    gpio_set_level(IA_PIN, 0);
    gpio_set_level(IB_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(1000));

    gpio_set_level(IA_PIN, 0);
    gpio_set_level(IB_PIN, 0);
}

static void podijeli_igracu(int broj_karata) {
    for (int c = 0; c < broj_karata; c++) {
        izbaci_kartu();
    }
}

static void runda(int karata_po_igracu) {
    for (int i = 0; i < 4; i++) {
        podijeli_igracu(karata_po_igracu);

        if (i < 3) {
            stepper_okreni(KORAKA_90, 1);
            vTaskDelay(pdMS_TO_TICKS(300));
        }
    }

    stepper_okreni(KORAKA_270, -1);
    vTaskDelay(pdMS_TO_TICKS(300));
}

static void cekaj_tipku(void) {
    while (gpio_get_level(BUTTON_PIN) == 1) {
        vTaskDelay(pdMS_TO_TICKS(20));
    }
    vTaskDelay(pdMS_TO_TICKS(50));

    while (gpio_get_level(BUTTON_PIN) == 0) {
        vTaskDelay(pdMS_TO_TICKS(20));
    }
    vTaskDelay(pdMS_TO_TICKS(50));
}

void app_main(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL<<IA_PIN)|(1ULL<<IB_PIN)
                      | (1ULL<<IN1)|(1ULL<<IN2)|(1ULL<<IN3)|(1ULL<<IN4),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    gpio_set_level(IA_PIN, 0);
    gpio_set_level(IB_PIN, 0);
    stepper_off();

    gpio_config_t btn_conf = {
        .pin_bit_mask = (1ULL << BUTTON_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&btn_conf);

    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = OLED_SDA,
        .scl_io_num = OLED_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    i2c_master_bus_handle_t bus;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &bus));

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = OLED_ADDR,
        .scl_speed_hz = 400000,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus, &dev_cfg, &oled_dev));

    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    uart_param_config(DF_UART, &uart_config);
    uart_set_pin(DF_UART, DF_TX_PIN, DF_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(DF_UART, 256, 0, 0, NULL, 0);

    vTaskDelay(pdMS_TO_TICKS(100));
    oled_init();
    oled_clear();

    vTaskDelay(pdMS_TO_TICKS(2500));
    df_send_cmd(0x06, 20);
    vTaskDelay(pdMS_TO_TICKS(500));

    while (1) {
        oled_clear();
        oled_text(3, 20, "PRITISNI");
        oled_text(4, 26, "TIPKU");

        cekaj_tipku();

        oled_clear();
        oled_text(3, 10, "DIJELJENJE");
        oled_text(4, 16, "ZAPOCINJE");
        df_play_from_folder(1, 1);

        vTaskDelay(pdMS_TO_TICKS(5000));
        oled_clear();

        runda(3);
        runda(3);
        runda(2);

        oled_clear();
        oled_text(3, 10, "DIJELJENJE JE");
        oled_text(4, 20, "ZAVRSENO");
        df_play_from_folder(1, 2);

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}