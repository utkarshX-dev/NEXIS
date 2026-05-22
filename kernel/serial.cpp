#include "serial.hpp"
#include "io.hpp"

namespace
{
    const unsigned short COM1 = 0x3F8;
    bool g_serial_ready = false;

    bool can_transmit()
    {
        return (inb(COM1 + 5) & 0x20) != 0;
    }
}

void serial_initialize()
{
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x80);
    outb(COM1 + 0, 0x03);
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x03);
    outb(COM1 + 2, 0xC7);
    outb(COM1 + 4, 0x0B);

    g_serial_ready = true;
}

bool serial_is_initialized()
{
    return g_serial_ready;
}

bool serial_has_data()
{
    if (!g_serial_ready)
    {
        return false;
    }

    return (inb(COM1 + 5) & 0x01) != 0;
}

char serial_read_char()
{
    while (!serial_has_data())
    {
    }

    return static_cast<char>(inb(COM1));
}

void serial_write_char(char c)
{
    if (!g_serial_ready)
    {
        return;
    }

    while (!can_transmit())
    {
    }

    outb(COM1, static_cast<unsigned char>(c));
}

void serial_write(const char* str)
{
    int i = 0;
    while (str[i])
    {
        if (str[i] == '\n')
        {
            serial_write_char('\r');
        }
        serial_write_char(str[i]);
        i++;
    }
}