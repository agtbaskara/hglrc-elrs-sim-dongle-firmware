#include "cli.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pico/time.h"
#include "tusb.h"
#include "config.h"

#define LINE_MAX 320  // long enough to paste a full line from 'show'

static char line[LINE_MAX];
static int line_len;
static volatile bool banner_pending;
static absolute_time_t banner_at;

// Write formatted text (one short line) to the serial port. The output of one
// command always fits in the TX buffer.
static void out(const char *fmt, ...) {
    char buf[128];
    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    if (n > 0)
        tud_cdc_write(buf, n < (int)sizeof(buf) ? n : (int)sizeof(buf) - 1);
}

static int crsf_to_us(uint16_t v) {
    return 1500 + ((int)v - CRSF_CHANNEL_MID) * 5 / 8;
}

static void print_help(void) {
    tud_cdc_write_str(
        "Commands:\r\n"
        "  show                     show which channel controls each axis and button\r\n"
        "  status                   show link state and all channel values\r\n"
        "  <axis>=<channel>         set which channel controls an axis\r\n"
        "                           axes: x y z rx ry rz slider dial (or 1-8)\r\n"
        "                           e.g. z=ch4, dial=none\r\n"
        "  b<n>=<channel> [invert]  set which channel presses button n (1-16)\r\n"
        "                           pressed when the channel is high, or low with invert\r\n"
        "                           e.g. b1=ch9, b2=ch9 invert, b1=none\r\n"
        "  defaults                 axes on channels 1-8, no buttons\r\n"
        "  save                     keep the current mapping after unplugging\r\n"
        "  bootsel                  reboot into BOOTSEL for flashing\r\n"
        "Several settings can go on one line, so a line from 'show' can be pasted back.\r\n"
        "High means above 1625 us, low means below 1375 us.\r\n");
}

static const char *axis_names[CONFIG_NUM_AXES] = {"X", "Y", "Z", "Rx", "Ry", "Rz", "Slider", "Dial"};

static void print_mapping(void) {
    out("Axes:   ");
    for (int i = 0; i < CONFIG_NUM_AXES; i++) {
        if (config.axis_channel[i])
            out(" %s=ch%d", axis_names[i], config.axis_channel[i]);
        else
            out(" %s=none", axis_names[i]);
    }
    out("\r\nButtons:");
    bool any = false;
    for (int i = 0; i < CONFIG_NUM_BUTTONS; i++) {
        uint8_t b = config.button_channel[i];
        if (!b)
            continue;
        out(" B%d=ch%d%s", i + 1, b & ~CONFIG_BUTTON_INVERT, (b & CONFIG_BUTTON_INVERT) ? "(invert)" : "");
        any = true;
    }
    out(any ? "\r\n" : " none\r\n");
}

static void print_status(const crsf_state_t *crsf, bool link_up) {
    out("Link: %s\r\n", link_up ? "up" : "DOWN (radio off, unbound, or receiver unpowered)");
    out("Channels (us):");
    for (int i = 0; i < CRSF_NUM_CHANNELS; i++)
        out(" %d:%d", i + 1, crsf_to_us(crsf->channels[i]));
    out("\r\n");
}

// Parse a whole number from min to max. Returns -1 if invalid.
static int parse_number(const char *s, int min, int max) {
    char *end;
    long v = strtol(s, &end, 10);
    return (end == s || *end || v < min || v > max) ? -1 : (int)v;
}

// Parse a channel: "ch4", "4" or "none" (0). Returns -1 if invalid.
static int parse_channel(const char *s) {
    if (!strcmp(s, "none"))
        return 0;
    if (!strncmp(s, "ch", 2))
        s += 2;
    return parse_number(s, 1, CRSF_NUM_CHANNELS);
}

// Parse what to set: an axis (name or 1-8) returns 0-7, a button (b1-b16)
// returns CONFIG_NUM_AXES + 0-15. Returns -1 if invalid.
static int parse_target(const char *s) {
    for (int i = 0; i < CONFIG_NUM_AXES; i++) {
        const char *n = axis_names[i];
        int j = 0;
        while (n[j] && tolower((unsigned char)n[j]) == s[j])
            j++;
        if (!n[j] && !s[j])
            return i;
    }
    if (s[0] == 'b') {
        int n = parse_number(s + 1, 1, CONFIG_NUM_BUTTONS);
        return n < 0 ? -1 : CONFIG_NUM_AXES + n - 1;
    }
    int n = parse_number(s, 1, CONFIG_NUM_AXES);
    return n < 0 ? -1 : n - 1;
}

