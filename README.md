# Nexis Kernel

Nexis is a small freestanding x86 kernel that boots with GRUB and provides a simple interactive shell.

## Features

- Boots through GRUB
- VGA text output
- Keyboard input
- Serial output for terminal-based runs
- In-memory shell commands
- Simple virtual filesystem for testing shell behavior

## Shell Commands

- `help` - Show the full command list.
- `name [newname]` - Show or change the system name.
- `echo <text>` - Print text to the screen.
- `ls [path]` - List the contents of a directory.
- `pwd` - Print the current directory.
- `cd <path>` - Change the current directory.
- `mkdir <path>` - Create a new directory.
- `touch <path>` - Create an empty file.
- `write <file> <text>` - Replace a file's contents.
- `cat <file>` - Print a file's contents.
- `rm <file>` - Delete a file.
- `rmdir <dir>` - Delete an empty directory.
- `uname` - Print the system name.
- `whoami` - Print the current user.
- `clear` - Clear the screen.

## Examples

```text
help
name nexis
echo hello
ls
pwd
mkdir notes
touch notes/todo.txt
write notes/todo.txt finish kernel shell
cat notes/todo.txt
```

## Requirements

You need these tools in WSL Ubuntu or another Linux environment:

- `nasm`
- `g++`
- `ld`
- `make`
- `grub-mkrescue`
- `xorriso`
- `mtools`

If you are on Ubuntu WSL, install them with:

```text
sudo apt-get update
sudo apt-get install -y build-essential nasm xorriso grub-pc-bin mtools
```

## Build

From Windows PowerShell:

```powershell
wsl bash -lc "cd /mnt/d/nexis/MyKernel && make"
```

To build only the ISO:

```powershell
wsl bash -lc "cd /mnt/d/nexis/MyKernel && make iso"
```

## Run

The default run target uses terminal mode, so it works even when GUI display forwarding is not available.

From Windows PowerShell:

```powershell
wsl bash -lc "cd /mnt/d/nexis/MyKernel && make run"
```

This runs QEMU with serial console output in the terminal.

If you want to run the emulator only without rebuilding first:

```powershell
wsl bash -lc "cd /mnt/d/nexis/MyKernel && make run-tty"
```

## What To Expect

When the kernel starts, you should see:

```text
Nexis v0.1
Type 'help' for commands
> 
```

Type commands directly after the prompt. For example:

```text
name
name core
uname
pwd
ls
```

## Project Layout

- `boot/` - Boot assembly entry point
- `kernel/` - Kernel source code
- `grub.cfg` - GRUB boot menu configuration
- `linker.ld` - Linker script
- `Makefile` - Build and run commands


## Cleanup

To remove build artifacts:

```powershell
wsl bash -lc "cd /mnt/d/nexis/MyKernel && make clean"
```

If you want to delete them manually instead:

```powershell
wsl bash -lc "cd /mnt/d/nexis/MyKernel && rm -f boot.o io.o kernel.o serial.o terminal.o kernel.bin mykernel.iso"
```

## Notes

- The shell keeps its filesystem in memory while the kernel is running.
- `make run` and `make run-tty` are equivalent in this version.
- The terminal cursor is visible while typing.
