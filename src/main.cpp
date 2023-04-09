/*###########################################################################
        copyright qqqlab.com / github.com/qqqlab

        This program is free software: you can redistribute it and/or modify
        it under the terms of the GNU General Public License as published by
        the Free Software Foundation, either version 3 of the License, or
        (at your option) any later version.

        This program is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
        GNU General Public License for more details.

        You should have received a copy of the GNU General Public License
        along with this program.  If not, see <http://www.gnu.org/licenses/>.
###########################################################################*/
#include "qqqDALI.h"

#include "esp_log.h"
#include "esp_system.h"
#include "esp_attr.h"
#include "esp_timer.h"

#include "freertos/FreeRTOS.h"

#include "driver/gpio.h"
#include "driver/timer.h"
#include "driver/uart.h"

#include "esp_task_wdt.h"

#include <cstdio>

static const char* TAG = "main";

Dali dali;

// Define the GPIO pins used for DALI bus communication
#define DALI_RX_PIN GPIO_NUM_4
#define DALI_TX_PIN GPIO_NUM_16

#define TIMER_UPDATES_PER_SECOND 9600

// is bus asserted
uint8_t IRAM_ATTR bus_is_high()
{
    return gpio_get_level(DALI_RX_PIN);
}

// assert bus
void IRAM_ATTR bus_set_low()
{
    gpio_set_level(DALI_TX_PIN, 1);
}

// release bus
void IRAM_ATTR bus_set_high()
{
    gpio_set_level(DALI_TX_PIN, 0);
}

void IRAM_ATTR dali_timer_callback(void* arg)
{
    dali.timer();
}

void bus_init()
{
    // setup rx pin
    gpio_set_direction(DALI_RX_PIN, GPIO_MODE_INPUT);

    // setup tx pin
    gpio_set_direction(DALI_TX_PIN, GPIO_MODE_OUTPUT);

    const esp_timer_create_args_t my_timer_args = {
        .callback = &dali_timer_callback, 
        .arg = nullptr,
        .dispatch_method = esp_timer_dispatch_t::ESP_TIMER_TASK, 
        .name = "dali_timer_callback", 
        .skip_unhandled_events = false 
    };

    esp_timer_handle_t timer_handler;
    ESP_ERROR_CHECK(esp_timer_create(&my_timer_args, &timer_handler));
    ESP_ERROR_CHECK(esp_timer_start_periodic(timer_handler, 1000000 / TIMER_UPDATES_PER_SECOND));
}

// init_arg=11111111 : all without short address
// init_arg=00000000 : all
// init_arg=0AAAAAA1 : only for this shortadr
// returns number of new short addresses assigned
uint8_t debug_commission(uint8_t init_arg)
{
    uint8_t cnt = 0;
    uint8_t arr[64];
    uint8_t sa;
    for (sa = 0; sa < 64; sa++)
        arr[sa] = 0;

    dali.cmd(DALI_INITIALISE, init_arg);
    dali.cmd(DALI_RANDOMISE, 0x00);
    // need 100ms pause after RANDOMISE, scan takes care of this...

    // find used short addresses (run always, seems to work better than without...)
    printf("Find existing short adr\n");
    for (sa = 0; sa < 64; sa++) {
        esp_task_wdt_reset();

        int16_t rv = dali.cmd(DALI_QUERY_STATUS, sa);
        if (rv >= 0) {
            if (init_arg != 0b00000000)
                arr[sa] = 1; // remove address from list if not in "all" mode
            printf("sortadr=%d status=0x%X minLevel=%d\n", sa, rv, dali.cmd(DALI_QUERY_MIN_LEVEL, sa));
        }
    }

    //  dali.set_searchaddr(0x000000);
    //  dali.set_searchaddr(0xFFFFFF);
    // while(1) {
    //  dali.compare();
    //  delay(200);
    //}

    printf("Find random adr\n");
    while (1) {
        uint32_t adr = dali.find_addr();
        if (adr > 0xffffff)
            break;
        printf("found=%X\n", adr);

        // find available address
        for (sa = 0; sa < 64; sa++) {
            if (arr[sa] == 0)
                break;
        }
        if (sa >= 64)
            break;
        arr[sa] = 1;
        cnt++;

        printf("program short adr=%d\n", sa);
        dali.program_short_address(sa);

        printf("read short adr=%d\n", dali.query_short_address());
        dali.cmd(DALI_WITHDRAW, 0x00);
        
        vTaskDelay(1);
    }

    dali.cmd(DALI_TERMINATE, 0x00);
    return cnt;
}

uint8_t dali_read_memory_bank_verbose(uint8_t bank, uint8_t adr)
{
    uint16_t rv;

    if (dali.set_dtr0(0, adr))
        return 1;
    if (dali.set_dtr1(bank, adr))
        return 2;

    // uint8_t data[255];
    uint16_t len = dali.cmd(DALI_READ_MEMORY_LOCATION, adr);
    printf("memlen=%d\n", len);
    for (uint8_t i = 0; i < len; i++) {
        int16_t mem = dali.cmd(DALI_READ_MEMORY_LOCATION, adr);
        if (mem >= 0) {
            // data[i] = mem;
            printf("%x:%d 0x%x ", i, mem, mem);
            if (mem >= 32 && mem < 127)
                printf("%c", (char)mem);
            printf("\n");
        } else if (mem != -DALI_RESULT_NO_REPLY) {
            printf("%x:err=%d\n", i, mem);
        }
    }

    uint16_t dtr0 = dali.cmd(DALI_QUERY_CONTENT_DTR0, adr); // get DTR value
    if (dtr0 != 255)
        return 4;

    return 0;
}