// Apply a line of <target>=<channel> settings, e.g. "z=ch4 b2=ch9 invert", or
// a line pasted from 'show'. Nothing changes unless every setting is valid.
static void assign(const char *line) {
    // Tidy up: "z = 4" -> "z=4", "ch9(invert)" -> "ch9 invert"
    char tidy[LINE_MAX];
    int n = 0;
    for (const char *p = line; *p && n < LINE_MAX - 1; p++) {
        char c = (*p == '(' || *p == ')' || *p == '\t') ? ' ' : *p;
        if (c == ' ') {
            const char *q = p;
            while (*q == ' ' || *q == '\t')
                q++;
            if (*q == '=' || (n > 0 && tidy[n - 1] == '='))
                continue;
        }
        tidy[n++] = c;
    }
    tidy[n] = '\0';

    char *tokens[CONFIG_NUM_AXES + 2 * CONFIG_NUM_BUTTONS + 4];
    int count = 0;
    for (char *t = strtok(tidy, " "); t; t = strtok(NULL, " ")) {
        if (count == (int)(sizeof(tokens) / sizeof(tokens[0]))) {
            out("Too many settings on one line.\r\n");
            return;
        }
        tokens[count++] = t;
    }

    config_t updated = config;
    for (int i = 0; i < count; i++) {
        char *t = tokens[i];
        // A "Buttons:" line from 'show' lists every button in use, so the
        // ones it doesn't list (or "none") are cleared
        if (!strcmp(t, "buttons:")) {
            memset(updated.button_channel, 0, sizeof(updated.button_channel));
            continue;
        }
        if (!strcmp(t, "axes:") || !strcmp(t, "none"))
            continue;
        char *eq = strchr(t, '=');
        int target = -1, ch = -1;
        if (eq) {
            *eq = '\0';
            target = parse_target(t);
            ch = parse_channel(eq + 1);
            *eq = '=';
        }
        if (target < 0 || ch < 0) {
            out("Can't understand '%.40s'. Type 'help'.\r\n", t);
            return;
        }
        bool invert = i + 1 < count && !strcmp(tokens[i + 1], "invert");
        if (invert)
            i++;
        if (target < CONFIG_NUM_AXES) {
            if (invert) {
                out("'invert' only works for buttons.\r\n");
                return;
            }
            updated.axis_channel[target] = ch;
        } else {
            updated.button_channel[target - CONFIG_NUM_AXES] = ch ? (ch | (invert ? CONFIG_BUTTON_INVERT : 0)) : 0;
        }
    }
    config = updated;
    print_mapping();
    out("Not saved yet, type 'save' to keep it.\r\n");
}

// Run one command line. Returns true for "bootsel".
static bool run(char *cmd, const crsf_state_t *crsf, bool link_up) {
    for (char *p = cmd; *p; p++)
        *p = tolower((unsigned char)*p);
    while (*cmd == ' ' || *cmd == '\t')
        cmd++;
    // Settings, or a line pasted from 'show' (which may be "Buttons: none")
    if (strchr(cmd, '=') || !strncmp(cmd, "axes:", 5) || !strncmp(cmd, "buttons:", 8)) {
        assign(cmd);
        return false;
    }

    char *word = strtok(cmd, " \t");
    if (!word)
        return false;
    if (strtok(NULL, " \t")) {
        out("'%.40s' takes no arguments. Type 'help'.\r\n", word);
        return false;
    }

    if (!strcmp(word, "help") || !strcmp(word, "?")) {
        print_help();
    } else if (!strcmp(word, "show")) {
        print_mapping();
    } else if (!strcmp(word, "status")) {
        print_status(crsf, link_up);
    } else if (!strcmp(word, "defaults")) {
        config_defaults();
        print_mapping();
        out("Not saved yet, type 'save' to keep it.\r\n");
    } else if (!strcmp(word, "save")) {
        config_save();
        out("Saved.\r\n");
    } else if (!strcmp(word, "bootsel")) {
        out("Rebooting into BOOTSEL...\r\n");
        return true;
    } else {
        out("Unknown command '%.40s'. Type 'help'.\r\n", word);
    }
    return false;
}

// TinyUSB callback on every DTR/RTS change. A terminal raises DTR when it
// opens the port. The banner waits 200 ms, because terminals flush their input
// while setting up the port and would throw away anything sent earlier.
void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts) {
    (void)itf;
    (void)rts;
    if (dtr) {
        banner_pending = true;
        banner_at = make_timeout_time_ms(200);
    }
}

bool cli_task(const crsf_state_t *crsf, bool link_up) {
    if (banner_pending && tud_cdc_connected() && time_reached(banner_at)) {
        banner_pending = false;
        line_len = 0;
        out("\r\nELRS Sim Dongle configuration. Type 'help' for commands.\r\n");
        print_mapping();
        out("> ");
        tud_cdc_write_flush();
    }

    bool reboot = false;
    while (tud_cdc_available()) {
        char c = tud_cdc_read_char();
        if (c == '\r' || c == '\n') {
            // CRLF terminals send two line ends; ignore the empty one
            if (line_len == 0 && c == '\n')
                continue;
            out("\r\n");
            line[line_len] = '\0';
            line_len = 0;
            reboot |= run(line, crsf, link_up);
            out("> ");
        } else if (c == '\b' || c == 0x7F) {
            if (line_len > 0) {
                line_len--;
                out("\b \b");
            }
        } else if (isprint((unsigned char)c) && line_len < LINE_MAX - 1) {
            line[line_len++] = c;
            tud_cdc_write_char(c);
        }
        tud_cdc_write_flush();
    }
    return reboot;
}
