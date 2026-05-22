#pragma once

void serial_initialize();
bool serial_is_initialized();
bool serial_has_data();
char serial_read_char();
void serial_write_char(char c);
void serial_write(const char* str);