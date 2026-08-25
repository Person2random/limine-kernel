#include <stdint.h>
void utoa(uint64_t value, char *buffer)
{
    char temp[21]; // uint64_t max is 20 decimal digits
    int i = 0;
    int j = 0;

    if(value == 0)
    {
        buffer[0] = '0';
        buffer[1] = '\0';
        return;
    }

    while(value > 0)
    {
        temp[i++] = '0' + (value % 10);
        value /= 10;
    }

    while(i > 0)
    {
        buffer[j++] = temp[--i];
    }

    buffer[j] = '\0';
}


char* int_to_base_string(int value, char* buffer, int base) {
    if (base < 2 || base > 36) {
        if (buffer) {
            buffer[0] = '\0';
        }
        return buffer;
    }

    char* ptr = buffer;
    char* tptr = buffer;
    char temp_char;
    int remainder;
    
    int is_negative = 0;
    unsigned int unsigned_value = (unsigned int)value;

    // Handle sign safely for base 10
    if (value < 0 && base == 10) {
        is_negative = 1;
        // Two's complement safe negation
        unsigned_value = (unsigned int)(-value); 
    }

    // Extract digits in reverse order
    do {
        remainder = unsigned_value % base;
        *ptr++ = (remainder < 10) ? (remainder + '0') : (remainder - 10 + 'A');
        unsigned_value /= base;
    } while (unsigned_value > 0);

    if (is_negative) {
        *ptr++ = '-';
    }

    *ptr = '\0';

    // Reverse the string in-place
    ptr--;
    while (tptr < ptr) {
        temp_char = *tptr;
        *tptr = *ptr;
        *ptr = temp_char;
        tptr++;
        ptr--;
    }

    return buffer;
}
