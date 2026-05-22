#include "terminal.hpp"
#include "io.hpp"
#include "serial.hpp"

namespace
{
    const char* kPrompt = "> ";
    const int kInputMax = 128;
    const int kMaxNodes = 64;
    const int kMaxName = 16;
    const int kMaxContent = 96;
    const int kMaxPath = 64;

    char input_buffer[kInputMax];
    int input_len = 0;

    struct VirtualNode
    {
        bool used;
        bool is_directory;
        int parent;
        int first_child;
        int next_sibling;
        char name[kMaxName];
        char content[kMaxContent];
    };

    VirtualNode nodes[kMaxNodes];
    int current_directory = 0;
    char system_name[kMaxName] = "Nexis";

    char scancode_to_ascii(unsigned char scancode)
    {
        static const char keymap[128] = {
            0,  27, '1', '2', '3', '4', '5', '6', '7', '8',
            '9','0', '-', '=', '\b','\t','q', 'w', 'e', 'r',
            't','y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0,
            'a','s', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',
            '\'', '`', 0,  '\\','z', 'x', 'c', 'v', 'b', 'n',
            'm',',', '.', '/', 0,   '*', 0,   ' ', 0
        };

        if (scancode >= 128)
        {
            return 0;
        }

        return keymap[scancode];
    }

    bool is_space(char c)
    {
        return c == ' ' || c == '\t';
    }

    bool streq(const char* a, const char* b)
    {
        int index = 0;
        while (a[index] && b[index])
        {
            if (a[index] != b[index])
            {
                return false;
            }
            index++;
        }

        return a[index] == b[index];
    }

    const char* skip_spaces(const char* text)
    {
        while (*text && is_space(*text))
        {
            text++;
        }
        return text;
    }

    int string_length(const char* text)
    {
        int length = 0;
        while (text[length])
        {
            length++;
        }
        return length;
    }

    void copy_string(char* destination, int destination_size, const char* source)
    {
        int index = 0;
        while (source[index] && index < destination_size - 1)
        {
            destination[index] = source[index];
            index++;
        }
        destination[index] = 0;
    }

    void clear_string(char* destination, int destination_size)
    {
        for (int index = 0; index < destination_size; index++)
        {
            destination[index] = 0;
        }
    }

    const char* read_token(const char* input, char* token, int token_size)
    {
        input = skip_spaces(input);

        int index = 0;
        while (input[index] && !is_space(input[index]) && index < token_size - 1)
        {
            token[index] = input[index];
            index++;
        }
        token[index] = 0;

        while (input[index] && !is_space(input[index]))
        {
            index++;
        }

        return skip_spaces(input + index);
    }

    bool path_is_absolute(const char* path)
    {
        return path[0] == '/';
    }

    bool path_is_root(const char* path)
    {
        return path[0] == '/' && path[1] == 0;
    }

    bool is_name_char(char c)
    {
        return (c >= 'a' && c <= 'z')
            || (c >= 'A' && c <= 'Z')
            || (c >= '0' && c <= '9')
            || c == '_'
            || c == '-'
            || c == '.';
    }

    void reset_node(VirtualNode& node)
    {
        node.used = false;
        node.is_directory = false;
        node.parent = -1;
        node.first_child = -1;
        node.next_sibling = -1;
        clear_string(node.name, kMaxName);
        clear_string(node.content, kMaxContent);
    }

    int find_child(int parent, const char* name)
    {
        for (int child = nodes[parent].first_child; child != -1; child = nodes[child].next_sibling)
        {
            if (streq(nodes[child].name, name))
            {
                return child;
            }
        }
        return -1;
    }

    int allocate_node()
    {
        for (int index = 1; index < kMaxNodes; index++)
        {
            if (!nodes[index].used)
            {
                return index;
            }
        }
        return -1;
    }

    bool validate_segment(const char* segment)
    {
        if (*segment == 0 || streq(segment, ".") || streq(segment, ".."))
        {
            return false;
        }

        for (int index = 0; segment[index]; index++)
        {
            if (!is_name_char(segment[index]))
            {
                return false;
            }
        }

        return true;
    }

