#include <ctype.h>
#include "mcp/syscalls.h"
#include "input.h"

#define CON_IOCTRL_ANSI_ON  0x01            /* IOCTRL Command: turn on ANSI terminal codes */
#define CON_IOCTRL_ANSI_OFF 0x02            /* IOCTRL Command: turn off ANSI terminal codes */
#define CON_IOCTRL_ECHO_ON  0x03            /* IOCTRL Command: turn on echo of input characters */
#define CON_IOCTRL_ECHO_OFF 0x04            /* IOCTRL Command: turn off echo of input characters */
#define CON_IOCTRL_BREAK    0x05            /* IOCTRL Command: return the status of the keyboard BREAK */
#define CON_IOCTRL_CURS_ON  0x06            /* IOCTRL Command: show the cursor */
#define CON_IOCTRL_CURS_OFF 0x07            /* IOCTRL Command: hide the cursor */
/*
 * States to interpret ANSI escape codes
 */
typedef enum {
    CLI_ES_BASE = 0,    // Base state
    CLI_ES_ESC,         // "ESC" seen
    CLI_ES_CSI,         // "ESC[" has been seen
    CLI_ES_SEMI         // Semicolon has been seen
} cli_state;

/**
 * Make sure all the console settings are setup so that the console works correctly
 */
void cli_ensure_console(short channel) {
    // Make sure the console is set up correctly for the CLI
    sys_chan_ioctrl(channel, CON_IOCTRL_ECHO_OFF, 0, 0);
    sys_chan_ioctrl(channel, CON_IOCTRL_ANSI_ON, 0, 0);
    sys_chan_ioctrl(channel, CON_IOCTRL_CURS_ON, 0, 0);

    // Make sure the screen has text enabled
    sys_txt_set_mode(sys_chan_device(channel), TXT_MODE_TEXT);
}

/**
 * Decode ANSI modifier codes
 *
 * @param modifiers the ANSI modifier codes from an escape sequence
 * @return CLI modifier flags
 */
short cli_translate_modifiers(short modifiers) {
    char buffer[10];
    short flags = 0;

    if (modifiers > 0) {
        modifiers--;
    }

    if (modifiers & 0x01) flags |= CLI_FLAG_SHIFT;
    if (modifiers & 0x02) flags |= CLI_FLAG_ALT;
    if (modifiers & 0x04) flags |= CLI_FLAG_CTRL;
    if (modifiers & 0x08) flags |= CLI_FLAG_OS;

    return flags;
}

/**
 * Translate escape sequences that end in a letter code
 *
 * @param modifiers optional parameter
 * @code the letter code of the key
 */
short cli_translate_alpha(short modifiers, char code) {
    short key_code = 0;

    key_code = cli_translate_modifiers(modifiers);

    switch (code) {
        case 'A':
            key_code |= CLI_KEY_UP;
            break;

        case 'B':
            key_code |= CLI_KEY_DOWN;
            break;

        case 'C':
            key_code |= CLI_KEY_RIGHT;
            break;

        case 'D':
            key_code |= CLI_KEY_LEFT;
            break;

        default:
            return 0;
    }

    return key_code;
}

/**
 * Translate escape sequences that end in a numeric code
 *
 * @param modifiers optional parameter
 * @code the numeric code of the key
 */
short cli_translate_numeric(short modifiers, short code) {
    short key_code = 0;

    key_code = cli_translate_modifiers(modifiers);

    if ((code >= 11) && (code <= 15)) {
        // Function keys 1 - 5
        key_code |= CLI_FLAG_FUNC | (code - 10);

    } else if ((code >= 17) && (code <= 21)) {
        // Function keys 6 - 10
        key_code |= CLI_FLAG_FUNC | (code - 11);

    } else if (code == 30) {
        // MONITOR key
        key_code |= CLI_KEY_MONITOR;

    } else if (code == 31) {
        // CTX SWITCH key
        key_code |= CLI_KEY_CTX;

    } else if (code == 32) {
        // MENU HELP key
        key_code |= CLI_KEY_HELP;

    } else if (code == 3) {
        // DELETE key
        key_code |= CLI_KEY_DEL;

    } else {
        // Unknown escape code
        key_code = 0;
    }

    return key_code;
}

/**
 * Get a character from the console, processing recognized escape sequences
 *
 * @param channel the number of the input channel
 * @return the 16-bit functional character code
 */
short cli_getchar(short channel) {
    char buffer[10];
    cli_state state = CLI_ES_BASE;      // Current state of the escape sequence
    short number1 = 0, number2 = 0;     // Two numbers that can be embedded in the sequence
    char c;                             // The current character read from the console

    do {
        c = sys_chan_read_b(channel);
        switch (state) {
            case CLI_ES_BASE:
                // We are not processing a sequence...
                if (c == CHAR_ESC) {
                    // Escape has been seen
                    state = CLI_ES_ESC;

                } else {
                    // It's an ordinary character, so return it
                    return c;
                }
                break;

            case CLI_ES_ESC:
                // Escape has been seen... check for CSI
                if (c == '[') {
                    // ESC [ has been seen...
                    state = CLI_ES_CSI;

                } else {
                    // Bad escape sequence...just return the character
                    state = CLI_ES_BASE;
                    return c;
                }
                break;

            case CLI_ES_CSI:
                // ESC [ has been seen... next is either a number, a letter, a semi-colon, or a tilda
                if (isdigit(c)) {
                    // It's a number... shift it onto number1
                    number1 = number1 * 10 + (c - '0');

                } else if (isalpha(c)) {
                    // It's a letter... treat as a code
                    return cli_translate_alpha(number1, c);

                } else if (c == ';') {
                    // Got a semicolon, go to that state
                    state = CLI_ES_SEMI;

                } else if (c == '~') {
                    // Got a tilda... end of numeric code with no parameters
                    return cli_translate_numeric(0, number1);

                } else {
                    // Bad sequence... just return the current character
                    state = CLI_ES_BASE;
                    return c;
                }
                break;

            case CLI_ES_SEMI:
                // Semicolon has been seen... next is either a number or a tilda
                if (isdigit(c)) {
                    // It's a number... shift it onto number1
                    number2 = number2 * 10 + (c - '0');

                } else if (c == '~') {
                    // Got a tilda... end of numeric code with parameters
                    return cli_translate_numeric(number2, number1);

                } else {
                    // Bad sequence... just return the current character
                    state = CLI_ES_BASE;
                    return c;
                }
                break;

            default:
                state = CLI_ES_BASE;
                return c;
        }
    } while (1);
}