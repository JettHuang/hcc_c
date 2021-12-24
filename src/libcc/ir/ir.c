/* \brief
 *		intermediate representation.
 */

#include "ir.h"
#include "parser/types.h"
#include "parser/symbols.h"


int cc_ir_typecode(const struct tagCCType* ty)
{
	ty = UnQual(ty);

	switch (ty->_op)
	{
	case Type_SInteger:
		switch (ty->_size)
		{
		case 1: return IR_S8;
		case 2: return IR_S16;
		case 4: return IR_S32;
		case 8: return IR_S64;
		}
		break;
	case Type_UInteger:
		switch (ty->_size)
		{
		case 1: return IR_U8;
		case 2: return IR_U16;
		case 4: return IR_U32;
		case 8: return IR_U64;
		}
		break;
	case Type_Float:
		switch (ty->_size)
		{
		case 4: return IR_F32;
		case 8: return IR_F64;
		}
		break;
	case Type_Enum:
		return cc_ir_typecode(ty->_type);
	case Type_Void:
		return IR_VOID;
	case Type_Pointer:
	case Type_Array:
		return IR_PTR;
	case Type_Union:
	case Type_Struct:
		return IR_BLK;
	case Type_Function:
		return IR_PTR;
	}

	return IR_VOID;
}