    bool create_entry(int parent, const char* name, bool is_directory, const char* content);

    void initialize_virtual_fs()
    {
        for (int index = 0; index < kMaxNodes; index++)
        {
            reset_node(nodes[index]);
        }

        nodes[0].used = true;
        nodes[0].is_directory = true;
        nodes[0].parent = -1;
        nodes[0].first_child = -1;
        nodes[0].next_sibling = -1;
        copy_string(nodes[0].name, kMaxName, "/");

        create_entry(0, "README.md", false, "This is a virtual in-memory shell filesystem.\n");
        create_entry(0, "grub.cfg", false, "set timeout=0\n");
        create_entry(0, "boot", true, 0);
        create_entry(0, "include", true, 0);
        create_entry(0, "kernel", true, 0);
        create_entry(0, "iso", true, 0);
    }

    bool create_entry(int parent, const char* name, bool is_directory, const char* content)
    {
        if (parent < 0 || parent >= kMaxNodes || !nodes[parent].used || !nodes[parent].is_directory)
        {
            return false;
        }

        if (!validate_segment(name) || find_child(parent, name) != -1)
        {
            return false;
        }

        int slot = allocate_node();
        if (slot == -1)
        {
            return false;
        }

        nodes[slot].used = true;
        nodes[slot].is_directory = is_directory;
        nodes[slot].parent = parent;
        nodes[slot].first_child = -1;
        nodes[slot].next_sibling = nodes[parent].first_child;
        nodes[parent].first_child = slot;
        copy_string(nodes[slot].name, kMaxName, name);
        clear_string(nodes[slot].content, kMaxContent);

        if (!is_directory && content)
        {
            copy_string(nodes[slot].content, kMaxContent, content);
        }

        return true;
    }

    void unlink_child(int child)
    {
        int parent = nodes[child].parent;
        if (parent < 0)
        {
            return;
        }

        int previous = -1;
        for (int current = nodes[parent].first_child; current != -1; current = nodes[current].next_sibling)
        {
            if (current == child)
            {
                if (previous == -1)
                {
                    nodes[parent].first_child = nodes[current].next_sibling;
                }
                else
                {
                    nodes[previous].next_sibling = nodes[current].next_sibling;
                }
                break;
            }
            previous = current;
        }
    }

    void delete_entry(int index)
    {
        unlink_child(index);
        reset_node(nodes[index]);
    }

    bool directory_is_empty(int index)
    {
        return nodes[index].first_child == -1;
    }

    int resolve_path(const char* path, int start_directory)
    {
        if (*path == 0)
        {
            return start_directory;
        }

        if (path_is_root(path))
        {
            return 0;
        }

        int current = path_is_absolute(path) ? 0 : start_directory;
        const char* cursor = path_is_absolute(path) ? path + 1 : path;

        while (*cursor)
        {
            char segment[kMaxName];
            int segment_length = 0;

            while (cursor[segment_length] && cursor[segment_length] != '/' && segment_length < kMaxName - 1)
            {
                segment[segment_length] = cursor[segment_length];
                segment_length++;
            }
            segment[segment_length] = 0;

            while (cursor[segment_length] && cursor[segment_length] != '/')
            {
                segment_length++;
            }

            if (segment[0] == 0)
            {
                return -1;
            }

            if (streq(segment, "."))
            {
                // Stay in the same directory.
            }
            else if (streq(segment, ".."))
            {
                if (current != 0)
                {
                    current = nodes[current].parent;
                }
            }
            else if (!validate_segment(segment))
            {
                return -1;
            }
            else
            {
                int child = find_child(current, segment);
                if (child == -1)
                {
                    return -1;
                }
                current = child;
            }

            cursor += segment_length;
            while (*cursor == '/')
            {
                cursor++;
            }
        }

        return current;
    }

