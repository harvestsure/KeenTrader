//
// Created by zxp on 17/07/18.
//
#pragma once

#include <ctime>
#include <string>
#include <map>

char * GetTimestamp(char *timestamp, int len);
AString JsonFormat(AString jsonStr);
unsigned int str_hex(unsigned char *str,unsigned char *hex);
void hex_str(unsigned char *inchar, unsigned int len, unsigned char *outtxt);
AString Rjust(const AString& str, size_t width, const char fillchar);
