/*
  utils.h
  Author: Straight Coder<simpleisrobust@gmail.com>
  Date: April 12, 2020
*/

#ifndef _UTILS_H_
#define _UTILS_H_

long ston(u8_t* s);
void ntos(long n, u8_t* s);
int ToLower(int nChar);
int ToUpper(int nChar);
int token_match(const char *s1, const char *s2);
int Strnicmp(char *str1, char *str2, int n);
void TrimFloat(char* floatString);
int DecodeB64(unsigned char* data);
int URLDecode(char* url);

#endif
