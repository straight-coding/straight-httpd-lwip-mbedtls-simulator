/*
  utils.h
  Author: Straight Coder<simpleisrobust@gmail.com>
  Date: April 12, 2020
*/

#ifndef _UTILS_H_
#define _UTILS_H_

#pragma warning(disable:4996) //_CRT_SECURE_NO_WARNINGS

typedef struct
{
	char *extension;
	char *content_type;
}TypeHeader;

long ston(unsigned char* s);
void ntos(long n, unsigned char* s);
int ToLower(int nChar);
int ToUpper(int nChar);
int token_match(const char *s1, const char *s2);
int Strnicmp(char *str1, char *str2, int n);
void TrimFloat(char* floatString);
int DecodeB64(unsigned char* data);
int URLDecode(char* url);
void MakeDeepPath(char* szPath);

char* GetContentType(const char* extension);

#endif
