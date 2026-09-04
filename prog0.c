#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

// Union to store 4 bytes as an array of bytes, an unsigned, signed, or float number
union value {
    unsigned uval;
    int sval;
    float fval;
    unsigned char bytes[4];
};

// reads 8 hex characters from string input and stores it in the union v
// returns -1 if the hexadecimal number is invalid, 0 otherwise
int read_hex(union value *v, char *input);

// converts the ASCII hex character c to binary
// returns the hex value of c if c is a valid hex digit, -1 otherwise
char hexDigit(char c);

// returns true if x has any even bit equal to 1, 0 otherwise
int any_even_one(unsigned x);

// returns a mask indicating the position of the left most one in x
int leftmost_one(unsigned x);

// returns x shifted n positions to the left with the n most significant bits of x
// inserted at the right of x
unsigned rotate_left(unsigned x, int n);

// returns x shifted n positions to the right with the n least significant bits of x
// inserted at the left of x
unsigned rotate_right(unsigned x, int n);

// returns x+y if no overflow occurs
// returns TMAX if a positive overflow occurs
// returns TMIN if a negative overflow occurs
int saturating_add(int x, int y);

// multiplies the binary representation of a float number f by 2
unsigned float_twice(unsigned f);

// divides the binary representation of a float number f by 2
unsigned float_half(unsigned f);


/* Convert one hexadecimal character to its numeric value */
char hexDigit(char c) {
    if (c >= '0' && c <= '9')
        return c - '0';

    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;

    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;

    return -1;
}


/* Read exactly 8 hexadecimal characters */
int read_hex(union value *v, char *input) {
    int i;
    unsigned result = 0;
    char h;

    if (strlen(input) != 8)
        return -1;

    for (i = 0; i < 8; i++) {
        h = hexDigit(input[i]);

        if (h == -1)
            return -1;

        result = (result << 4) | h;
    }

    /*
     * Store the four bytes in little-endian order.
     */
    v->bytes[0] = result & 0xff;
    v->bytes[1] = (result >> 8) & 0xff;
    v->bytes[2] = (result >> 16) & 0xff;
    v->bytes[3] = (result >> 24) & 0xff;

    return 0;
}


/* Return 1 if any even-numbered bit is 1 */
int any_even_one(unsigned x) {
    /*
     * Bits 0, 2, 4, ... are selected by 0x55555555.
     */
    return (x & 0x55555555) != 0;
}


/* Return a mask containing only the leftmost 1 bit */
int leftmost_one(unsigned x) {
    if (x == 0)
        return 0;

    /*
     * Spread the leftmost 1 to all positions to its right.
     */
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;

    /*
     * Isolate the original leftmost bit.
     */
    return x ^ (x >> 1);
}


/* Rotate x left by n bits */
unsigned rotate_left(unsigned x, int n) {
    if (n == 0)
        return x;

    return (x << n) | (x >> (32 - n));
}


/* Rotate x right by n bits */
unsigned rotate_right(unsigned x, int n) {
    if (n == 0)
        return x;

    return (x >> n) | (x << (32 - n));
}


/* Saturating addition */
int saturating_add(int x, int y) {
    int sum = x + y;

    /*
     * Positive overflow:
     * x and y are positive, but sum became negative.
     */
    if (x > 0 && y > 0 && sum < 0)
        return INT_MAX;

    /*
     * Negative overflow:
     * x and y are negative, but sum became positive.
     */
    if (x < 0 && y < 0 && sum >= 0)
        return INT_MIN;

    return sum;
}


/* Multiply floating-point bit representation by 2 */
unsigned float_twice(unsigned f) {
    unsigned sign = f & 0x80000000;
    unsigned exp = f & 0x7f800000;
    unsigned frac = f & 0x007fffff;

    /*
     * NaN or infinity: return unchanged.
     */
    if (exp == 0x7f800000)
        return f;

    /*
     * Denormalized number:
     * shift fraction left by one.
     */
    if (exp == 0) {
        frac <<= 1;

        /*
         * If the shift creates a leading bit,
         * the number becomes normalized.
         */
        if (frac & 0x00800000) {
            exp = 0x00800000;
            frac &= 0x007fffff;
        }

        return sign | exp | frac;
    }

    /*
     * Normalized number:
     * increase exponent by 1.
     */
    exp += 0x00800000;

    /*
     * If exponent becomes 255, fraction must be zero.
     */
    if (exp == 0x7f800000)
        frac = 0;

    return sign | exp | frac;
}