void menu_read_memory()
{
    /*
      uint8_t v = 123;
      uint8_t adr = 0xff;

      while(1) {
        int16_t rv = dali.cmd(DALI_DATA_TRANSFER_REGISTER0, v); //store value in DTR
        Serial.print("rv=");
        Serial.print(rv);

        int16_t dtr = dali.cmd(DALI_QUERY_CONTENT_DTR0, adr); //get DTR value
        Serial.print(" dtr=");
        Serial.println(dtr);
        delay(13);
      }
    */

    printf("Running: Scan all short addresses\n");
    uint8_t sa;
    uint8_t cnt = 0;
    for (sa = 0; sa < 64; sa++) {
        int16_t rv = dali.cmd(DALI_QUERY_STATUS, sa);
        if (rv >= 0) {
            cnt++;
            printf("\nshort address %d\n", sa);
            printf("status=0x%X\n", rv);
            printf("minLevel=%d\n", dali.cmd(DALI_QUERY_MIN_LEVEL, sa));

            dali_read_memory_bank_verbose(0, sa);

        } else if (-rv != DALI_RESULT_NO_REPLY) {
            printf("short address=%d ERROR=%d\n", sa, -rv);
        }
    }
    printf("DONE, found %d short addresses\n", cnt);
}

void menu()
{
    printf("----------------------------\n");
    printf("1 Blink all lamps\n");
    printf("2 Scan short addresses\n");
    printf("3 Commission short addresses\n");
    printf("4 Commission short addresses (VERBOSE)\n");
    printf("5 Delete short addresses\n");
    printf("6 Read memory bank\n");
    printf("----------------------------\n");
}

void menu_blink()
{
    printf("Running: Blinking all lamps\n");

    for (uint8_t i = 0; i < 4; i++) {
        dali.set_level(254);
        printf(".");
        vTaskDelay(pdMS_TO_TICKS(500));
        dali.set_level(0);
        printf(".");
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    printf("Running: Blinking lamps one by one\n");

    for (uint8_t i = 0; i < 4; i++) {
        dali.set_level(254, i);
        printf(".");
        vTaskDelay(pdMS_TO_TICKS(500));
        dali.set_level(0);
        printf(".");
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    printf("\n");
}

void menu_scan_short_addr()
{
    printf("Running: Scan all short addresses\n");
    uint8_t sa;
    uint8_t cnt = 0;
    for (sa = 0; sa < 64; sa++) {
        int16_t rv = dali.cmd(DALI_QUERY_STATUS, sa);
        if (rv >= 0) {
            cnt++;
            printf("shortAddress=%d status=0x%X minLevel=%d   flashing", sa, rv, dali.cmd(DALI_QUERY_MIN_LEVEL, sa));
            for (uint8_t i = 0; i < 5; i++) {
                dali.set_level(254, sa);
                printf(".");
                vTaskDelay(pdMS_TO_TICKS(500));
                dali.set_level(0, sa);
                printf(".");
                vTaskDelay(pdMS_TO_TICKS(500));
            }
            printf("\n");
        } else if (-rv != DALI_RESULT_NO_REPLY) {
            printf("short address=%d ERROR=%d\n", sa, -rv);
        }
    }
    printf("DONE, found %d short addresses\n", cnt);
}

// might need a couple of calls to find everything...
void menu_commission()
{
    printf("Running: Commission\n");
    printf("Might need a couple of runs to find all lamps ...\n");
    printf("Be patient, this takes a while ...\n");
    uint8_t cnt = dali.commission(0xff); // init_arg=0b11111111 : all without short address
    printf("DONE, assigned %d new short addresses\n", cnt);
}

// might need a couple of calls to find everything...
void menu_commission_debug()
{
    printf("Running: Commission (VERBOSE)\n");
    printf("Might need a couple of runs to find all lamps ...\n");
    printf("Be patient, this takes a while ...\n");
    uint8_t cnt = debug_commission(0xff); // init_arg=0b11111111 : all without short address
    printf("DONE, assigned %d new short addresses\n", cnt);
}

void menu_delete_short_addr()
{
    printf("Running: Delete all short addresses\n");
    // remove all short addresses
    dali.cmd(DALI_DATA_TRANSFER_REGISTER0, 0xFF);
    dali.cmd(DALI_SET_SHORT_ADDRESS, 0xFF);
    printf("DONE delete\n");
}


extern "C" void app_main()
{
    uint8_t data;
    size_t size;

    // Configure UART parameters
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    // Install UART driver
    uart_driver_install(UART_NUM_0, 1024, 0, 0, NULL, 0);

    // Configure UART parameters
    uart_param_config(UART_NUM_0, &uart_config);

    // Set UART pins (using default pins)
    uart_set_pin(UART_NUM_0, 1, 3, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    vTaskDelay(1);

    dali.begin(bus_is_high, bus_set_low, bus_set_high);
    bus_init();

    printf("\nDALI Commissioning Demo\n");

    menu();

    for (;;) {

        if (uart_get_buffered_data_len(UART_NUM_0, &size) != ESP_OK) {
            ESP_LOGE(TAG, "uart_get_buffered_data_len failed");
            return;
        }

        while (size > 0) {
            uart_read_bytes(UART_NUM_0, &data, 1, 0);
            switch (data) {
            case '1':
                menu_blink();
                menu();
                break;
            case '2':
                menu_scan_short_addr();
                menu();
                break;
            case '3':
                menu_commission();
                menu();
                break;
            case '4':
                menu_commission_debug();
                menu();
                break;
            case '5':
                menu_delete_short_addr();
                menu();
                break;
            case '6':
                menu_read_memory();
                menu();
                break;
            }
            size--;
        }

        vTaskDelay(1);
    }
}