    bool split_parent_and_leaf(const char* path, char* parent, int parent_size, char* leaf, int leaf_size)
    {
        int last_slash = -1;

        for (int index = 0; path[index]; index++)
        {
            if (path[index] == '/')
            {
                last_slash = index;
            }
        }

        if (last_slash < 0)
        {
            if (string_length(path) >= leaf_size)
            {
                return false;
            }

            copy_string(leaf, leaf_size, path);
            parent[0] = 0;
            return true;
        }

        int leaf_length = string_length(path) - last_slash - 1;
        if (leaf_length <= 0 || leaf_length >= leaf_size)
        {
            return false;
        }

        if (last_slash == 0)
        {
            if (parent_size < 2)
            {
                return false;
            }
            parent[0] = '/';
            parent[1] = 0;
        }
        else
        {
            if (last_slash >= parent_size)
            {
                return false;
            }

            for (int index = 0; index < last_slash; index++)
            {
                parent[index] = path[index];
            }
            parent[last_slash] = 0;
        }

        for (int index = 0; index < leaf_length; index++)
        {
            leaf[index] = path[last_slash + 1 + index];
        }
        leaf[leaf_length] = 0;
        return true;
    }

    void print_node_path(int index)
    {
        if (index == 0)
        {
            terminal_write("/\n");
            return;
        }

        char segments[16][kMaxName];
        int segment_count = 0;
        int current = index;

        while (current > 0 && segment_count < 16)
        {
            copy_string(segments[segment_count], kMaxName, nodes[current].name);
            segment_count++;
            current = nodes[current].parent;
        }

        char path[kMaxPath];
        int position = 0;
        path[position++] = '/';

        for (int segment = segment_count - 1; segment >= 0; segment--)
        {
            for (int character = 0; segments[segment][character] && position < kMaxPath - 1; character++)
            {
                path[position++] = segments[segment][character];
            }

            if (segment > 0 && position < kMaxPath - 1)
            {
                path[position++] = '/';
            }
        }

        path[position] = 0;
        terminal_write(path);
        terminal_write("\n");
    }

    void print_directory_listing(int index)
    {
        for (int child = nodes[index].first_child; child != -1; child = nodes[child].next_sibling)
        {
            terminal_write(nodes[child].name);
            if (nodes[child].is_directory)
            {
                terminal_write("/");
            }
            terminal_write("  ");
        }

        terminal_write("\n");
    }

    void print_help()
    {
        terminal_write("Commands:\n");
        terminal_write("  help                 Show this help\n");
        terminal_write("  name [newname]       Show or change system name\n");
        terminal_write("  echo <text>          Print text\n");
        terminal_write("  ls [path]            List a directory\n");
        terminal_write("  pwd                  Print current path\n");
        terminal_write("  cd <path>            Change directory\n");
        terminal_write("  mkdir <path>         Create a directory\n");
        terminal_write("  touch <path>         Create an empty file\n");
        terminal_write("  write <file> <text>  Replace file contents\n");
        terminal_write("  cat <file>           Print file contents\n");
        terminal_write("  rm <file>            Delete a file\n");
        terminal_write("  rmdir <dir>          Delete an empty directory\n");
        terminal_write("  uname                Show system name\n");
        terminal_write("  whoami               Show current user\n");
        terminal_write("  clear                Clear screen\n");
    }

