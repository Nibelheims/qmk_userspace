#include <string.h>
#include QMK_KEYBOARD_H
#include <stdio.h>
#include <stdint.h>

enum layer_number {
  _QWERTY = 0,
  _LOWER,
  _RAISE,
  _ADJUST,
};

enum custom_keycodes {
    MY_NAV_REFRESH = SAFE_RANGE
};

typedef struct {
  char name[4+1/*final null byte*/];
  uint8_t percent;
} measure_t;

typedef struct {
  measure_t  measures[4]; // maximum nb of measures
  size_t     nb_measures;
} measures_t;

static bool prev_was_measures = false;
static volatile measures_t measures;
static volatile bool metrics_received = false;
static deferred_token metrics_timeout_token = INVALID_DEFERRED_TOKEN;

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {

/* QWERTY
 * ,-----------------------------------------.                    ,-----------------------------------------.
 * | ESC  |   1  |   2  |   3  |   4  |   5  |                    |   6  |   7  |   8  |   9  |   0  |  `   |
 * |------+------+------+------+------+------|                    |------+------+------+------+------+------|
 * | Tab  |   Q  |   W  |   E  |   R  |   T  |                    |   Y  |   U  |   I  |   O  |   P  |  -   |
 * |------+------+------+------+------+------|                    |------+------+------+------+------+------|
 * |LCTRL |   A  |   S  |   D  |   F  |   G  |-------.    ,-------|   H  |   J  |   K  |   L  |   ;  |  '   |
 * |------+------+------+------+------+------|   [   |    |    ]  |------+------+------+------+------+------|
 * |LShift|   Z  |   X  |   C  |   V  |   B  |-------|    |-------|   N  |   M  |   ,  |   .  |   /  |RShift|
 * `-----------------------------------------/       /     \      \-----------------------------------------'
 *                   | LAlt | LGUI |LOWER | /Space  /       \Enter \  |RAISE |BackSP| RAlt |
 *                   |      |      |      |/       /         \      \ |      |      |      |
 *                   `----------------------------'           '------''--------------------'
 */

 [_QWERTY] = LAYOUT(
  KC_ESC,   KC_1,   KC_2,    KC_3,    KC_4,    KC_5,                     KC_6,    KC_7,    KC_8,    KC_9,    KC_0,    KC_GRV,
  KC_TAB,   KC_Q,   KC_W,    KC_E,    KC_R,    KC_T,                     KC_Y,    KC_U,    KC_I,    KC_O,    KC_P,    KC_MINS,
  KC_LCTL,  KC_A,   KC_S,    KC_D,    KC_F,    KC_G,                     KC_H,    KC_J,    KC_K,    KC_L,    KC_SCLN, KC_QUOT,
  KC_LSFT,  KC_Z,   KC_X,    KC_C,    KC_V,    KC_B, KC_LBRC,  KC_RBRC,  KC_N,    KC_M,    KC_COMM, KC_DOT,  KC_SLSH,  KC_RSFT,
                        KC_LALT, KC_LGUI, MO(_LOWER), KC_SPC, KC_ENT, MO(_RAISE), KC_BSPC, KC_RALT
),
/* LOWER
 * ,-----------------------------------------.                    ,-----------------------------------------.
 * | CALC |      | RApp |CtrlF5|      |      |                    |      | PgUp | PgDn | Ins  | @    |      |
 * |------+------+------+------+------+------|                    |------+------+------+------+------+------|
 * |  F1  |  F2  |  F3  |  F4  |  F5  |  F6  |                    |  F7  |  F8  |  F9  | F10  | F11  | F12  |
 * |------+------+------+------+------+------|                    |------+------+------+------+------+------|
 * |   `  |   !  |   @  |   #  |   $  |   %  |-------.    ,-------|   ^  |   &  |   *  |   (  |   )  |   -  |
 * |------+------+------+------+------+------|   [   |    |    ]  |------+------+------+------+------+------|
 * | LSFT | CAPS | <(fr)| >(fr)|      |      |-------|    |-------|  |   |   _  |   +  |   {  |   }  | RSFT |
 * `-----------------------------------------/       /     \      \-----------------------------------------'
 *                   | LAlt | LGUI |LOWER | /Space  /       \Enter \  |RAISE | DEL  | RGUI |
 *                   |      |      |      |/       /         \      \ |      |      |      |
 *                   `----------------------------'           '------''--------------------'
 */
[_LOWER] = LAYOUT(
  KC_CALC, _______, KC_APP,  MY_NAV_REFRESH, _______, _______,             _______, KC_PGUP, KC_PGDN, KC_INS, KC_AT,    _______,
  KC_F1,   KC_F2,   KC_F3,   KC_F4,   KC_F5,   KC_F6,                      KC_F7,   KC_F8,   KC_F9,   KC_F10,  KC_F11,  KC_F12,
  KC_GRV,  KC_EXLM, KC_AT,   KC_HASH, KC_DLR,  KC_PERC,                    KC_CIRC, KC_AMPR, KC_ASTR, KC_LPRN, KC_RPRN, KC_TILD,
  KC_LSFT, KC_CAPS, KC_BSLS ,KC_NUBS, S(KC_NUBS),_______, _______, _______,  KC_PIPE, KC_UNDS, KC_PLUS, KC_LCBR, KC_RCBR, KC_RSFT,
                             _______, _______, _______, _______, _______,  _______, KC_DEL, KC_RGUI
),
/* RAISE
 * ,-----------------------------------------.                    ,-----------------------------------------.
 * |      | MUTE | VOLD | VOLU |      |      |                    |      |      | BRID | BRIU |      |      |
 * |------+------+------+------+------+------|                    |------+------+------+------+------+------|
 * |   `  |   1  |   2  |   3  |   4  |   5  |                    |   6  |   7  |   8  |   9  |   0  |      |
 * |------+------+------+------+------+------|                    |------+------+------+------+------+------|
 * |  F1  |  F2  |  F3  |  F4  |  F5  |  F6  |-------.    ,-------| HOME | Left | Down |  Up  |Right | END  |
 * |------+------+------+------+------+------|   F7  |    |    ]  |------+------+------+------+------+------|
 * | LSFT |  F8  |  F9  | F10  | F11  | F12  |-------|    |-------|   +  |   -  |   =  |   [  |   ]  | RSFT |
 * `-----------------------------------------/       /     \      \-----------------------------------------'
 *                   | ESC  | LGUI |LOWER | /Space  /       \Enter \  |RAISE |BackSP| RGUI |
 *                   |      |      |      |/       /         \      \ |      |      |      |
 *                   `----------------------------'           '------''--------------------'
 */

[_RAISE] = LAYOUT(
  _______, KC_MUTE, KC_VOLD, KC_VOLU, _______, _______,                     _______, _______, KC_BRID, KC_BRIU, _______, _______,
  KC_GRV,  KC_1,    KC_2,    KC_3,    KC_4,    KC_5,                        KC_6,    KC_7,    KC_8,    KC_9,    KC_0,    _______,
  KC_F1,   KC_F2,   KC_F3,   KC_F4,   KC_F5,   KC_F6,                       KC_HOME, KC_LEFT, KC_DOWN, KC_UP,   KC_RGHT, KC_END,
  KC_LSFT, KC_F8,   KC_F9,   KC_F10,  KC_F11,  KC_F12,     KC_F7, _______,  KC_PLUS, KC_MINS, KC_EQL,  KC_LBRC, KC_RBRC, KC_RSFT,
                             KC_ESC,  _______, _______,  _______, _______,  _______, _______, KC_RGUI
),
/* ADJUST
 * ,-----------------------------------------.                    ,-----------------------------------------.
 * |      |      |      |      |      |      |                    |      |      |      |      |      |      |
 * |------+------+------+------+------+------|                    |------+------+------+------+------+------|
 * |      |      |      |      |      |      |                    |      |      |      |      |      |      |
 * |------+------+------+------+------+------|                    |------+------+------+------+------+------|
 * |      |   1  |   2  |   3  |   4  |   5  |                    |   6  |   7  |   8  |   9  |   0  |      |
 * |------+------+------+------+------+------|       |    |       |------+------+------+------+------+------|
 * |      |      |      |      |      |      |-------|    |-------|      |      |      |      |      |      |
 * `-----------------------------------------/       /     \      \-----------------------------------------'
 *                   | LAlt | LGUI |LOWER | /Space  /       \Enter \  |RAISE |BackSP| RGUI |
 *                   |      |      |      |/       /         \      \ |      |      |      |
 *                   `----------------------------'           '------''--------------------'
 */
  [_ADJUST] = LAYOUT(
  XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,                   XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,
  XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,                   XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,
  _______,  KC_1,    KC_2,    KC_3,    KC_4,    KC_5,                     KC_6,    KC_7,    KC_8,    KC_9,    KC_0,    _______,
  XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,
                             _______, _______, _______, _______, _______,  _______, _______, _______
  )
};

