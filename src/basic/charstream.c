/* \brief
 *		char stream implementation.
 */

#include "charstream.h"
#include "mm.h"
#include "logger.h"
#include "hstring.h"


 /* create char stream */
FCharStream* cs_create_fromstring(const char* str)
{
	FCharStream* cs = NULL;

	assert(str);
	cs = (FCharStream*)mm_alloc(sizeof(FCharStream));
	if (!cs) { return NULL; }

	cs->_streamtype = EST_STRING;
	cs->_charsource._str = str;
	cs->_srcfilename = NULL;
	cs->_line = 1;
	cs->_col = 1;
	cs->_ringhead = 0;
	cs->_cachedcnt = 0;
	return cs;
}

FCharStream* cs_create_fromfile(const char* filename)
{
	FCharStream* cs = NULL;
	
	assert(filename);
	FILE* fp = fopen(filename, "rt");
	if (!fp) {
		logger_output_s("open source file failed. %s\n", filename);
		return NULL;
	}

	cs = (FCharStream*)mm_alloc(sizeof(FCharStream));
	if (!cs) { 
		fclose(fp);
		return NULL; 
	}

	cs->_streamtype = EST_FILE;
	cs->_charsource._file = fp;
	cs->_srcfilename = hs_hashstr(filename);
	cs->_line = 1;
	cs->_col = 1;
	cs->_ringhead = 0;
	cs->_cachedcnt = 0;
	return cs;
}

void cs_release(FCharStream* cs)
{
	assert(cs);

	if (cs->_streamtype == EST_FILE)
	{
		if (cs->_charsource._file) {
			fclose(cs->_charsource._file);
		}
	}

	mm_free(cs);
}

static unsigned int ring_buffer_index(unsigned int rh, unsigned int k)
{
	return (rh + k) % MAX_LOOKAHEAD_CNT;
}

static char read_char(FCharStream* cs)
{
	char ch = EOF;

	if (cs->_streamtype == EST_FILE)
	{
		if (cs->_charsource._file) {
			ch = fgetc(cs->_charsource._file);
		}
	}
	else 
	{
		const char* ptrchar = cs->_charsource._str;
		if (ptrchar && *ptrchar)
		{
			ch = *ptrchar;
			cs->_charsource._str++;
		}
	}

	return ch;
}

char cs_current(FCharStream* cs)
{
	return cs_lookahead(cs, 0);
}

char cs_lookahead(FCharStream* cs, unsigned int k)
{
	unsigned int idx;

	assert(k < MAX_LOOKAHEAD_CNT);
	while (k >= cs->_cachedcnt)
	{
		idx = ring_buffer_index(cs->_ringhead, cs->_cachedcnt);
		cs->_ringbuf[idx] = read_char(cs);
	
		cs->_cachedcnt++;
	} /* end while */
	
	idx = ring_buffer_index(cs->_ringhead, k);
	return cs->_ringbuf[idx];
}

void cs_forward(FCharStream* cs)
{
	char ch = cs_current(cs);
	if (ch == '\n') {
		cs->_line++;
		cs->_col = 1;
	} else if (ch == '\t') {
		cs->_col += (8 - cs->_col % 8);
	} else {
		cs->_col++;
	}

	assert(cs->_cachedcnt > 0);
	if (cs->_cachedcnt > 0)
	{
		cs->_ringhead = (cs->_ringhead + 1) % MAX_LOOKAHEAD_CNT;
		cs->_cachedcnt--;
	}
}

void cs_replace_current(FCharStream* cs, char ch)
{
	cs_current(cs);
	cs->_ringbuf[cs->_ringhead] = ch;
}

