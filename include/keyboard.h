#pragma once

#include <types.h>

void keyboard_handle_keypress();

bool keyboard_alt();
bool keyboard_shift();
bool keyboard_ctrl();
void keyboard_data(u8 *buf, u64 max_size);