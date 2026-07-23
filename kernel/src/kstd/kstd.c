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
