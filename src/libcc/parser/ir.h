/* \brief
 *		intermediate representation
 */

#ifndef __CC_IR_H__
#define __CC_IR_H__

#include "utils.h"
#include "mm.h"
#include "cc.h"


/* types code */
#define IR_S8		0x01
#define IR_U8		0x02
#define IR_S16		0x03
#define IR_U16		0x04
#define IR_S32		0x05
#define IR_U32		0x06
#define IR_S64		0x07
#define IR_U64		0x08
#define IR_F32		0x09
#define IR_F64		0x0A
#define IR_PTR		0x0B	/* pointer */
#define IR_BLK		0x0C	/* structure */
#define IR_STRA		0x0D	/* multi bytes string */
#define IR_STRW		0x0E	/* wide char string */
#define IR_VOID		0x0F

/* op-codes : expression */
#define IR_CONST	0x01
#define IR_ASSIGN	0x02
#define IR_ADD		0x03
#define IR_SUB		0x04
#define IR_MUL		0x05
#define IR_DIV		0x06
#define IR_MOD		0x07
#define IR_LSHIFT	0x08	/* << */
#define IR_RSHIFT	0x09	/* >> */
#define IR_BITAND	0x0A	/* &  */
#define IR_BITOR	0x0B	/* |  */
#define IR_BITXOR	0x0C	/* ^  */
#define IR_LOGAND	0X0D	/* && */
#define IR_LOGOR	0x0E	/* || */
#define IR_EQUAL	0x0F	/* == */
#define IR_UNEQ		0x10	/* != */
#define IR_LESS		0x11	/* <  */
#define IR_GREAT	0x12	/* >  */
#define IR_LEQ		0x13	/* <= */
#define IR_GEQ		0x14	/* >= */
#define IR_NEG		0x15	/* -  */
#define IR_NOT		0x16	/* !  */
#define IR_BCOM		0x17	/* ~  */
#define IR_CVT		0x18	/* type convert */
#define IR_INDIR	0x19	/* indirect visit */
#define IR_ADDRG	0x1A	/* address global object */
#define IR_ADDRF	0x1B	/* address param object */
#define IR_ADDRL	0x1C	/* address local object */
#define IR_CALL		0x1D	/* function invoke */
#define IR_SEQ		0x1E	/* sequence, e1, e2, e3, and final result is e3 */
#define IR_COND		0x1F	/* ? : */

/* op-codes : code */
#define IR_JMP		0x20


/* format: op | ty0 | ty1 */
#define IR_MKOP(op)				(op)
#define IR_MKOP1(op, ty)		(((op) << 16) | ((ty) << 8))
#define IR_MKOP2(op, ty0, ty1)	(((op) << 16) | ((ty0) << 8) | (ty1))
#define IR_OP(op)				((op) >> 16)
#define IR_OPTY0(op)			(((op) >> 8) & 0x00FF)
#define IR_OPTY1(op)			((op) & 0x00FF)

#define IsAddrOp(op)	((IR_OP(op) == IR_ADDRG) || (IR_OP(op) == IR_ADDRF) || (IR_OP(op) == IR_ADDRL))


 /* expression IR tree */
typedef struct tagCCExprTree {
	unsigned int _op;
	FLocation	 _loc;
	struct tagCCType* _ty; /* the derivation type of this expression */

	union {
		struct tagCCExprTree* _kids[3];
		struct tagCCSymbol* _symbol;
		struct tagStructField* _field;
		struct {
			struct tagCCExprTree* _lhs;
			struct tagCCExprTree** _args; /* end with null */
		} _f;
		FCCConstVal _val;
	} _u;

	/* auxiliary flags */
	int _isbitfield : 1;
	int _islvalue : 1;
} FCCIRTree;


int cc_ir_typecode(const struct tagCCType* ty);


#endif /* __CC_IR_H__ */
