#pragma once

#include <io.h>
#include <types.h>

void keyboard_handle_keypress();

bool keyboard_alt();
bool keyboard_shift();
bool keyboard_ctrl();
void keyboard_data(u8 *buf, i64 max_size);