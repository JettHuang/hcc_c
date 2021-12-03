// \brief
//		libcc vprintf
//

#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

#include "lexer/token.h"
#include "parser/types.h"
#include "parser/symbols.h"


static char* outs(const char* str, FILE* f, char* bp) {
	if (f)
		fputs(str, f);
	else
		while (*bp = *str++)
			bp++;
	return bp;
}

static char* outd(long n, FILE* f, char* bp) {
	unsigned long m;
	char buf[25], * s = buf + sizeof buf;

	*--s = '\0';
	if (n < 0)
		m = -n;
	else
		m = n;
	do
		*--s = m % 10 + '0';
	while ((m /= 10) != 0);
	if (n < 0)
		*--s = '-';
	return outs(s, f, bp);
}

static char* outu(unsigned long n, int base, FILE* f, char* bp) {
	char buf[25], * s = buf + sizeof buf;

	*--s = '\0';
	do
		*--s = "0123456789abcdef"[n % base];
	while ((n /= base) != 0);
	return outs(s, f, bp);
}

static void outtype(FCCType* ty, FILE* f, char* bp, BOOL bdetail)
{
	if (!ty)
	{
		outs("Type NULL", f, bp);
		return;
	}

	switch (ty->_op)
	{
	case Type_Unknown:
		outs("unknown", f, bp); break;
	case Type_SInteger:
		if (ty->_size == 1) {
			outs("char", f, bp);
		}
		else {
			outs("sint", f, bp);
		}
		break;
	case Type_UInteger:
		if (ty->_size == 1) {
			outs("unsigned char", f, bp);
		}
		else {
			outs("uint", f, bp);
		}
		break;
	case Type_Float:
		if (ty->_size == sizeof(float)) {
			outs("float", f, bp);
		}
		else {
			outs("double", f, bp);
		}
		break;
	case Type_Void:
		outs("void", f, bp); break;
	case Type_Array:
	{
		outs("array[", f, bp);
		outd(ty->_size / ty->_type->_size, f, bp);
		outs(":", f, bp);
		outtype(ty->_type, f, bp, bdetail);
		outs("]", f, bp);
	}
	break;
	case Type_Enum:
	{
		FCCSymbol** enumid;
		outs("enum ", f, bp);
		outs(ty->_u._symbol->_name, f, bp);
		if (bdetail && ty->_u._symbol->_defined) {
			outs("<", f, bp);
			for (enumid = ty->_u._symbol->_u._enumids; *enumid; enumid++)
			{
				outs((*enumid)->_name, f, bp);
				outs("=", f, bp);
				outd((long)(*enumid)->_u._cnstval._sint, f, bp);
				outs(" ", f, bp);
			}
			outs(">", f, bp);
		}
	}
	break;
	case Type_Union:
	case Type_Struct:
	{
		outs(ty->_op == Type_Union ? "union " : "struct ", f, bp);
		outs(ty->_u._symbol->_name, f, bp);
		if (bdetail)
		{
			FCCField* field;

			outs("<align:", f, bp);
			outd(ty->_align, f, bp);
			outs(", size:", f, bp);
			outd(ty->_size, f, bp);
			outs(" members:\n", f, bp);
			for (field = cc_type_fields(ty); field; field=field->_next)
			{
				outs(field->_name, f, bp);
				outs(", offset:", f, bp);
				outd(field->_offset, f, bp);
				outs(", type:", f, bp);
				outtype(field->_type, f, bp, FALSE);
				if (field->_lsb > 0)
				{
					outs("\nlsb: ", f, bp);
					outd(field->_lsb, f, bp);
					outs(", bits:", f, bp);
					outd(field->_bitsize, f, bp);
				}
				outs("\n", f, bp);
			} // end for k
			outs(">\n", f, bp);
		}
	}
	break;
	case Type_Function:
	{
		int k;

		outs("function<", f, bp);
		outtype(cc_type_rettype(ty), f, bp, FALSE);
		outs("(", f, bp);
		for (k = 0; ty->_u._f._protos[k]; ++k)
		{
			if (k != 0)
			{
				outs(", ", f, bp);
			}
			outtype(ty->_u._f._protos[k], f, bp, FALSE);
		} // end for k
		outs(")>", f, bp);
	}
	break;
	case Type_Pointer:
	{
		outs("ptr<", f, bp);
		outtype(ty->_type, f, bp, bdetail);
		outs(">", f, bp);
	}
	break;
	default:
	{
		if (IsQual(ty))
		{
			if (ty->_op & Type_Const)
			{
				outs("const ", f, bp);
			}
			if (ty->_op & Type_Restrict)
			{
				outs("restrict ", f, bp);
			}
			if (ty->_op & Type_Volatile)
			{
				outs("volatile ", f, bp);
			}

			outtype(ty->_type, f, bp, bdetail);
		}
		else
		{
			outs("??", f, bp);
		}
	}
	break;
	}
}


