// Author: strawberryhacker

#include "utilities.h"

//--------------------------------------------------------------------------------------------------

enum {
    FORMAT_FLAG_ZERO_PAD     = 0x1,
    FORMAT_FLAG_NO_SIGN      = 0x2,
    FORMAT_FLAG_ALWAYS_SIGN  = 0x4,
    FORMAT_FLAG_LEFT_JUSTIFY = 0x8,
    FORMAT_FLAG_SIZE_GIVEN   = 0x10,
    FORMAT_FLAG_UPPERCASE    = 0x20,
    FORMAT_FLAG_HEX          = 0x40,
    FORMAT_FLAG_SIGNED       = 0x80,
    FORMAT_FLAG_BINARY       = 0x100,
    FORMAT_FLAG_BIG_NUMBER   = 0x200,
};

//--------------------------------------------------------------------------------------------------

static void write_char(char c, char** buffer, char* end) {
    if (*buffer != end) {
        *(*buffer)++ = c; 
    }
}

//--------------------------------------------------------------------------------------------------

static char* write_number(s64 value, int width, u32 flags, char* buffer, char* end) {
    char* hex_table = (flags & FORMAT_FLAG_UPPERCASE) ? "0123456789ABCDEF" : "0123456789abcdef";
    int base = (flags & FORMAT_FLAG_HEX) ? 16 : (flags & FORMAT_FLAG_BINARY) ? 2 : 10;
    int left = (flags & FORMAT_FLAG_LEFT_JUSTIFY);
    char pad_char = (flags & FORMAT_FLAG_ZERO_PAD) ? '0' : ' ';
    char number_buffer[64];
    u64 unsigned_value = 0;
    int length = 0;
    char sign = 0;
    int pad = 0;

    if (flags & FORMAT_FLAG_SIGNED && (flags & FORMAT_FLAG_NO_SIGN) == 0 && value < 0) {
        sign = '-';
        value = -value;
    }

    if ((flags & FORMAT_FLAG_SIGNED) == 0 && flags & FORMAT_FLAG_ALWAYS_SIGN) {
        sign = '+';
    }

    if (flags & FORMAT_FLAG_BIG_NUMBER) {
        unsigned_value = (u64)value;
    }
    else {
        unsigned_value = (u64)(u32)value;
    }

    do {
        number_buffer[length++] = hex_table[unsigned_value % base];
        unsigned_value = unsigned_value / base;
    } while (unsigned_value);

    if (sign) {
        length++;
    }

    pad = (width > length) ? width - length : 0;

    while (left == 0 && pad--) {
        write_char(pad_char, &buffer, end);
    }

    if (sign) {
        length--;
        write_char(sign, &buffer, end);
    }

    do {
        write_char(number_buffer[--length], &buffer, end);
    } while (length);

    while (left && pad--) {
        write_char(pad_char, &buffer, end);
    }

    return buffer;
}

//--------------------------------------------------------------------------------------------------

static s64 read_number(va_list* arguments, int flags) {
    return (flags & FORMAT_FLAG_BIG_NUMBER) ? (s64)va_arg(*arguments, long long) : (s64)va_arg(*arguments, int);
}

//--------------------------------------------------------------------------------------------------