/* Divide floating-point bit representation by 2 */
unsigned float_half(unsigned f) {
    unsigned sign = f & 0x80000000;
    unsigned exp = (f >> 23) & 0xff;
    unsigned frac = f & 0x007fffff;

    /*
     * NaN or infinity: return unchanged.
     */
    if (exp == 0xff)
        return f;

    /*
     * If normalized with exponent 1,
     * dividing by 2 makes it denormalized.
     */
    if (exp == 1) {
        unsigned combined = frac | 0x00800000;
        unsigned remainder = combined & 1;

        combined >>= 1;

        /*
         * Round to even.
         */
        if (remainder && (combined & 1))
            combined++;

        return sign | combined;
    }

    /*
     * Already denormalized.
     */
    if (exp == 0) {
        unsigned remainder = frac & 1;

        frac >>= 1;

        /*
         * Round to even.
         */
        if (remainder && (frac & 1))
            frac++;

        return sign | frac;
    }

    /*
     * Normalized number with exponent > 1.
     */
    exp--;

    return sign | (exp << 23) | frac;
}


int main(int argc, char** argv) {

    union value v;
    union value v2;
    int n;

    /*
     * Check number of arguments.
     */
    if (argc != 3 && argc != 4) {
        printf("Invalid number of arguments\n");
        exit(0);
    }


    /*
     * ANY EVEN ONE
     * ./prog0 even A0000000
     */
    if (strcmp(argv[1], "even") == 0) {

        if (argc != 3) {
            printf("Invalid number of arguments\n");
            exit(0);
        }

        if (read_hex(&v, argv[2]) == -1) {
            printf("Invalid hex value\n");
            exit(0);
        }

        if (any_even_one(v.uval))
            printf("True\n");
        else
            printf("False\n");
    }


    /*
     * LEFTMOST ONE
     * ./prog0 left 7f800000
     */
    else if (strcmp(argv[1], "left") == 0) {

        if (argc != 3) {
            printf("Invalid number of arguments\n");
            exit(0);
        }

        if (read_hex(&v, argv[2]) == -1) {
            printf("Invalid hex value\n");
            exit(0);
        }

        printf("%08x\n", leftmost_one(v.uval));
    }


    /*
     * ROTATE LEFT
     * ./prog0 lrotate 7F400000 2
     */
    else if (strcmp(argv[1], "lrotate") == 0) {

        if (argc != 4) {
            printf("Invalid number of arguments\n");
            exit(0);
        }

        if (read_hex(&v, argv[2]) == -1) {
            printf("Invalid hex value\n");
            exit(0);
        }

        n = atoi(argv[3]);

        if (n < 0 || n > 32) {
            printf("Invalid number of shift positions\n");
            exit(0);
        }

        printf("%08x\n", rotate_left(v.uval, n));
    }


    /*
     * ROTATE RIGHT
     * ./prog0 rrotate 7F400000 2
     */
    else if (strcmp(argv[1], "rrotate") == 0) {

        if (argc != 4) {
            printf("Invalid number of arguments\n");
            exit(0);
        }

        if (read_hex(&v, argv[2]) == -1) {
            printf("Invalid hex value\n");
            exit(0);
        }

        n = atoi(argv[3]);

        if (n < 0 || n > 32) {
            printf("Invalid number of shift positions\n");
            exit(0);
        }

        printf("%08x\n", rotate_right(v.uval, n));
    }


    /*
     * SATURATING ADD
     * ./prog0 saturate 7fffffff 00000010
     */
    else if (strcmp(argv[1], "saturate") == 0) {

        if (argc != 4) {
            printf("Invalid number of arguments\n");
            exit(0);
        }

        if (read_hex(&v, argv[2]) == -1 ||
            read_hex(&v2, argv[3]) == -1) {
            printf("Invalid hex value\n");
            exit(0);
        }

        v.sval = saturating_add(v.sval, v2.sval);

        printf("%08x %d\n", v.uval, v.sval);
    }


    /*
     * FLOAT TWICE
     * ./prog0 twice 3f400000
     */
    else if (strcmp(argv[1], "twice") == 0) {

        if (argc != 3) {
            printf("Invalid number of arguments\n");
            exit(0);
        }

        if (read_hex(&v, argv[2]) == -1) {
            printf("Invalid hex value\n");
            exit(0);
        }

        v.uval = float_twice(v.uval);

        printf("%08x %e\n", v.uval, v.fval);
    }


    /*
     * FLOAT HALF
     * ./prog0 half 3fc00000
     */
    else if (strcmp(argv[1], "half") == 0) {

        if (argc != 3) {
            printf("Invalid number of arguments\n");
            exit(0);
        }

        if (read_hex(&v, argv[2]) == -1) {
            printf("Invalid hex value\n");
            exit(0);
        }

        v.uval = float_half(v.uval);

        printf("%08x %e\n", v.uval, v.fval);
    }


    /*
     * Invalid operation
     */
    else {
        printf("Invalid operation\n");
    }

    return 0;
}