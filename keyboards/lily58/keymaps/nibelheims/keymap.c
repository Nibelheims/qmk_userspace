#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include QMK_KEYBOARD_H
#include <stdio.h>
#include <stdint.h>

#define HIDRAW_NAME_LEN     4
#define HIDRAW_VALUE_LEN    1
#define HIDRAW_DATA_LEN     (HIDRAW_NAME_LEN + HIDRAW_VALUE_LEN)
#define MAX_NB_MEASURES     4

#ifdef OLED_ENABLE
// the bar is like: xxxx-[==========]xxx%
#define BAR_NB_PX   60 // the screen is 128px wide, each glyph is 6 wide, use 6 before the bar and 5 after, 10 remaining
#endif

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
  char name[HIDRAW_NAME_LEN+1/*final null byte*/];
  uint8_t percent;
} measure_t;

#ifdef OLED_ENABLE
// bar animation stuff, so that transitions between values are smooth
const uint16_t ANIM_DURATION = 300; //ms for a full bar travel
typedef struct {
    size_t   current_step;
    size_t   nb_steps;
    uint16_t ms_period; // per pixel
    uint16_t timer;
    uint8_t  px;
    int8_t   increase; // direction, +1 or -1
} animation_t;
#endif

typedef struct {
  measure_t   measures[MAX_NB_MEASURES];
  size_t      nb_measures;
#ifdef OLED_ENABLE
  animation_t animations[MAX_NB_MEASURES];
#endif
} measures_t;

static bool prev_was_measures = false;
static volatile measures_t measures;
static volatile bool metrics_received = false;
// make me volatile to use the horrible hack in metrics_timeout()
static /*volatile*/ deferred_token metrics_timeout_token = INVALID_DEFERRED_TOKEN;

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

void keyboard_pre_init_user(void) {
  memset((void*)&measures, 0, sizeof(measures_t));
  prev_was_measures = false;
  metrics_received = false;
  metrics_timeout_token = INVALID_DEFERRED_TOKEN;
}

//SSD1306 OLED update loop, make sure to enable OLED_ENABLE=yes in rules.mk
#ifdef OLED_ENABLE

static inline int PERCENT_TO_PX(int p) {return (p * BAR_NB_PX)/100;}
static inline int PX_TO_PERCENT(int p) {return (p * 100)/BAR_NB_PX;}

static inline void init_animation(size_t anim_idx, uint8_t current_percent_value, uint8_t new_percent_value) {
    volatile animation_t *p_animation = &measures.animations[anim_idx];

     // if an animation is still on-going, keep the currently drawn value
    if (p_animation->nb_steps == 0) {
        // no on-going animation
        p_animation->px = (uint8_t)PERCENT_TO_PX((int)current_percent_value);
    } else {
        // on-going animation, use the current bar level to compute the number of steps
        current_percent_value = (uint8_t)PX_TO_PERCENT((int)(p_animation->px));
        // keep p_animation->px
    }
    p_animation->increase = (new_percent_value > current_percent_value) ? 1 : -1;
    p_animation->nb_steps = (size_t)abs(PERCENT_TO_PX((int)current_percent_value) - PERCENT_TO_PX((int)new_percent_value));
    if (p_animation->nb_steps > 0) {
        p_animation->ms_period = ANIM_DURATION / p_animation->nb_steps; // > 1
    }
    p_animation->current_step = 0;
    p_animation->timer = timer_read();

    return;
}

static inline void draw_bar(char *name, char *percent, uint8_t px) {
    const int GLYPH_WIDTH = 6;
    const int GLYPH_BAR_OFFSET = 3;
    char bar[4+ 2+10+1 + 4 + 1]; // starting decoration, bar itself, ending decoration, final null byte

    int nb_full   = px / GLYPH_WIDTH;
    int remaining = px % GLYPH_WIDTH;
    int i = 0;

    // bar label
    bar[0] = '\0';
    strncat(bar, name, 4);

    // bar init decoration
    memcpy(&bar[4], "\x01\x02", 2);

    // 10-glyph bar
    for (i = 0; i < nb_full; i++) {
        bar[6+i] = GLYPH_BAR_OFFSET + GLYPH_WIDTH;
    }
    bar[6+i] = GLYPH_BAR_OFFSET + remaining;
    for (i++; i < 10; i++) {
        bar[6+i] = GLYPH_BAR_OFFSET;
    }
    // bar ending decoration
    memcpy(&bar[6+10], "\x0B\x00", 2);

    // percentage
    strncat(bar, percent, 4);

    oled_write_ln(bar, false);
    return;
}