    void run_command(const char* command)
    {
        const char* trimmed = skip_spaces(command);

        if (*trimmed == 0)
        {
            return;
        }

        char name[kMaxName];
        const char* arguments = read_token(trimmed, name, kMaxName);

        if (streq(name, "help"))
        {
            print_help();
            return;
        }

        if (streq(name, "name"))
        {
            char new_name[kMaxName];
            read_token(arguments, new_name, kMaxName);

            if (new_name[0] == 0)
            {
                terminal_write(system_name);
                terminal_write("\n");
                return;
            }

            if (!validate_segment(new_name))
            {
                terminal_write("name: invalid name\n");
                return;
            }

            copy_string(system_name, kMaxName, new_name);
            terminal_write("System name changed to ");
            terminal_write(system_name);
            terminal_write("\n");
            return;
        }

        if (streq(name, "echo"))
        {
            terminal_write(skip_spaces(arguments));
            terminal_write("\n");
            return;
        }

        if (streq(name, "ls"))
        {
            char path[kMaxPath];
            const char* path_argument = skip_spaces(arguments);

            if (*path_argument == 0)
            {
                print_directory_listing(current_directory);
                return;
            }

            read_token(path_argument, path, kMaxPath);
            int directory = resolve_path(path, current_directory);
            if (directory == -1 || !nodes[directory].is_directory)
            {
                terminal_write("ls: no such directory: ");
                terminal_write(path);
                terminal_write("\n");
                return;
            }

            print_directory_listing(directory);
            return;
        }

        if (streq(name, "pwd"))
        {
            print_node_path(current_directory);
            return;
        }

        if (streq(name, "cd"))
        {
            char path[kMaxPath];
            read_token(arguments, path, kMaxPath);

            if (path[0] == 0)
            {
                terminal_write("cd: missing path\n");
                return;
            }

            int target = resolve_path(path, current_directory);
            if (target == -1 || !nodes[target].is_directory)
            {
                terminal_write("cd: not a directory: ");
                terminal_write(path);
                terminal_write("\n");
                return;
            }

            current_directory = target;
            return;
        }

        if (streq(name, "mkdir"))
        {
            char path[kMaxPath];
            char parent_path[kMaxPath];
            char leaf[kMaxName];
            read_token(arguments, path, kMaxPath);

            if (path[0] == 0)
            {
                terminal_write("mkdir: missing path\n");
                return;
            }

            if (!split_parent_and_leaf(path, parent_path, kMaxPath, leaf, kMaxName))
            {
                terminal_write("mkdir: invalid path\n");
                return;
            }

            int parent = parent_path[0] == 0 ? current_directory : resolve_path(parent_path, current_directory);
            if (parent == -1 || !nodes[parent].is_directory)
            {
                terminal_write("mkdir: parent directory not found\n");
                return;
            }

            if (!create_entry(parent, leaf, true, 0))
            {
                terminal_write("mkdir: could not create ");
                terminal_write(path);
                terminal_write("\n");
                return;
            }

            return;
        }

        if (streq(name, "touch"))
        {
            char path[kMaxPath];
            char parent_path[kMaxPath];
            char leaf[kMaxName];
            read_token(arguments, path, kMaxPath);

            if (path[0] == 0)
            {
                terminal_write("touch: missing path\n");
                return;
            }

            if (!split_parent_and_leaf(path, parent_path, kMaxPath, leaf, kMaxName))
            {
                terminal_write("touch: invalid path\n");
                return;
            }

            int parent = parent_path[0] == 0 ? current_directory : resolve_path(parent_path, current_directory);
            if (parent == -1 || !nodes[parent].is_directory)
            {
                terminal_write("touch: parent directory not found\n");
                return;
            }

            int existing = find_child(parent, leaf);
            if (existing != -1)
            {
                if (nodes[existing].is_directory)
                {
                    terminal_write("touch: path is a directory\n");
                }
                return;
            }

            if (!create_entry(parent, leaf, false, ""))
            {
                terminal_write("touch: could not create ");
                terminal_write(path);
                terminal_write("\n");
                return;
            }

            return;
        }

        if (streq(name, "write"))
        {
            char path[kMaxPath];
            char parent_path[kMaxPath];
            char leaf[kMaxName];
            read_token(arguments, path, kMaxPath);

            if (path[0] == 0)
            {
                terminal_write("write: missing file\n");
                return;
            }

            const char* text = skip_spaces(arguments);

            if (!split_parent_and_leaf(path, parent_path, kMaxPath, leaf, kMaxName))
            {
                terminal_write("write: invalid path\n");
                return;
            }

            int parent = parent_path[0] == 0 ? current_directory : resolve_path(parent_path, current_directory);
            if (parent == -1 || !nodes[parent].is_directory)
            {
                terminal_write("write: parent directory not found\n");
                return;
            }

            int existing = find_child(parent, leaf);
            if (existing == -1)
            {
                if (!create_entry(parent, leaf, false, text))
                {
                    terminal_write("write: could not create ");
                    terminal_write(path);
                    terminal_write("\n");
                    return;
                }

                return;
            }

            if (nodes[existing].is_directory)
            {
                terminal_write("write: path is a directory\n");
                return;
            }

            copy_string(nodes[existing].content, kMaxContent, text);
            return;
        }

        if (streq(name, "cat"))
        {
            char path[kMaxPath];
            read_token(arguments, path, kMaxPath);

            if (path[0] == 0)
            {
                terminal_write("cat: missing file\n");
                return;
            }

            int target = resolve_path(path, current_directory);
            if (target == -1)
            {
                terminal_write("cat: file not found: ");
                terminal_write(path);
                terminal_write("\n");
                return;
            }

            if (nodes[target].is_directory)
            {
                terminal_write("cat: is a directory: ");
                terminal_write(path);
                terminal_write("\n");
                return;
            }

            terminal_write(nodes[target].content);
            terminal_write("\n");
            return;
        }

        if (streq(name, "rm"))
        {
            char path[kMaxPath];
            read_token(arguments, path, kMaxPath);

            if (path[0] == 0)
            {
                terminal_write("rm: missing file\n");
                return;
            }

            int target = resolve_path(path, current_directory);
            if (target == -1 || nodes[target].is_directory)
            {
                terminal_write("rm: file not found: ");
                terminal_write(path);
                terminal_write("\n");
                return;
            }

            delete_entry(target);
            return;
        }

        if (streq(name, "rmdir"))
        {
            char path[kMaxPath];
            read_token(arguments, path, kMaxPath);

            if (path[0] == 0)
            {
                terminal_write("rmdir: missing directory\n");
                return;
            }

            int target = resolve_path(path, current_directory);
            if (target <= 0 || !nodes[target].is_directory)
            {
                terminal_write("rmdir: directory not found: ");
                terminal_write(path);
                terminal_write("\n");
                return;
            }

            if (!directory_is_empty(target))
            {
                terminal_write("rmdir: directory not empty\n");
                return;
            }

            delete_entry(target);
            return;
        }

        if (streq(name, "uname"))
        {
            terminal_write(system_name);
            terminal_write("\n");
            return;
        }

        if (streq(name, "whoami"))
        {
            terminal_write("root\n");
            return;
        }

        if (streq(name, "clear"))
        {
            terminal_initialize();
            return;
        }

        terminal_write("Unknown command: ");
        terminal_write(trimmed);
        terminal_write("\nType 'help' for commands\n");
    }

