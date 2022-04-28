/* \brief
 *		char stream abstract of FILE or string.
 */

#ifndef __CHAR_STREAM_H__
#define __CHAR_STREAM_H__

#include <stdio.h>
#include <stdint.h>
#include <assert.h>


#define MAX_LOOKAHEAD_CNT		3
#define FILE_BUFSIZE			2048

enum EStreamType
{
	EST_STRING = 0,
	EST_FILE = 1
};

typedef struct tagCharStream
{
	enum EStreamType _streamtype;
	union {
		struct {
			FILE* _fp;
			char _buf[FILE_BUFSIZE];
			int  _charcnt;
			int  _cursor;
		} _file;
		
		const char* _str;
	} _charsource;

	const char* _srcfilename;
	unsigned int _line;
	unsigned int _col;

	/* cache buffer */
	char	_ringbuf[MAX_LOOKAHEAD_CNT];
	unsigned int _ringhead;
	unsigned int _cachedcnt;
} FCharStream;

/* create char stream */
FCharStream* cs_create_fromstring(const char* str);
FCharStream* cs_create_fromfile(const char* filename);
void cs_release(FCharStream* cs);

char cs_current(FCharStream* cs);
char cs_lookahead(FCharStream* cs, unsigned int k);
void cs_forward(FCharStream* cs);
void cs_replace_current(FCharStream* cs, char ch);


#endif /* __CHAR_STREAM_H__ */
