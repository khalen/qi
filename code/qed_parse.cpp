//
// Created by Jonathan Davis on 6/5/20.
//

#include "basictypes.h"
#include "game.h"
#include <stdarg.h>
#include <stdlib.h>
#include <setjmp.h>
#include <cstdio>
#include <ctype.h>

#include "util.h"
#include "keystore.h"
#include "memory.h"
#include "debug.h"
#include "stringtable.h"
#include "qed_parse.h"

#define PEEK(n) (gpc.s + n < gpc.end ? gpc.s[n] : 0)
#define NEXT()  PEEK(0)
#define SKIP()               \
	do                       \
	{                        \
		if (gpc.s < gpc.end) \
			gpc.s++;         \
	} while (0)

template<typename T, i32 InitialSize = 128>
struct PushBuffer
{
	bool hasGrown;
	u32  size;
	u32  used;
	T *  curPtr;
	T    buffer[InitialSize];
};

typedef PushBuffer<char>     CharBuffer;
typedef PushBuffer<ValueRef> ValueBuffer;

struct ParseContext
{
	jmp_buf     escBuf;
	u32         line;
	KeyStore ** ksp;
	bool        ownsKs;
	const char *s, *end;
	char        errorBuf[80];
} gpc;

static void     P_SkipComment();
static void     P_SkipSpace();
static bool     P_SkipToken(const char *tok);
static void     P_Optional(const char *tok);
static ValueRef P_ParseNumber();
static ValueRef P_ParseString();
static ValueRef P_ParseValue();
static ValueRef P_ParseArray();
static ValueRef P_ParseObject(bool topLevel);

bool P_CanStartSymbol(char c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c == '_');
}

bool P_CanContinueSymbol(char c)
{
	return P_CanStartSymbol(c) || (c >= '0' && c <= '9');
}

bool P_IsNonASCII(char c)
{
	return !isprint(c);
}

template<typename T, i32 InitialSize = 128>
void PB_Init(PushBuffer<T, InitialSize> *pb)
{
	pb->hasGrown = false;
	pb->size     = InitialSize;
	pb->curPtr   = pb->buffer;
	pb->used     = 0;
}
template<typename T, i32 InitialSize = 128>
void PB_Destroy(PushBuffer<T, InitialSize> *pb)
{
	if (pb->hasGrown)
	{
		BA_Free(KS_GetKeyStoreAllocator(), pb->curPtr);
	}
}
template<typename T, i32 InitialSize = 128>
void PB_Grow(PushBuffer<T, InitialSize> *pb)
{
	pb->size *= 2;
	if (!pb->hasGrown)
	{
		pb->curPtr = (T *)BA_Alloc(KS_GetKeyStoreAllocator(), pb->size * sizeof(T));
		memcpy(pb->curPtr, pb->buffer, InitialSize * sizeof(T));
		pb->hasGrown = true;
	}
	else
	{
		pb->curPtr = (T *)BA_Realloc(KS_GetKeyStoreAllocator(), pb->curPtr, pb->size * sizeof(T));
	}
}

template<typename T, i32 InitialSize = 128>
void PB_Push(PushBuffer<T, InitialSize> *pb, T value)
{
	if (pb->used == pb->size)
	{
		PB_Grow<T, InitialSize>(pb);
	}
	Assert(pb->used < pb->size);
	pb->curPtr[pb->used++] = value;
}

const char *P_ParseBuffer(KeyStore **ksp, const char *ksName, const char *buffer, size_t bufSize, ValueRef *refOut)
{
	if (*ksp == nullptr)
	{
		const char *name = ksName ? ksName : "Unnamed";
		*ksp             = KS_Create(name);
		gpc.ownsKs       = true;
	}
	else
	{
		gpc.ownsKs = false;
	}

	if (setjmp(gpc.escBuf))
	{
		if (gpc.ownsKs)
		{
			KS_Free(gpc.ksp);
			gpc.ownsKs = false;
		}
		*refOut = NilValue;
		return gpc.errorBuf;
	}

	gpc.ksp         = ksp;
	gpc.s           = buffer;
	gpc.end         = buffer + bufSize;
	gpc.errorBuf[0] = 0;

	ValueRef parsed = P_ParseObject(true);
	*refOut         = parsed;

	return nullptr;
}
static void
#if HAS(IS_CLANG)
	__attribute__((format(printf, 1, 2)))
