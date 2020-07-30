/*
  utils.c
  Author: Straight Coder<simpleisrobust@gmail.com>
  Date: April 12, 2020
*/

#include <stdlib.h>
#include <stdio.h>

#include "utils.h"

static const TypeHeader contentTypes[] = {
	{ "html",  "text/html"},
	{ "htm",   "text/html"},

	{ "shtml", "text/html"},
	{ "shtm",  "text/html"},
	{ "ssi",   "text/html"},
	{ "css",   "text/css"},

	{ "json",  "application/json"},
	{ "js",    "application/javascript"},

	{ "gif",   "image/gif"},
	{ "png",   "image/png"},
	{ "jpg",   "image/jpeg"},
	{ "bmp",   "image/bmp"},
	{ "ico",   "image/x-icon"},

	{ "mp4",   "video/mp4"},
	{ "flv",   "video/x-flv"},
	{ "3gp",   "video/3gpp"},
	{ "mov",   "video/quicktime"},
	{ "avi",   "video/x-msvideo"},
	{ "wmv",   "video/x-ms-wmv"},

	{ "class", "application/octet-stream"},
	{ "cls",   "application/octet-stream"},
	{ "swf",   "application/x-shockwave-flash"},
	{ "ram",   "application/javascript"},
	{ "pdf",   "application/pdf"},

	{ "xml",   "text/xml"},
	{ "xsl",   "text/xml"},

	{ "",    "text/plain"}
};

long ston(unsigned char* s)
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

void ntos(long n, unsigned char* s)
{
	const unsigned char hex_alpha[16] = "0123456789";
	unsigned char i, first, div, remain;

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

void TrimFloat(char* floatString)
{
	int len = strlen(floatString);
	if (strchr(floatString, '.') <= 0)
		return;
	while (len > 0)
	{
		if (floatString[len - 1] != '0')
		{
			if (floatString[len - 1] == '.')
				floatString[len - 1] = 0;
			break;
		}
		floatString[len - 1] = 0;
		len--;
	}
}

char* base64Chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
int DecodeB64(unsigned char* data)
{
	int i = 0;
	int resultCount = 0;
	int validCount = 0;
	int value = 0;
	unsigned long convered = 0;
	while (data[i] != 0)
	{
		if ((data[i] == '\r') || (data[i] == '\n') || (data[i] == '='))
		{ //ingnored
			i++;
			continue;
		}
		if ((data[i] >= 'A') && (data[i] <= 'Z'))
		{
			value = (int)(data[i] - 'A');
		}
		else if ((data[i] >= 'a') && (data[i] <= 'z'))
		{
			value = (int)(data[i] - 'a' + 26);
		}
		else if ((data[i] >= '0') && (data[i] <= '9'))
		{
			value = (int)(data[i] - '0' + 52);
		}
		else if (data[i] == '+')
		{
			value = 62;
		}
		else if (data[i] >= '/')
		{
			value = 63;
		}
		else
		{ //bad character
			return 0;
		}

		i++;
		validCount++;

		convered <<= 6;
		convered += value;

		if ((validCount & 0x3) == 0)
		{
			data[resultCount+0] = (unsigned char)((convered >> 16) & 0xFF);
			data[resultCount+1] = (unsigned char)((convered >> 8) & 0xFF);
			data[resultCount+2] = (unsigned char)((convered) & 0xFF);

			resultCount += 3;
			convered = 0;
		}
	}
	if ((validCount & 0x3) == 3)
	{
		data[resultCount + 0] = (unsigned char)((convered >> 10) & 0xFF);
		data[resultCount + 1] = (unsigned char)((convered >> 2) & 0xFF);
		resultCount += 2;
	}
	else if ((validCount & 0x3) == 2)
	{
		data[resultCount] = (unsigned char)((convered >> 4) & 0xFF);
		resultCount ++;
	}
	else if ((validCount & 0x3) == 1)
	{
	}
	data[resultCount] = 0;
	return resultCount;
}

int Hex2Int(char c)
{
	char* set = "00112233445566778899AaBbCcDdEeFf";
	int i = 0;
	while (set[i] != 0)
	{
		if (c == set[i])
			return (i>>1);
		i++;
	}
	return -1;
}

int URLDecode(char* url)
{
	int i = 0;
	int len = strlen(url);

	while((i < len) && (url[i] != 0))
	{
		if (url[i] != '%')
		{
			i++;
			continue;
		}

		if (i < len - 2)
		{
			int a = Hex2Int(url[i + 1]);
			int b = Hex2Int(url[i + 2]);
			if ((a >= 0) && (b >= 0))
			{
				a = (a << 4) + b;
				url[i] = a;
				memmove(url + i + 1, url + i + 3, len - i - 3);
				len -= 2;
				i++;
				continue;
			}
		}

		i++;
	}
	return i;
}

void MakeDeepPath(char* szPath)
{
	char	*p1, *p2, chTmp;
	char	szTemp[256];

	memset(szTemp, 0, sizeof(szTemp));
	strncpy(szTemp, szPath, sizeof(szTemp)-1);

	p1 = szTemp;
	while (*p1 != 0)
	{
		if (*p1 == '\\')
			*p1 = '/';
		p1++;
	}

	p1 = szTemp;
	while (1)
	{
		p2 = strchr(p1, '/');
		if (p2 == NULL)
			break;
		chTmp = p2[0];
		p2[0] = 0;
		_mkdir(szTemp);
		p2[0] = chTmp;
		p1 = p2 + 1;
	}
}

char* GetContentType(const char* extension)
{
	int i = 0;
	int extLen = strlen(extension);
	while (1)
	{
		int typeLen = strlen(contentTypes[i].extension);
		if (typeLen == 0)
			return contentTypes[i].content_type;

		if (typeLen == extLen)
		{
			if (Strnicmp(extension, contentTypes[i].extension, typeLen) == 0)
				return contentTypes[i].content_type;
		}
		i++;
	}
}

unsigned long GetIpAddress(char* addr)
{
	unsigned char* p = addr;
	unsigned long ip = (unsigned char)ston(p);

	p = strchr(p, '.');
	if (p != NULL)
	{
		p ++; ip <<= 8; ip += ston(p);

		p = strchr(p, '.');
		if (p != NULL)
		{
			p ++; ip <<= 8; ip += ston(p);

			p = strchr(p, '.');
			if (p != NULL)
			{
				p++; ip <<= 8; ip += ston(p);
			}
		}
	}
	return ip;
}
