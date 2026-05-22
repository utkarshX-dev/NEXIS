#include "terminal.hpp"
#include "serial.hpp"

static const int VGA_WIDTH = 80;
static const int VGA_HEIGHT = 25;
static int row = 0;
static int col = 0;
static unsigned short* buffer = (unsigned short*)0xB8000;

static void terminal_draw_cursor()
{
    buffer[row * VGA_WIDTH + col] = 0x0F7C;
}

void terminal_initialize()
{
    row = 0;
    col = 0;
    for (int y = 0; y < VGA_HEIGHT; y++)
    {
        for (int x = 0; x < VGA_WIDTH; x++)
        {
            buffer[y * VGA_WIDTH + x] = 0x0720;
        }
    }

    terminal_draw_cursor();
}

void terminal_putchar(char c)
{
    if (c == '\b')
    {
        terminal_backspace();
        return;
    }

    if (c == '\n')
    {
        col = 0;
        row++;
        if (row >= VGA_HEIGHT) row = 0;
        terminal_draw_cursor();
        serial_write("\n");
        return;
    }

    buffer[row * VGA_WIDTH + col] = (0x07 << 8) | c;
    serial_write_char(c);
    col++;
    if (col >= VGA_WIDTH)
    {
        col = 0;
        row++;
        if (row >= VGA_HEIGHT) row = 0;
    }

    terminal_draw_cursor();
}

void terminal_write(const char* str)
{
    int i = 0;
    while (str[i])
    {
        terminal_putchar(str[i]);
        i++;
    }
}

void terminal_backspace()
{
    if (col > 0)
    {
        col--;
    }
    else if (row > 0)
    {
        row--;
        col = VGA_WIDTH - 1;
    }
    else
    {
        return;
    }

    buffer[row * VGA_WIDTH + col] = 0x0720;
    terminal_draw_cursor();
    serial_write("\b \b");
}
