/*
This is the c configuration file for the keymap

Copyright 2012 Jun Wako <wakojun@gmail.com>
Copyright 2015 Jack Humbert

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

/* Select hand configuration */

#define MASTER_LEFT
// #define MASTER_RIGHT
// #define EE_HANDS

// ESC key
#define BOOTMAGIC_LITE_ROW  0
#define BOOTMAGIC_LITE_COLUMN   0

// upper right key, '`'
#define BOOTMAGIC_LITE_ROW_RIGHT  5
#define BOOTMAGIC_LITE_COLUMN_RIGHT   0

#define QUICK_TAP_TERM 0
#define TAPPING_TERM 100

#define SOFT_SERIAL_PIN D2

// use the slave half OLED as a status monitor
#define SPLIT_LAYER_STATE_ENABLE
#define SPLIT_LED_STATE_ENABLE
#define SPLIT_MODS_ENABLE
#define SPLIT_WPM_ENABLE

// use my own logo and progress bars dedicated glyphs for gometrics
#undef OLED_FONT_H
#define OLED_FONT_H "glcdfont_niflheims.c"

// timeout in ms after which the logo gets displayed instead of dead metrics
#define GOMETRICS_TIMEOUT_MS 10000
