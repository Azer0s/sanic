#include <ctype.h>

#include "../include/internal/string_util.h"

char *str_uppercase(char *input, int len) {
  for(int i = 0; i < len; i++) {
    input[i] = toupper(input[i]); // NOLINT(cppcoreguidelines-narrowing-conversions)
  }
  return input;
}
