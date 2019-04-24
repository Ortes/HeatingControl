#include <string.h>

#include "utils.h"

inline int ishex(int x)
{
  return(x >= '0' && x <= '9')||
    (x >= 'a' && x <= 'f')||
    (x >= 'A' && x <= 'F');
}

int decode(const char *s, char *dec)
{
  char *o;
  const char *end = s + strlen(s);
  int c;

  for (o = dec; s <= end; o++) {
    c = *s++;
    if (c == '+') c = ' ';
    else if (c == '%' && (!ishex(*s++)||
                          !ishex(*s++)||
                          !sscanf(s - 2, "%2x", &c)))
      return -1;

    if (dec) *o = c;
  }

  return o - dec;
}

esp_err_t get_value(const char* src, const char* key, char* dest, size_t size)
{
  size_t key_length = strlen(key);
  char token[key_length + 2];
  strcpy(token, key);
  token[key_length] = '=';
  token[key_length + 1] = '\0';
  char* ptr = strstr(src, token);
  if (!ptr)
    return ESP_ERR_NOT_FOUND;
  ptr += key_length + 1;
  char* end = strchr(ptr, '&');
  if (end) {
    size_t len = end - ptr > size ? size : end - ptr;
    strncpy(dest, ptr, len);
    dest[len == size ? size - 1 : len] = '\0';
  }
  else
    strncpy(dest, ptr, size);
  dest[size - 1] = '\0';
  return ESP_OK;
}