/* vfprint - formatted output to f or string bp */
static void vfprint(FILE* f, char* bp, const char* fmt, va_list ap) {
	for (; *fmt; fmt++)
		if (*fmt == '%')
			switch (*++fmt) {
			case 'd': bp = outd(va_arg(ap, int), f, bp); break;
			case 'D': bp = outd(va_arg(ap, long), f, bp); break;
			case 'U': bp = outu(va_arg(ap, unsigned long), 10, f, bp); break;
			case 'u': bp = outu(va_arg(ap, unsigned), 10, f, bp); break;
			case 'o': bp = outu(va_arg(ap, unsigned), 8, f, bp); break;
			case 'X': bp = outu(va_arg(ap, unsigned long), 16, f, bp); break;
			case 'x': bp = outu(va_arg(ap, unsigned), 16, f, bp); break;
			case 'f': case 'e':
			case 'g': {
				static char format[] = "%f";
				char buf[128];
				format[1] = *fmt;
				sprintf(buf, format, va_arg(ap, double));
				bp = outs(buf, f, bp);
			}
					; break;
			case 's': bp = outs(va_arg(ap, char*), f, bp); break;
			case 'p': {
				void* p = va_arg(ap, void*);
				if (p)
					bp = outs("0x", f, bp);
				bp = outu((unsigned long)p, 16, f, bp);
				break;
			}
			case 'c': if (f) fputc(va_arg(ap, int), f); else *bp++ = va_arg(ap, int); break;
			case 'S': { char* s = va_arg(ap, char*);
				int n = va_arg(ap, int);
				if (s)
					for (; n-- > 0; s++)
						if (f) (void)putc(*s, f); else *bp++ = *s;
			} break;
			case 'k':  // token
			{ 
				enum ECCToken tk = va_arg(ap, enum ECCToken);
				outs(gCCTokenMetas[tk]._text, f, bp);
			} 
				break;
			case 't': /* type brief */
			{ 
				FCCType* ty = va_arg(ap, FCCType*);
				assert(ty);
				outtype(ty, f, bp, FALSE);
			} 
				break;
			case 'T': /* type detail */
			{
				FCCType* ty = va_arg(ap, FCCType*);
				assert(ty);
				outtype(ty, f, bp, TRUE);
			}
				break;
			case 'w': 
			{ 
				FLocation* loc = va_arg(ap, FLocation*);
				if (loc) {
					bp = outs(loc->_filename ? loc->_filename : "", f, bp);
					bp = outs(":", f, bp);
					bp = outd(loc->_line, f, bp);
					bp = outs(": ", f, bp);
					bp = outd(loc->_col, f, bp);
				}
				else {
					bp = outs("N/A", f, bp);
				}
			}
				break;
			case 'I': { int n = va_arg(ap, int);
				while (--n >= 0)
					if (f) (void)putc(' ', f); else *bp++ = ' ';
			} break;
			default:  if (f) (void)putc(*fmt, f); else *bp++ = *fmt; break;
			}
		else if (f)
			(void)putc(*fmt, f);
		else
			*bp++ = *fmt;
	if (!f)
		*bp = '\0';
}

void cc_logger_outc(int c)
{
	fputc(c, stdout);
}

void cc_logger_outs(const char* format, va_list arg)
{
	vfprint(stdout, NULL, format, arg);
}

const char* cc_sclass_displayname(int sclass)
{
	const char* name;

	switch (sclass)
	{
	case SC_Auto: name = "auto"; break;
	case SC_Register: name = "register"; break;
	case SC_Static: name = "static"; break;
	case SC_External: name = "external"; break;
	case SC_Typedef: name = "typedef"; break;
	case SC_Enum: name = "enum"; break;
	default: name = "unknown"; break;
	}

	return name;
}
