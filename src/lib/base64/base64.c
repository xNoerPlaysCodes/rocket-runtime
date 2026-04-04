#include "lib/base64/base64.h"

uint8_t char_table[64] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

size_t b64_encodedLength(size_t length) { return (length + 2) / 3 * 4; }

size_t b64_decodedLength(size_t length) { return length * 3 / 4; }

void b64_encode(uint8_t *src, size_t len, uint8_t *dst) {
  for (size_t src_pos = 0; src_pos < len; src_pos += 3) {
    dst[0] = char_table[src[src_pos] >> 2];
    uint8_t s1 = (src[src_pos] & 0x03) << 4;
    if (src_pos + 1 < len)
      s1 |= src[src_pos + 1] >> 4;
    dst[1] = char_table[s1];

    if (src_pos + 1 < len) {
      uint8_t s2 = (src[src_pos + 1] & 0x0f) << 2;
      if (src_pos + 2 < len)
        s2 |= src[src_pos + 2] >> 6;
      dst[2] = char_table[s2];

      if (src_pos + 2 < len) {
        dst[3] = char_table[src[src_pos + 2] & 0x3f];
      } else {
        dst[3] = '=';
      }
    } else {
      dst[2] = '=';
      dst[3] = '=';
    }

    dst += 4;
  }
}

uint8_t lookupB64(uint8_t c) {
  switch (c) {
  default:
  case 'A':
    return 0;
  case 'B':
    return 1;
  case 'C':
    return 2;
  case 'D':
    return 3;
  case 'E':
    return 4;
  case 'F':
    return 5;
  case 'G':
    return 6;
  case 'H':
    return 7;
  case 'I':
    return 8;
  case 'J':
    return 9;
  case 'K':
    return 10;
  case 'L':
    return 11;
  case 'M':
    return 12;
  case 'N':
    return 13;
  case 'O':
    return 14;
  case 'P':
    return 15;
  case 'Q':
    return 16;
  case 'R':
    return 17;
  case 'S':
    return 18;
  case 'T':
    return 19;
  case 'U':
    return 20;
  case 'V':
    return 21;
  case 'W':
    return 22;
  case 'X':
    return 23;
  case 'Y':
    return 24;
  case 'Z':
    return 25;
  case 'a':
    return 26;
  case 'b':
    return 27;
  case 'c':
    return 28;
  case 'd':
    return 29;
  case 'e':
    return 30;
  case 'f':
    return 31;
  case 'g':
    return 32;
  case 'h':
    return 33;
  case 'i':
    return 34;
  case 'j':
    return 35;
  case 'k':
    return 36;
  case 'l':
    return 37;
  case 'm':
    return 38;
  case 'n':
    return 39;
  case 'o':
    return 40;
  case 'p':
    return 41;
  case 'q':
    return 42;
  case 'r':
    return 43;
  case 's':
    return 44;
  case 't':
    return 45;
  case 'u':
    return 46;
  case 'v':
    return 47;
  case 'w':
    return 48;
  case 'x':
    return 49;
  case 'y':
    return 50;
  case 'z':
    return 51;
  case '0':
    return 52;
  case '1':
    return 53;
  case '2':
    return 54;
  case '3':
    return 55;
  case '4':
    return 56;
  case '5':
    return 57;
  case '6':
    return 58;
  case '7':
    return 59;
  case '8':
    return 60;
  case '9':
    return 61;
  case '+':
    return 62;
  case '/':
    return 63;
  }
}

size_t b64_decode(uint8_t *src, size_t len, uint8_t *dst) {
  size_t dst_size = 0;

  while (len >= 4) {
    size_t write_bytes = 3;
    if (src[3] == '=')
      write_bytes = 2;
    if (src[2] == '=')
      write_bytes = 1;
    dst_size += write_bytes;

    dst[0] = lookupB64(src[0]) << 2;
    dst[0] |= lookupB64(src[1]) >> 4;

    if (write_bytes > 1) {
      dst[1] = lookupB64(src[1]) << 4;
      dst[1] |= lookupB64(src[2]) >> 2;
    }
    if (write_bytes > 2) {
      dst[2] = lookupB64(src[2]) << 6;
      dst[2] |= lookupB64(src[3]);
    }

    dst += 3;
    src += 4;
    len -= 4;
  }

  return dst_size;
}
