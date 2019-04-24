#ifndef __HTTPD_UTILS_H__
#define __HTTPD_UTILS_H__

#include "esp_err.h"

int decode(const char *s, char *dec);
esp_err_t get_value(const char* src, const char* key, char* dest, size_t size);

#endif