#endif
	P_Error(const char *msg, ...)
{
	va_list args;
	va_start(args, msg);
	vsnprintf(gpc.errorBuf, sizeof(gpc.errorBuf), msg, args);
	va_end(args);
	longjmp(gpc.escBuf, 1);
}
static void P_SkipUntil(const char *tok)
{
	size_t tokLen = strlen(tok);
	if (tokLen == 0)
		return;
	while (gpc.s < gpc.end)
	{
		size_t i;
		for (i = 0; i < tokLen; i++)
		{
			if (PEEK(i) != tok[i])
				break;
		}
		if (i == tokLen)
			return;
		SKIP();
	}
	P_Error("End of buffer encountered looking for '%s'", tok);
}
static void P_SkipComment()
{
	char c = NEXT();
	SKIP();
	P_SkipUntil(c == '*' ? "*/" : "\n");
}
static void P_SkipSpace()
{
	while (gpc.s < gpc.end)
	{
		if (NEXT() == ' ' || NEXT() == '\t' || NEXT() == ',')
		{
			SKIP();
		}
		else if (NEXT() == '\n')
		{
			gpc.line++;
			SKIP();
			if (NEXT() == '\r')
				SKIP();
		}
		else if (NEXT() == '\r')
		{
			gpc.line++;
			SKIP();
			if (gpc.s < gpc.end && *gpc.s == '\n')
				gpc.s++;
		}
		else if (NEXT() == '/' && (PEEK(1) == '/' || PEEK(1) == '*'))
		{
			SKIP();
			P_SkipComment();
		}
		else
		{
			break;
		}
	}
}
static bool P_SkipToken(const char *tok)
{
	P_SkipSpace();
	const char *start = gpc.s;
	while (*tok)
	{
		if (*gpc.s++ != *tok++)
		{
			gpc.s = start;
			return false;
		}
	}
	return true;
}
static void P_Expect(const char *tok)
{
	if (!P_SkipToken(tok))
		P_Error("Expected '%s', got %c", tok, *gpc.s);
}
static void P_Optional(const char *tok)
{
	P_SkipToken(tok);
}
static ValueRef P_ParseNil()
{
	P_SkipToken("nil");
	return NilValue;
}
static ValueRef P_ParseFalse()
{
	P_SkipToken("false");
	return FalseValue;
}
static ValueRef P_ParseTrue()
{
	P_SkipToken("true");
	return TrueValue;
}
static ValueRef P_ParseSymbol()
{
	CharBuffer sym;

	PB_Init(&sym);
	if (NEXT() == '`')
	{
		SKIP();
		while (gpc.s < gpc.end && NEXT() != '`')
		{
			PB_Push(&sym, *gpc.s);
			gpc.s++;
		}
		P_SkipToken("`");
	}
	while (gpc.s < gpc.end && P_CanContinueSymbol(*gpc.s))
	{
		PB_Push(&sym, *gpc.s);
		gpc.s++;
	}

	PB_Push(&sym, (char)0);
	ValueRef ref = KS_AddSymbol(gpc.ksp, sym.curPtr);
	PB_Destroy(&sym);

	return ref;
}

static IntValue P_CharValue(IntValue base, char c)
{
	if (c >= 'a')
	{
		c = c - 'a';
	}
	else if (c >= 'A')
	{
		c = c - 'A';
	}
	else
	{
		c = c - '0';
	}
	if (c > base - 1)
	{
		P_Error("Bad character '%c' in base %lld integer constant", c, base);
	}
	return (IntValue)c;
}

