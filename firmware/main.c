// ELRS Sim Dongle firmware: CRSF from the onboard ELRS receiver in,
// USB HID joystick with channels 1-8 as 8 analog axes out.

#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "tusb.h"

#include "crsf.h"
#include "usb_descriptors.h"

#define RX_UART uart1
#define RX_UART_IRQ UART1_IRQ
#define RX_TX_PIN 4
#define RX_RX_PIN 5
#define RX_BAUD 420000

#define LED_PIN 25
#define LINK_TIMEOUT 500

// UART RX ring buffer, filled from the IRQ so USB work never drops bytes
static uint8_t rx_ring[1024];
static volatile uint16_t rx_head;
static volatile uint16_t rx_tail;

// UART1 RX interrupt: move every received byte into the ring buffer.
// Bytes are dropped if the buffer is full (main loop stalled > ~25 ms).
static void on_uart_rx(void) {
    while (uart_is_readable(RX_UART)) {
        uint8_t b = uart_getc(RX_UART);
        uint16_t next = (rx_head + 1) % sizeof(rx_ring);
        if (next != rx_tail) {
            rx_ring[rx_head] = b;
            __compiler_memory_barrier();  // data must land before head moves
            rx_head = next;
        }
    }
}

// Set up UART1 on GPIO4/5 at the ELRS CRSF baud rate, with RX interrupts.
static void rx_uart_init(void) {
    uart_init(RX_UART, RX_BAUD);
    gpio_set_function(RX_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(RX_RX_PIN, GPIO_FUNC_UART);
    gpio_pull_up(RX_RX_PIN);
    uart_set_fifo_enabled(RX_UART, true);
    irq_set_exclusive_handler(RX_UART_IRQ, on_uart_rx);
    irq_set_enabled(RX_UART_IRQ, true);
    uart_set_irq_enables(RX_UART, true, false);
}

// Map a raw CRSF channel (172..1811) to the joystick axis range
// (0..JOYSTICK_AXIS_MAX), clamping anything outside.
static uint16_t channel_to_axis(uint16_t v) {
    if (v <= CRSF_CHANNEL_MIN)
        return 0;
    if (v >= CRSF_CHANNEL_MAX)
        return JOYSTICK_AXIS_MAX;
    return (uint32_t)(v - CRSF_CHANNEL_MIN) * JOYSTICK_AXIS_MAX /
           (CRSF_CHANNEL_MAX - CRSF_CHANNEL_MIN);
}

// Send channels 1-8 as one HID report whenever the endpoint is free
// (i.e. on every 1 ms USB poll).
static void send_joystick_report(const crsf_state_t *crsf) {
    if (!tud_hid_ready())
        return;
    joystick_report_t report;
    for (int i = 0; i < JOYSTICK_NUM_AXES; i++)
        report.axes[i] = channel_to_axis(crsf->channels[i]);
    tud_hid_report(0, &report, sizeof(report));
}

// Main loop: USB, CRSF parsing, joystick report, and link LED
// (solid = link, blinking = no link).
int main(void) {
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    rx_uart_init();
    tud_init(0);

    static crsf_state_t crsf;
    crsf_init(&crsf);

    while (true) {
        tud_task();

        uint32_t now = to_ms_since_boot(get_absolute_time());

        while (rx_tail != rx_head) {
            __compiler_memory_barrier();  // read data only after seeing head move
            crsf_feed(&crsf, rx_ring[rx_tail], now);
            rx_tail = (rx_tail + 1) % sizeof(rx_ring);
        }

        // On link loss, axes hold their last values (ELRS stops sending frames)
        send_joystick_report(&crsf);

        bool link_up = crsf.last_channels_ms && now - crsf.last_channels_ms < LINK_TIMEOUT;
        gpio_put(LED_PIN, link_up ? 1 : (now / 500) & 1);
    }
}
