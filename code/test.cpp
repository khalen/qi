//
// Copyright 2017, Quantum Immortality Software and Jon Davis
//
// Test stuff
//

#include "basictypes.h"

#include "vector.h"
#include "noise.h"
#include "lexer.h"

#include <stdio.h>

static_assert(sizeof(Vector4) == sizeof(r32) * 4, "Bad size");
static_assert(GetVectorType<Vector4>::Type::Rank == 4, "Rank test fail");
static_assert(IsGreater<4, 0>::Value == true, "Bool weird?");
static_assert(GetVectorRank<Vector4>::Value == 4, "Rank test fail");
static_assert(GetVectorRank<Vector2>::Value == 2, "Rank test fail");
static_assert(GetVectorRank<float>::Value == 1, "Rank test fail");
static_assert(HasType<RemoveReferenceCV<GetVectorType<Vector4>>>::Value == true, "HasType fail");

void
Qi_Assert_Handler(const char* msg, const char* file, const int line)
{
#if HAS(OSX_BUILD)
	asm("int $3");
#else
	__debugbreak();
#endif
}

void testLex()
{
    char tokenBuf[1024];
    char fileBuf[65536];

    memset(fileBuf, 0, sizeof(fileBuf));
    FILE* f = fopen("../qi/data/sample.qid", "r");

    fseek(f, 0, SEEK_END);
    size_t len = ftell(f);

    fseek(f, 0, SEEK_SET);
    fread(fileBuf, 1, len, f);
    fclose(f);

    Lexer lex;
    Lex_Init(&lex, fileBuf, len, tokenBuf, sizeof(tokenBuf));
    LexerToken t = TOK_EOF;

    do
    {
        t = Lex_GetNextToken(&lex);
        switch(t)
        {
        case TOK_EOF:
            printf("EOF\n");
            return;
        case TOK_ERROR:
            printf("ERROR: %s ", lex.errorMsg);
            return;
        case TOK_SYMBOL:
            printf("SYM: %s ", Lex_GetTokenText(&lex));
            break;
        case TOK_STRING:
            printf("STR: %s ", Lex_GetTokenText(&lex));
            break;
        case TOK_NUMBER:
            printf("NUM: %s ", Lex_GetTokenText(&lex));
            break;
        case TOK_NEWLINE:
            printf("NL\n");
            break;
        case TOK_SPACE:
            printf("  ");
            break;
        default:
            printf("C: %c", (char)t);
            break;
        }
    } while(t != TOK_EOF);
}

int main(int, char**)
{
	Vector4 ta(1.0f, 0.0f, 0.0f, 4.0f);
    Vector4 tb(ta.wzyx);
    Vector2 ba(ta.xy);
    Vector2 bb(ta.zw);
    Vector4 te(ba, bb);

    r32 v[4];
    r32 r[4];

    for (int i = 0; i < 4; i++)
    {
        v[i] = ta[i];
        r[i] = tb[i];
    }

    Vector4 td = ta + tb;
    Vector3 tc = td.xxx;
    tc.zxy = td.yzw;
    printf("tc: %f %f %f  td: %f %f %f %f\n", tc[0], tc[1], tc[2], td[0], td[1], td[2], td[3]);

    tc.yzx = td.yxz - ta.xyz + tc;

    printf("ta: %f %f %f %f  tb: %f %f %f %f\n", v[0], v[1], v[2], v[3], r[0], r[1], r[2], r[3]);
    printf("tc: %f %f %f  td: %f %f %f %f\n", tc[0], tc[1], tc[2], td[0], td[1], td[2], td[3]);
    printf("ba: %f %f bb: %f %f\n", ba[0], ba[1], bb[0], bb[1]);
    printf("te: %f %f %f %f\n", te[0], te[1], te[2], te[3]);

    IVector4 ia(8, 9, 10, 11);

    testLex();
}
