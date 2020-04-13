/*
  utils.c
  Author: Straight Coder<simpleisrobust@gmail.com>
  Date: April 12, 2020
*/

#include <stdlib.h>
#include <stdio.h>

#include <windows.h>

#include <lwip/arch.h>

#include "utils.h"

long ston(u8_t* s)
{
	long sign = +1;

	long result = 0;
	char* p = (char*)s;

	if ((p == 0) || (*p == 0))
		return result;

	while ((*p != 0) && ((*p == ' ') || (*p == '\t')))
	{
		p++;
	}

	if (p[0] == '-')
	{
		p++;
		sign = -1;
	}

	if (p[0] == '+')
	{
		p++;
	}

	while ((*p != 0) && (*p >= '0') && (*p <= '9'))
	{
		result *= 10;
		result += (*p - '0');
		p++;
	}

	return (sign*result);
}

void ntos(long n, u8_t* s)
{
	const u8_t hex_alpha[16] = "0123456789";
	u32_t i, first, div, remain;

	i = 0;
	if (n == 0)
	{
		s[i++] = '0';
		s[i] = 0;
		return;
	}

	if (n < 0)
	{
		s[i++] = '-';
		n *= (-1);
	}

	first = 1;
	div = 1000000000;
	while (1)
	{
		remain = n / div;
		if (remain != 0)
		{
			n -= (div*remain);
			first = 0;
		}
		if (first == 0)
		{
			s[i++] = hex_alpha[remain];
			s[i] = 0;
		}
		if (div == 1)
			break;
		div /= 10;
	}
}

int ToLower(int nChar)
{
	unsigned char cTemp = (unsigned char)nChar;
	if ((cTemp >= 'A') && (cTemp <= 'Z'))
		cTemp |= 0x20;
	return cTemp;
}

int ToUpper(int nChar)
{
	unsigned char cTemp = (unsigned char)nChar;
	if ((cTemp >= 'a') && (cTemp <= 'z'))
		cTemp &= 0xDF;
	return cTemp;
}

int token_match(const char *s1, const char *s2)
{
	int c1;
	int c2;
	int end1 = 0;
	int end2 = 0;
	for (;;)
	{
		c1 = *s1++;
		c2 = *s2++;

		if (isalpha(c1) && isupper(c1)) //alphabet 
			c1 = tolower(c1);
		if (isalpha(c2) && isupper(c2)) //alphabet 
			c2 = tolower(c2);

		end1 = !(isalnum(c1) || (c1 == '_') || (c1 == '-')); //not (alphabet and number)
		end2 = !(isalnum(c2) || (c2 == '_') || (c2 == '-')); //not (alphabet and number)

		if (end1 || end2 || (c1 != c2))
			break;
	}
	return end1 && end2;
}

int Strnicmp(char *str1, char *str2, int n)
{
	while (*str1 && *str2 && n)
	{
		int res = ToUpper((unsigned)*str1) - ToUpper((unsigned)*str2);
		if (res)
			return res < 0 ? -1 : 1;
		str1++;
		str2++;
		n--;
	}

	if (n == 0)		return 0;
	if (*str1) 		return 1;
	if (*str2) 		return -1;

	return 0;
}
