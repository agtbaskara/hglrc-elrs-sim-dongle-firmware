// USB serial <-> UART1 (GPIO4/5) bridge to the onboard ESP8285 receiver,
// for esptool.

#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "hardware/uart.h"
#include "tusb.h"

#define BRIDGE_UART uart1
#define BRIDGE_TX_PIN 4
#define BRIDGE_RX_PIN 5
#define DEFAULT_BAUD 115200
#define LED_PIN 25

static volatile bool reboot_to_bootsel;

// Host changed serial settings: follow its baud rate (esptool switches speed),
// or reboot to BOOTSEL if it asks for 1200 baud.
void tud_cdc_line_coding_cb(uint8_t itf, cdc_line_coding_t const *coding) {
    (void)itf;
    if (coding->bit_rate == 1200)
        reboot_to_bootsel = true;
    else if (coding->bit_rate)
        uart_set_baudrate(BRIDGE_UART, coding->bit_rate);
}

// Main loop: copy USB -> UART and UART -> USB, and flash the LED on traffic.
int main(void) {
    uart_init(BRIDGE_UART, DEFAULT_BAUD);
    gpio_set_function(BRIDGE_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(BRIDGE_RX_PIN, GPIO_FUNC_UART);
    gpio_pull_up(BRIDGE_RX_PIN);
    uart_set_fifo_enabled(BRIDGE_UART, true);

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    tud_init(0);

    uint8_t buf[64];
    absolute_time_t led_off_at = nil_time;

    while (true) {
        tud_task();

        if (reboot_to_bootsel) {
            sleep_ms(50);
            reset_usb_boot(0, 0);
        }

        bool activity = false;

        // USB -> UART
        if (tud_cdc_available()) {
            uint32_t n = tud_cdc_read(buf, sizeof(buf));
            uart_write_blocking(BRIDGE_UART, buf, n);
            activity = n > 0;
        }

        // UART -> USB
        uint32_t n = 0;
        while (n < sizeof(buf) && uart_is_readable(BRIDGE_UART) &&
               n < tud_cdc_write_available())
            buf[n++] = uart_getc(BRIDGE_UART);
        if (n) {
            tud_cdc_write(buf, n);
            tud_cdc_write_flush();
            activity = true;
        }

        if (activity) {
            gpio_put(LED_PIN, 1);
            led_off_at = make_timeout_time_ms(30);
        } else if (!is_nil_time(led_off_at) && time_reached(led_off_at)) {
            gpio_put(LED_PIN, 0);
            led_off_at = nil_time;
        }
    }
}
