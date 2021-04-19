/* \brief
		utility functions
 */

#include "utils.h"


unsigned int util_str_hash(const char* str, unsigned int len)
{
	unsigned int h = 0;
	while (len-- > 0)
	{
		h = h * 13131 + (unsigned int)*str++;
	}

	return h;
}

char util_char_lower(char c)
{
	return (unsigned short)c - (((unsigned short)c - 'a') < 26u) << 5;
}

char util_char_upper(char c)
{
	return (unsigned short)c + (((unsigned short)c - 'A') < 26u) << 5;
}
