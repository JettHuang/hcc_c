/* \brief
 *		logger interface
 */

#ifndef __LOGGER_H__
#define __LOGGER_H__


void logger_set(void (*outc)(int), void (*outs)(const char*));
void logger_output_c(int c);
void logger_output_s(const char* str);

#endif /* __LOGGER_H__ */

