#include <util.h>


int get_msb(unsigned int n) {
    if (n == 0) {
        return 0;
    }
    int i = 0;
    n = n / 2;
    while (n != 0) {
        i++;
        n = n / 2;
    }
    return i;
}