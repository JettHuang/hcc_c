/* \brief
 *		logger interface
 */

#ifndef __LOGGER_H__
#define __LOGGER_H__

#include <stdarg.h>


void logger_set(void (*outc)(int), void (*outs)(const char*, va_list));
void logger_output_c(int c);
void logger_output_s(const char* format, ...);

#endif /* __LOGGER_H__ */

