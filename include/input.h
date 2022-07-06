#ifndef _edit_input_h
#define _edit_input_h
/*
 * CLI special key code definitions
 */

enum ControlKeys {
    KEY_NULL = 0,       /* NULL */        
    CTRL_A, CTRL_B, CTRL_C, CTRL_D,
    CTRL_E, CTRL_F, CTRL_G, BACKSPACE,
    TAB,    CTRL_J, CTRL_K, CTRL_L,
    ENTER,  CTRL_N, CTRL_O, CTRL_P,
    CTRL_Q, CTRL_R, CTRL_S, CTRL_T,
    CTRL_U, CTRL_V, CTRL_W, CTRL_X,
    CTRL_Y, CTRL_Z, ESC
};


#define CLI_FLAG_CTRL   0x0100  /* Flag indicating CTRL is pressed */
#define CLI_FLAG_SHIFT  0x0200  /* Flag indicating SHIFT is pressed */
#define CLI_FLAG_ALT    0x0400  /* Flag indicating ALT is pressed */
#define CLI_FLAG_OS     0x0800  /* Flag indicating OS key is pressed */
#define CLI_FLAG_FUNC   0x4000  /* Function keys: 0x4001 - 0x400C */
#define CLI_FLAG_SPEC   0x8000  /* Special keys: */
#define CLI_KEY_LEFT    0x8001
#define CLI_KEY_RIGHT   0x8002
#define CLI_KEY_UP      0x8003
#define CLI_KEY_DOWN    0x8004
#define CLI_KEY_DEL     0x8005
#define CLI_KEY_MONITOR 0x8010  /* A2560K Monitor key */
#define CLI_KEY_CTX     0x8011  /* A2560K CTX Switch key */
#define CLI_KEY_HELP    0x8012  /* A2560K Menu/Help key */

/**
 * Get a character from the console, processing recognized escape sequences
 *
 * @param channel the number of the input channel
 * @return the 16-bit functional character code
 */
short cli_getchar(short channel);

/**
 * Make sure all the console settings are setup so that the console works correctly
 */
void cli_ensure_console(short channel);


#endif