    void submit_input_line()
    {
        input_buffer[input_len] = '\0';
        terminal_write("\n");
        run_command(input_buffer);
        input_len = 0;
        terminal_write(kPrompt);
    }

    unsigned char read_scancode_blocking()
    {
        for (;;)
        {
            if (inb(0x64) & 0x01)
            {
                return inb(0x60);
            }
        }
    }

    char read_input_char_blocking()
    {
        for (;;)
        {
            if (serial_has_data())
            {
                char c = serial_read_char();
                if (c == '\r')
                {
                    return '\n';
                }
                if (c == 0x7F)
                {
                    return '\b';
                }
                return c;
            }

            if (inb(0x64) & 0x01)
            {
                unsigned char scancode = inb(0x60);
                if (scancode & 0x80)
                {
                    continue;
                }

                char ch = scancode_to_ascii(scancode);
                if (ch != 0)
                {
                    return ch;
                }
            }
        }
    }

    void shell_loop()
    {
        terminal_write(kPrompt);

        for (;;)
        {
            char ch = read_input_char_blocking();

            if (ch == '\n')
            {
                submit_input_line();
                continue;
            }

            if (ch == '\b')
            {
                if (input_len > 0)
                {
                    input_len--;
                    terminal_backspace();
                }
                continue;
            }

            if (input_len < (kInputMax - 1))
            {
                input_buffer[input_len++] = ch;
                terminal_putchar(ch);
            }
        }
    }
}

extern "C" void kernel_main()
{
    terminal_initialize();
    serial_initialize();
    initialize_virtual_fs();
    terminal_write(system_name);
    terminal_write(" v0.1\n");
    terminal_write("Type 'help' for commands\n");
    shell_loop();
}