#define L_BASE 0
#define L_LOWER (1 << 1)
#define L_RAISE (1 << 2)
#define L_ADJUST (1 << 3)
#define L_ADJUST_TRI (L_ADJUST | L_RAISE | L_LOWER)

layer_state_t layer_state_set_user(layer_state_t state) {
  return update_tri_layer_state(state, _LOWER, _RAISE, _ADJUST);
}

//SSD1306 OLED update loop, make sure to enable OLED_ENABLE=yes in rules.mk
#ifdef OLED_ENABLE

oled_rotation_t oled_init_user(oled_rotation_t rotation) {
  memset((void*)&measures, 0, sizeof(measures));
  if (is_keyboard_master())
    return OLED_ROTATION_0;
  else {
    return OLED_ROTATION_270;
  }
}

// When you add source files to SRC in rules.mk, you can use functions.
// const char *read_layer_state(void);
const char *read_logo(void);
// void set_keylog(uint16_t keycode, keyrecord_t *record);
// const char *read_keylog(void);
// const char *read_keylogs(void);
// const char *read_mode_icon(bool swap);
// const char *read_host_led_state(void);
// void set_timelog(void);
// const char *read_timelog(void);

const char* read_mod_state(void) {
    static char mod_str[5] = {'\0'};
    uint8_t mods = get_mods();

    mod_str[0] = (mods & MOD_MASK_CTRL)  ? 'C' : ' ';
    mod_str[1] = (mods & MOD_MASK_SHIFT) ? 'S' : ' ';
    mod_str[2] = (mods & MOD_MASK_ALT)   ? 'A' : ' ';
    mod_str[3] = (mods & MOD_MASK_GUI)   ? 'G' : ' ';

    return mod_str;
}

