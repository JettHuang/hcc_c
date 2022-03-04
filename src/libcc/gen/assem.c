/* \brief
 *		abstract assembly code.
 *
 */

#include "assem.h"
#include "mm.h"
#include "logger.h"


FCCASCode* cc_as_newcode(enum EMMArea where)
{
	FCCASCode* code = mm_alloc_area(sizeof(FCCASCode), where);
	if (!code) {
		logger_output_s("error: out of memory at %s:%d\n", __FILE__, __LINE__);
		return NULL;
	}

	util_memset(code, 0, sizeof(FCCASCode));
	return code;
}

void cc_as_codelist_append(FCCASCodeList* l, FCCASCode* c)
{
	if (l->_tail) {
		c->_prev = l->_tail;
		l->_tail->_next = c;
	}
	else {
		l->_head = c;
	}

	l->_tail = c;
}

FCCASCode* cc_as_codelist_remove(FCCASCodeList* l, FCCASCode* c)
{
	FCCASCode* next = NULL;

	if (c->_prev) {
		c->_prev->_next = c->_next;
	}
	else {
		assert(l->_head == c);
		l->_head = c->_next;
	}

	if (c->_next) {
		c->_next->_prev = c->_prev;
	}
	else {
		assert(l->_tail == c);
		l->_tail = c->_prev;
	}

	next = c->_next;
	c->_prev = c->_next = NULL;
	return next;
}