int format_string(const char* string, char* buffer, int buffer_size, va_list arguments) {
    char* start = buffer;
    char* end = buffer + buffer_size;

    while (*string) {
        if (*string != '{') {
            write_char(*string++, &buffer, end);
            continue;
        }

        int flags = 0;
        int width = 0;
        char c = 0;

        while (1) {
            c = *(++string);

            if (c == '0') {
                flags |= FORMAT_FLAG_ZERO_PAD;
            }
            else if (c == '<') {
                flags |= FORMAT_FLAG_LEFT_JUSTIFY;
            }
            else if (c == '+') {
                flags |= FORMAT_FLAG_ALWAYS_SIGN;
            }
            else if (c == ' ') {
                flags |= FORMAT_FLAG_NO_SIGN;
            }
            else if (c == '_') {
                width = va_arg(arguments, int);
                if (width < 0) {
                    width = -width;
                    flags |= FORMAT_FLAG_SIZE_GIVEN;
                    flags |= FORMAT_FLAG_LEFT_JUSTIFY;
                }
            }
            else {
                break;
            }
        }

        c = *string;

        if ('0' <= c && c <= '9') {
            flags |= FORMAT_FLAG_SIZE_GIVEN;
        }

        for (; '0' <= c && c <= '9'; c = *(++string)) {
            width = 10 * width + (c - '0');
        }

        c = *string++;

        if (*string == 'l') {
            flags |= FORMAT_FLAG_BIG_NUMBER;
            string++;
        }

        if (c == 't') {
            char* text = va_arg(arguments, char*);
            int left = flags & FORMAT_FLAG_LEFT_JUSTIFY;
            char pad_char = (flags & FORMAT_FLAG_ZERO_PAD) ? '0' : ' ';
            int length = 0;
            int pad = 0;

            if (text == 0) {
                text = "nullptr";
            }

            if (flags & FORMAT_FLAG_SIZE_GIVEN) {
                while (text[length] && length < width) {
                    length++;
                }

                pad = width - length;
            }
            else {
                while (text[length]) {
                    length++;
                }
            }
            
            while (left == 0 && pad--) {
                write_char(pad_char, &buffer, end);
            }

            while (length--) {
                write_char(*text++, &buffer, end);   
            }

            while (left && pad--) {
                write_char(pad_char, &buffer, end);
            }
        }
        else if (c == 'u') {
            buffer = write_number((flags & FORMAT_FLAG_BIG_NUMBER) ? (s64)va_arg(arguments, long long) : (s64)va_arg(arguments, unsigned int), width, flags, buffer, end);
        }
        else if (c == 's') {
            flags |= FORMAT_FLAG_SIGNED;
            buffer = write_number(read_number(&arguments, flags), width, flags, buffer, end);
        }
        else if (c == 'h') {
            flags |= FORMAT_FLAG_HEX;
            buffer = write_number(read_number(&arguments, flags), width, flags, buffer, end);
        }
        else if (c == 'b') {
            flags |= FORMAT_FLAG_BINARY;
            buffer = write_number(read_number(&arguments, flags), width, flags, buffer, end);
        }
        else if (c == '{') {
            write_char('{', &buffer, end);
        }

        if (*string == '}') {
            string++;
        }
    }

    return buffer - start;
}

//--------------------------------------------------------------------------------------------------

void memory_copy(const void* source, void* destination, int size) {
    const u8* source_pointer = source;
    u8* destination_pointer = destination;

    for (int i = 0; i < size; i++) {
        destination_pointer[i] = source_pointer[i];
    }
}

//--------------------------------------------------------------------------------------------------

void memory_move(const void* source, void* destination, int size) {
    const u8* source_pointer = source;
    u8* dest_pointer = destination;

    if (dest_pointer > source_pointer) {
        for (int i = 0; i < size; i++) {
            dest_pointer[size - 1 - i] = source_pointer[size - 1 - i];
        }
    }
    else if (dest_pointer < source_pointer) {
        for (int i = 0; i < size; i++) {
            dest_pointer[i] = source_pointer[i];
        }
    }
}

//--------------------------------------------------------------------------------------------------

void memory_fill(void* memory, u8 fill, int size) {
    u8* destination = memory;

    for (int i = 0; i < size; i++) {
        destination[i] = fill;
    }
}

//--------------------------------------------------------------------------------------------------

void memory_clear(void* memory, int size) {
    u8* destination = memory;

    for (int i = 0; i < size; i++) {
        destination[i] = 0;
    }
}

//--------------------------------------------------------------------------------------------------

int align_down(int value, int alignment) {
    return value / alignment * alignment;
}

//--------------------------------------------------------------------------------------------------

int align_up(int value, int alignment) {
    return (value + alignment - 1) / alignment * alignment;
}

//--------------------------------------------------------------------------------------------------

void* pointer_align_down(void* pointer, int alignment) {
    return (void *)((ptr)pointer / (ptr)alignment * (ptr)alignment);
}

//--------------------------------------------------------------------------------------------------

void* pointer_align_up(void* pointer, int alignment) {
    return (void *)(((ptr)pointer + (ptr)alignment - 1) / (ptr)alignment * (ptr)alignment);
}