static ValueRef P_ParseNumber()
{
	bool     isReal   = false;
	IntValue sign     = 1;
	IntValue intv     = 0;
	IntValue base     = 10;
	IntValue fracv    = 0;
	IntValue fracdivv = 1;
	IntValue expv     = 0;
	IntValue expsign  = 1;

	// Integer part
	char c = NEXT();
	// Color constant
	if (c == '#')
	{
		base = 16;
		SKIP();
		c = NEXT();
	}

	// Sign
	if (c == '-')
	{
		sign = -1;
		SKIP();
	}
	else if (c == '+')
	{
		SKIP();
	}
	// Optional leading 0s + detect hex or octal
	else if (c == '0')
	{
		SKIP();
		c = NEXT();
		if (c == 'x' || c == 'X')
		{
			base = 16;
		}
		else if (c == 'o' || c == 'O')
		{
			base = 8;
		}

		if (c == '0')
		{
			while (gpc.s < gpc.end && *gpc.s == '0')
				gpc.s++;
		}
	}

	// Number
	c = NEXT();
	while (gpc.s < gpc.end && ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f')))
	{
		intv = base * intv + P_CharValue(base, c);
		SKIP();
		c = NEXT();
	}

	if (base != 10)
	{
		goto done;
	}

	// Optional fractional part
	P_SkipSpace();
	c = NEXT();
	if (c == '.')
	{
		SKIP();
		P_SkipSpace();
		c      = NEXT();
		c      = NEXT();
		isReal = true;
		while (gpc.s < gpc.end && (c >= '0' && c <= '9'))
		{
			fracv = 10 * fracv + (c - '0');
			fracdivv *= 10;
			SKIP();
			c = NEXT();
		}
		P_SkipSpace();
		c = NEXT();
	}

	// Optional exponent
	if (c == 'e' || c == 'E')
	{
		SKIP();
		P_SkipSpace();
		c      = NEXT();
		isReal = true;

		// Exponent sign
		if (c == '+')
		{
			SKIP();
			c = NEXT();
		}
		else if (c == '-')
		{
			expsign = -1;
			SKIP();
			c = NEXT();
		}

		// Exponent
		while (gpc.s < gpc.end && (c >= '0' && c <= '9'))
		{
			expv = 10 * expv + (c - '0');
			SKIP();
			c = NEXT();
		}
	}

done:
	ValueRef ret;
	if (isReal)
	{
		RealValue value = (RealValue)sign * ((RealValue)intv + ((RealValue)fracv / (RealValue)fracdivv)) * pow(10.0, (RealValue)expsign * (RealValue)expv);
		ret             = KS_AddReal(gpc.ksp, value);
	}
	else
	{
		IntValue value = sign * intv;
		if (value >= SMALLEST_SMALLINT && value <= LARGEST_SMALLINT)
		{
			ret = KS_AddSmallInt(gpc.ksp, (SmallIntValue)value);
		}
		else
		{
			ret = KS_AddInt(gpc.ksp, value);
		}
	}

	return ret;
}
static ValueRef P_ParseVerbatimString()
{
	CharBuffer str;
	PB_Init(&str);

	// ParseString already skipped the first "
	P_SkipToken("\"\"");
	while (gpc.s < gpc.end)
	{
		if (PEEK(0) == '\"' && PEEK(1) == '\"' && PEEK(2) == '"')
			break;
		PB_Push(&str, *gpc.s);
		gpc.s++;
	}
	P_SkipToken("\"\"\"");
	PB_Push(&str, (char)0);

	ValueRef ref = KS_AddString(gpc.ksp, str.curPtr);
	PB_Destroy(&str);
	return ref;
}
static ValueRef P_ParseString()
{
	ValueRef ref = NilValue;

	P_SkipToken("\"");
	if (PEEK(0) == '\"' && PEEK(1) == '\"')
	{
		ref = P_ParseVerbatimString();
	}
	else
	{
		CharBuffer str;
		PB_Init(&str);
		while (gpc.s < gpc.end && *gpc.s != '\"')
		{
			char c = *gpc.s;
			if (c == '\\')
			{
				SKIP();
				c = *gpc.s;
				switch (c)
				{
				case '"':
				case '\\':
				case '/':
					PB_Push(&str, c);
					break;
				case 'b':
					PB_Push(&str, '\b');
					break;
				case 'r':
					PB_Push(&str, '\r');
					break;
				case 'n':
					PB_Push(&str, '\n');
					break;
				case 'f':
					PB_Push(&str, '\f');
					break;
				case 't':
					PB_Push(&str, '\t');
					break;
				default:
					P_Error("Unexpected character: '%c'", c);
					break;
				}
				SKIP();
			}
			PB_Push(&str, *gpc.s);
			SKIP();
		}
		P_SkipToken("\"");

		PB_Push(&str, (char)0);
		ref = KS_AddString(gpc.ksp, str.curPtr);
		PB_Destroy(&str);
	}

	return ref;
}

static ValueRef P_ParseValue()
{
	P_SkipSpace();
	char c = NEXT();
	if ((c >= '0' && c <= '9') || (c == '+' || c == '-' || c == '.') || c == '#')
		return P_ParseNumber();
	else if (c == 'n')
		return P_ParseNil();
	else if (c == 'f')
		return P_ParseFalse();
	else if (c == 't')
		return P_ParseTrue();
	else if (c == '\"')
		return P_ParseString();
	else if (P_CanStartSymbol(c) || c == '`')
		return P_ParseSymbol();
	else if (c == '[')
		return P_ParseArray();
	else if (c == '{')
		return P_ParseObject(false);
	else
		P_Error("Unexpected character '%c'", c);

	return NilValue;
}

static ValueRef P_ParseArray()
{
	ValueBuffer vb;
	PB_Init(&vb);
	P_SkipSpace();
	P_SkipToken("[");
	P_SkipSpace();
	while (gpc.s < gpc.end && NEXT() != ']')
	{
		ValueRef val = P_ParseValue();
		PB_Push(&vb, val);
		P_SkipSpace();
	}
	P_SkipToken("]");
	ValueRef rval = KS_AddArray(gpc.ksp, vb.buffer, vb.used, 0);
	PB_Destroy(&vb);
	return rval;
}

ValueRef P_ParseObject(bool topLevel)
{
	ValueBuffer vb;
	PB_Init(&vb);
	P_SkipSpace();
	if (topLevel)
	{
		P_Optional("{");
	}
	else
	{
		P_SkipToken("{");
	}
	P_SkipSpace();
	while (gpc.s < gpc.end && NEXT() != '}')
	{
		ValueRef key = P_ParseSymbol();
		PB_Push(&vb, key);
		P_SkipSpace();
		P_SkipToken("=");
		ValueRef value = P_ParseValue();
		PB_Push(&vb, value);
		P_SkipSpace();
	}
	if (topLevel)
	{
		P_Optional("}");
	}
	else
	{
		P_SkipToken("}");
	}
	ValueRef rval = KS_AddObject(gpc.ksp, (KeyValue *)vb.curPtr, vb.used / 2, 0);
	PB_Destroy(&vb);
	return rval;
}
const char *QED_LoadBuffer(KeyStore **ksp, const char *ksName, const char *buffer, size_t bufSize)
{
	ValueRef    result     = NilValue;
	const char *parseError = P_ParseBuffer(ksp, ksName, buffer, bufSize, &result);
	if (parseError == nullptr)
	{
		Assert(*ksp);
		KS_SetRoot(*ksp, result);
	}
	return nullptr;
}
const char *QED_LoadFile(KeyStore **ksp, const char *ksName, const char *fileName)
{
	size_t fileSize = 0;
	void * fileBuf  = plat->ReadEntireFile(nullptr, fileName, &fileSize);
	if (fileBuf == nullptr)
	{
		snprintf(gpc.errorBuf, sizeof(gpc.errorBuf), "QED error: Couldn't read file %s", fileName);
		return gpc.errorBuf;
	}

	const char *rval = QED_LoadBuffer(ksp, ksName, (const char *)fileBuf, fileSize);
	plat->ReleaseFileBuffer(nullptr, fileBuf);

	return rval;
}