oled_rotation_t oled_init_user(oled_rotation_t rotation) {
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
        oled_write_ln("", false);
        oled_write_ln("CAPS", host_keyboard_led_state().caps_lock);
    } else {
        char name_str_buf[4+1];
        char percent_str_buf[4+1];
        if (measures.nb_measures == 0) {
            if (prev_was_measures) oled_clear();
            oled_write(read_logo(), false);
            prev_was_measures = false;
        } else {
            if (!prev_was_measures) {
                oled_clear();
            }
            for (int i = 0; i < measures.nb_measures; i++) {
                measure_t measure = measures.measures[i]; // avoid glitches due to its volatile nature
                animation_t anim  = measures.animations[i];
                int val = (int)(measure.percent);
                snprintf(name_str_buf,    sizeof(name_str_buf),    "%.4s",  measure.name);
                snprintf(percent_str_buf, sizeof(percent_str_buf), "%3d%%", val);

                if (anim.nb_steps > 0) {
                    // do not directly draw the bar according to the value, use transitions
                    if (anim.current_step < anim.nb_steps) {
                        if (timer_elapsed(anim.timer) >= anim.ms_period) {
                            anim.px += anim.increase;
                            measures.animations[i].px = anim.px;
                            measures.animations[i].current_step++;
                            measures.animations[i].timer = timer_read();
                        }
                        draw_bar(name_str_buf, percent_str_buf, anim.px);
                    } else {
                        // end of animation, ensure a clean ending
                        draw_bar(name_str_buf, percent_str_buf, PERCENT_TO_PX(val));
                        measures.animations[i].nb_steps = 0; // disable this animation
                    }
                } else {
                    // no on-going transition, just display the value
                    draw_bar(name_str_buf, percent_str_buf, PERCENT_TO_PX(val));
                }
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
    // timeout!
    measures.nb_measures = 0;
    // could be useful but horrible, commented out
    // not to be called again, hence the hackish invalidation of the deferred token
    /*
    deferred_token *p_token = (deferred_token*)cb_arg;
    *p_token = INVALID_DEFERRED_TOKEN;
    */
    return 0;
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
            nb_items = (nb_items > MAX_NB_MEASURES) ? MAX_NB_MEASURES : nb_items;

            measures.nb_measures = nb_items;
            data++;

            if (nb_items > 0) {
                for (int i = 0; i < nb_items; i++, data += HIDRAW_DATA_LEN) {
                    uint8_t cur_value = measures.measures[i].percent;
                    uint8_t new_value = (uint8_t)data[HIDRAW_NAME_LEN];

                    strncpy((char *)measures.measures[i].name, (const char*)data, HIDRAW_NAME_LEN);
                    #ifdef OLED_ENABLE
                    init_animation(i, cur_value, new_value);
                    #endif
                    measures.measures[i].percent = new_value;
                }
                // timeout, to detect if metrics are no longer updated
                if (metrics_timeout_token != INVALID_DEFERRED_TOKEN) {
                    if (!extend_deferred_exec(metrics_timeout_token, GOMETRICS_TIMEOUT_MS)) {
                        // the timeout was triggered, the callback is not registered anymore
                        metrics_timeout_token = defer_exec(GOMETRICS_TIMEOUT_MS, metrics_timeout, NULL);
                    }
                } else {
                    metrics_timeout_token = defer_exec(GOMETRICS_TIMEOUT_MS, metrics_timeout, NULL);
                }
            }
            return false;
        } else {
            // something else, eg VIA
            return true;
        }
    }
    else {
        return true;
    }
}

/*
bool shutdown_user(bool jump_to_bootloader) {
    return true; // process keyboard level
}
*/