bool oled_task_user(void) {
    uint8_t n = get_current_wpm();
    if (!is_keyboard_master()) {
        oled_set_cursor(0,1);
        char wpm_counter[4];
        wpm_counter[3] = '\0';
        wpm_counter[2] = '0' + n % 10;
        wpm_counter[1] = (n /= 10) % 10 ? '0' + (n) % 10 : (n / 10) % 10 ? '0' : ' ';
        wpm_counter[0] = n / 10 ? '0' + n / 10 : ' ';
        oled_write("WPM: ", false);
        oled_write_ln(wpm_counter, false);
        oled_set_cursor(0,4);
        oled_write_ln("RAISE", layer_state == L_RAISE);
        oled_write_ln("BASE ", layer_state == L_BASE );
        oled_write_ln("LOWER", layer_state == L_LOWER);
        oled_write_ln(read_mod_state(),true);
        oled_write_ln("CAPS", host_keyboard_led_state().caps_lock);
    } else {
        char name[4+1];
        char percent[4+1];
        if (measures.nb_measures == 0) {
            if (prev_was_measures) oled_clear();
            oled_write(read_logo(), false);
            prev_was_measures = false;
        } else {
            if (!prev_was_measures) oled_clear();
            for (int i = 0; i < measures.nb_measures; i++) {
                snprintf(name, sizeof(name), "%.4s", measures.measures[i].name);
                snprintf(percent, sizeof(percent), "%3d%%", (int) measures.measures[i].percent);
                oled_write(name, false);
                oled_write_ln(percent, false);
            }
            prev_was_measures = true;
        }
    }
    return false;
}
#endif // OLED_ENABLE

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case MY_NAV_REFRESH:
            if (record->event.pressed) {
                SEND_STRING(SS_DOWN(X_LCTL) SS_TAP(X_F5) SS_UP(X_LCTL));
            }
            break;
    }

    return true;
}

// stop rendering metrics when data is not received anymore
uint32_t metrics_timeout(uint32_t trigger_time, void *cb_arg) {
    bool metrics_rcvd = *((bool*)cb_arg);
    if (!metrics_rcvd) {
        // timeout! let's display the logo again
        measures.nb_measures = 0;
    }
    // ms delay, 0 if not to be called again
    return metrics_rcvd ? GOMETRICS_TIMEOUT_MS : 0;
}

bool raw_hid_receive_user(uint8_t *data, uint8_t length) {
    // gometrics
    if (is_keyboard_master()) {
        const uint8_t MAGIC[] = {0x67, 0x6F, 0x6D, 0x65, 0x74, 0x72, 0x69, 0x63, 0x73};
        if (!memcmp(MAGIC, data, sizeof(MAGIC))) {
            // the report was for us
            // skip the header and read the number of items
            data += sizeof(MAGIC);
            size_t nb_items = (size_t)*data;
            nb_items = (nb_items > 4) ? 4 : nb_items;

            measures.nb_measures = nb_items;
            data++;

            if (nb_items > 0) {
                memset((void*)&measures, 0, sizeof(measures)); // all names are null
                for (int i = 0; i < nb_items; i++, data+=5) {
                    strncpy((char *)measures.measures[i].name, (const char*)data, 4);
                    measures.measures[i].percent = (uint8_t)data[4];
                }
                metrics_received = true;
                if (metrics_timeout_token != INVALID_DEFERRED_TOKEN) {
                    extend_deferred_exec(metrics_timeout_token, GOMETRICS_TIMEOUT_MS);
                } else {
                    metrics_timeout_token = defer_exec(GOMETRICS_TIMEOUT_MS, metrics_timeout, (void*)&metrics_received);
                }
            } else {
                metrics_received = false;
            }

            return false;
        } else {
            // something else, eg VIA
            metrics_received = false;
            return true;
        }
    }
    else {
        return true;
    }
}

