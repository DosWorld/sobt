#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#define MAXIDLEN 32
#define MAXTYPELEN 32
#define MAXFNAMELEN 256
#define STABSIZE 512
#define STABBUFSIZE (8 * 1024)

#define TNULL 0
#define TMODULE 1
#define TBEGIN 2
#define TEND 3
#define TIMPORT 4
#define TCONST 5
#define TVAR 6
#define TPROC 7
#define TIF 8
#define TTHEN 9
#define TELSIF 10
#define TELSE 11
#define TWHILE 12
#define TDO 13
#define TREPEAT 14
#define TUNTIL 15
#define TRETURN 16
#define TARRAY 17
#define TOF 18
#define TPOINTER 19
#define TTO 20
#define TINC 21
#define TDEC 22
#define TBREAK 23
#define TCONT 24

#define TTYPEINT 100
#define TTYPELONG 101
#define TTYPEREAL 102
#define TTYPEDBL 103
#define TTYPEBOOL 104
#define TTYPECHAR 105

#define TFABS 110
#define TFODD 111
#define TFASH 112
#define TFASSERT 113
#define TFORD 114
#define TFCHR 115
#define TFFLOOR 116

#define TIDENT 50
#define TNUMBER 51
#define TCHAR 52
#define TSTRING 53

#define TASSIGN 60
#define TEQ 61
#define TNEQ 62
#define TLT 63
#define TLTE 64
#define TGT 65
#define TGTE 66

#define TPLUS 70
#define TMINUS 71
#define TOR 72
#define TMUL 73
#define TDIV 74
#define TMOD 75
#define TAND 76
#define TNOT 77

#define TLPAREN 80
#define TRPAREN 81
#define TLBRACK 82
#define TRBRACK 83
#define TCOMMA 84
#define TCOLON 85
#define TSEMICOL 86
#define TDOT 87
#define TEOF 99
#define TNIL 103
#define TTRUE 101
#define TFALSE 102

#define TSYMTHISMOD 200
#define TSYMIMOD 201
#define TSYMAMOD 202

#define TSYMCONST 300
#define TSYMPROC 301
#define TSYMGVAR 302
#define TSYMGEVAR 303
#define TSYMPARAM 304

FILE *fileIn = NULL;
FILE *fileC = NULL;
FILE *fileHeader = NULL;

char *captureBuffer = NULL;

int currentLine = 1;
int currentChar;
int currentSymbol = 0;
char currentToken[MAXIDLEN];
char moduleName[MAXIDLEN];

char sourceFileName[MAXFNAMELEN];
char outputNameC[MAXFNAMELEN];
char outputNameHeader[MAXFNAMELEN];

int isGlobalDefinition;

char curProcName[MAXIDLEN];
char curArgList[4096];
char curOneArg[MAXTYPELEN + MAXIDLEN * 2];
char curRetPrefix[MAXTYPELEN];
char curRetSuffix[MAXTYPELEN];
char curParamPrefix[MAXTYPELEN];
char curParamSuffix[MAXTYPELEN];
char curParamId[MAXIDLEN];

char desMName[MAXIDLEN];
char desName[MAXIDLEN];
char stmtLhsBuffer[2048];
char basicTypeModName[MAXIDLEN];
char basicTypeName[MAXIDLEN * 2 + 1];
char varDeclId[MAXIDLEN];
char varDeclPrefix[MAXTYPELEN];
char varDeclSuffix[MAXTYPELEN];
char varDeclBuf[MAXTYPELEN + MAXIDLEN * 2];
char constDeclName[MAXIDLEN];
char arrSizeBuf[MAXIDLEN];

int symbolTable[STABSIZE];
int symbolTableType[STABSIZE];
int symbolTableId[STABSIZE];
int symbolTableDataType[STABSIZE];

int typeForm[STABSIZE];
int typeBase[STABSIZE];
int typeLen[STABSIZE];
int typesPtr = 10;

int symbolTableFoundIndex, symbolTableFoundId, symbolTableFoundType;
char *symbolTableFoundName;

int symbolTablePtr;
char symbolTableNameBuffer[STABBUFSIZE];
int symbolTableNameBufferPtr;

void nextToken(void);
int parseExpression(void);
void parseStatementSequence(void);
int parseType(char *prefix, char *suffix);
int parseDesignator(void);
int parseBasicType(char *prefix, char *suffix);
int parseSimpleExpression(void);

void cleanupFiles(void) {
    if (fileC) fclose(fileC);
    if (fileHeader) fclose(fileHeader);
    if (fileIn) fclose(fileIn);
    fileC = fileHeader = fileIn = NULL;
}

void fatalError(const char *msg) {
    printf("%s:%d: %s\n", sourceFileName, currentLine, msg);
    cleanupFiles();
    remove(outputNameC);
    remove(outputNameHeader);
    exit(1);
}

void matchSymbol(int s, const char *msg) {
    if (currentSymbol == s) nextToken();
    else fatalError(msg);
}

int checkLexeme(int s) {
    if (currentSymbol == s) {
        nextToken();
        return 1;
    }
    return 0;
}

void emitCode(const char *s) {
    if (captureBuffer != NULL) {
        strcat(captureBuffer, s);
    } else {
        fprintf(fileC, "%s", s);
    }
}

const char* getOperatorString(int t) {
    if (t == TASSIGN) return " = ";
    if (t == TEQ) return " == ";
    if (t == TNEQ) return " != ";
    if (t == TLT) return " < ";
    if (t == TLTE) return " <= ";
    if (t == TGT) return " > ";
    if (t == TGTE) return " >= ";
    if (t == TPLUS) return " + ";
    if (t == TMINUS) return " - ";
    if (t == TOR) return " || ";
    if (t == TMUL) return " * ";
    if (t == TDIV) return " / ";
    if (t == TMOD) return " % ";
    if (t == TAND) return " && ";
    return NULL;
}

void checkTypeCompatibility(int t1, int t2) {
    if (t1 == 0 || t2 == 0) return;
    if (t1 == t2) return;
    if ((t1 == 1 || t1 == 2) && (t2 == 1 || t2 == 2)) return;
    if ((t1 == 3 || t1 == 4) && (t2 == 3 || t2 == 4)) return;
    if ((typeForm[t1] == 8) && (typeForm[t2] == 0)) return;
    if ((typeForm[t1] == 0) && (typeForm[t2] == 8)) return;
    printf("%s:%d: Type mismatch: %d != %d\n", sourceFileName, currentLine, t1, t2);
    cleanupFiles();
    exit(1);
}

int symbolTableAdd(char *name, int sid, int stype, int dtype) {
    if (symbolTablePtr >= STABSIZE) fatalError("Symbol table full");
    if (symbolTableNameBufferPtr + strlen(name) + 1 >= STABBUFSIZE) fatalError("Symbol table name buffer full");
    symbolTable[symbolTablePtr] = symbolTableNameBufferPtr;
    strcpy(&symbolTableNameBuffer[symbolTableNameBufferPtr], name);
    symbolTableNameBufferPtr += strlen(name) + 1;
    symbolTableType[symbolTablePtr] = stype;
    symbolTableId[symbolTablePtr] = sid;
    symbolTableDataType[symbolTablePtr] = dtype;
    symbolTablePtr++;
    return symbolTablePtr - 1;
}

int symbolTableFind(char *name) {
    int i;
    for (i = symbolTablePtr - 1; i >= 0; i--) {
        if (strcmp(&symbolTableNameBuffer[symbolTable[i]], name) == 0) {
            symbolTableFoundName = &symbolTableNameBuffer[symbolTable[i]];
            symbolTableFoundId = symbolTableId[i];
            symbolTableFoundType = symbolTableType[i];
            symbolTableFoundIndex = i;
            return 1;
        }
    }
    return 0;
}

void formatHex(int i) {
    int k;
    currentChar = fgetc(fileIn);
    currentToken[i] = 0;
    if (i > MAXIDLEN - 3) i = MAXIDLEN - 3;
    for (k = i; k >= 0; k--) currentToken[k + 2] = currentToken[k];
    currentToken[0] = '0';
    currentToken[1] = 'x';
}

void nextToken(void) {
    int i = 0, q = 0, level = 0;
    while (isspace(currentChar)) {
        if (currentChar == '\n') currentLine++;
        currentChar = fgetc(fileIn);
    }
    if (currentChar == EOF) {
        currentSymbol = TEOF;
        return;
    }
    if (isalpha(currentChar)) {
        i = 0;
        while (isalnum(currentChar)) {
            if (i < MAXIDLEN - 1) currentToken[i++] = (char)currentChar;
            currentChar = fgetc(fileIn);
        }
        currentToken[i] = 0;
        currentSymbol = TIDENT;
        if (symbolTableFind(currentToken)) {
            if (symbolTableFoundType < 200) {
                currentSymbol = symbolTableFoundType;
                if (currentSymbol == TTRUE) {
                    currentSymbol = TNUMBER;
                    strcpy(currentToken, "true");
                } else if (currentSymbol == TFALSE) {
                    currentSymbol = TNUMBER;
                    strcpy(currentToken, "false");
                } else if (currentSymbol == TNIL) {
                    currentSymbol = TNUMBER;
                    strcpy(currentToken, "NULL");
                }
            }
        }
        return;
    }
    if (isdigit(currentChar)) {
        i = 0;
        while (isxdigit(currentChar)) {
            if (i < MAXIDLEN - 1) currentToken[i++] = (char)currentChar;
            currentChar = fgetc(fileIn);
        }
        if (currentChar == 'H') {
            formatHex(i);
            currentSymbol = TNUMBER;
            return;
        }
        else if (currentChar == 'X') {
            formatHex(i);
            currentSymbol = TCHAR;
            return;
        }
        else if (currentChar == '.') {
            if (i < MAXIDLEN - 1) currentToken[i++] = (char)currentChar;
            currentChar = fgetc(fileIn);
            while (isdigit(currentChar)) {
                if (i < MAXIDLEN - 1) currentToken[i++] = (char)currentChar;
                currentChar = fgetc(fileIn);
            }
            if (currentChar == 'E' || currentChar == 'e') {
                if (i < MAXIDLEN - 1) currentToken[i++] = (char)currentChar;
                currentChar = fgetc(fileIn);
                if (currentChar == '+' || currentChar == '-') {
                    if (i < MAXIDLEN - 1) currentToken[i++] = (char)currentChar;
                    currentChar = fgetc(fileIn);
                }
                if (!isdigit(currentChar)) {
                    printf("Lexer error: Digit expected after exponent\n");
                    cleanupFiles();
                    exit(1);
                }
                while (isdigit(currentChar)) {
                    if (i < MAXIDLEN - 1) currentToken[i++] = (char)currentChar;
                    currentChar = fgetc(fileIn);
                }
            }
        }
        currentToken[i] = 0;
        currentSymbol = TNUMBER;
        return;
    }
    if (currentChar == '"' || currentChar == '\'') {
        q = currentChar;
        i = 0;
        currentToken[i++] = (char)q;
        currentChar = fgetc(fileIn);
        while (currentChar != q && currentChar != EOF) {
            if (i < MAXIDLEN - 2) currentToken[i++] = (char)currentChar;
            currentChar = fgetc(fileIn);
        }
        if (currentChar == q) currentChar = fgetc(fileIn);
        currentToken[i++] = (char)q;
        currentToken[i] = '\0';
        currentSymbol = (q == '"') ? TSTRING : TCHAR;
        return;
    }
    if (currentChar == '(') {
        currentChar = fgetc(fileIn);
        if (currentChar == '*') {
            currentChar = fgetc(fileIn);
            level = 1;
            while (level > 0 && currentChar != EOF) {
                if (currentChar == '(') {
                    currentChar = fgetc(fileIn);
                    if (currentChar == '*') {
                        level++;
                        currentChar = fgetc(fileIn);
                    }
                } else if (currentChar == '*') {
                    currentChar = fgetc(fileIn);
                    if (currentChar == ')') {
                        level--;
                        currentChar = fgetc(fileIn);
                    }
                } else {
                    currentChar = fgetc(fileIn);
                }
            }
            nextToken();
            return;
        }
        currentSymbol = TLPAREN;
    } else if (currentChar == ')') {
        currentChar = fgetc(fileIn);
        currentSymbol = TRPAREN;
    } else if (currentChar == '[') {
        currentChar = fgetc(fileIn);
        currentSymbol = TLBRACK;
    } else if (currentChar == ']') {
        currentChar = fgetc(fileIn);
        currentSymbol = TRBRACK;
    } else if (currentChar == ';') {
        currentChar = fgetc(fileIn);
        currentSymbol = TSEMICOL;
    } else if (currentChar == ',') {
        currentChar = fgetc(fileIn);
        currentSymbol = TCOMMA;
    } else if (currentChar == '.') {
        currentChar = fgetc(fileIn);
        currentSymbol = TDOT;
    } else if (currentChar == '=') {
        currentChar = fgetc(fileIn);
        currentSymbol = TEQ;
    } else if (currentChar == '#') {
        currentChar = fgetc(fileIn);
        currentSymbol = TNEQ;
    } else if (currentChar == '+') {
        currentChar = fgetc(fileIn);
        currentSymbol = TPLUS;
    } else if (currentChar == '-') {
        currentChar = fgetc(fileIn);
        currentSymbol = TMINUS;
    } else if (currentChar == '*') {
        currentChar = fgetc(fileIn);
        currentSymbol = TMUL;
    } else if (currentChar == '&') {
        currentChar = fgetc(fileIn);
        currentSymbol = TAND;
    } else if (currentChar == '~') {
        currentChar = fgetc(fileIn);
        currentSymbol = TNOT;
    } else if (currentChar == ':') {
        currentChar = fgetc(fileIn);
        if (currentChar == '=') {
            currentChar = fgetc(fileIn);
            currentSymbol = TASSIGN;
        } else {
            currentSymbol = TCOLON;
        }
    } else if (currentChar == '<') {
        currentChar = fgetc(fileIn);
        if (currentChar == '=') {
            currentChar = fgetc(fileIn);
            currentSymbol = TLTE;
        } else {
            currentSymbol = TLT;
        }
    } else if (currentChar == '>') {
        currentChar = fgetc(fileIn);
        if (currentChar == '=') {
            currentChar = fgetc(fileIn);
            currentSymbol = TGTE;
        } else {
            currentSymbol = TGT;
        }
    } else {
        printf("Lexer error: Unknown char '%c'\n", currentChar);
        cleanupFiles();
        exit(1);
    }
}

void consumeIdentifier(char *buf) {
    strncpy(buf, currentToken, MAXIDLEN - 1);
    buf[MAXIDLEN - 1] = '\0';
    matchSymbol(TIDENT, "Identifier expected");
}

int parseDesignator(void) {
    int tid = 0;
    int idxType;
    strcpy(desMName, moduleName);
    if(symbolTableFoundType == TSYMAMOD) {
        symbolTableFoundIndex = symbolTableId[symbolTableFoundIndex];
        symbolTableFoundName = &symbolTableNameBuffer[symbolTable[symbolTableFoundIndex]];
        symbolTableFoundType = symbolTableType[symbolTableFoundIndex];
    }
    if((symbolTableFoundType == TSYMTHISMOD) || (symbolTableFoundType == TSYMIMOD)) {
        strcpy(desMName, symbolTableFoundName);
        matchSymbol(TIDENT, "module name expected");
        matchSymbol(TDOT, ". expected");
        if (currentSymbol == TIDENT) {
            symbolTableFind(currentToken);
        }
    }
    tid = symbolTableDataType[symbolTableFoundIndex];
    consumeIdentifier(desName);
    emitCode(desMName);
    emitCode("_");
    emitCode(desName);
    while (checkLexeme(TLBRACK)) {
        if (typeForm[tid] != 7) fatalError("Array expected");
        tid = typeBase[tid];
        emitCode("[");
        idxType = parseExpression();
        checkTypeCompatibility(idxType, 1);
        matchSymbol(TRBRACK, "] expected");
        emitCode("]");
    }
    return tid;
}

void parseParameters(void) {
    matchSymbol(TLPAREN, "( expected");
    emitCode("(");
    if (currentSymbol != TRPAREN) {
        do {
            parseExpression();
            if (checkLexeme(TCOMMA)) emitCode(", ");
            else break;
        } while (1);
    }
    matchSymbol(TRPAREN, ") expected");
    emitCode(")");
}

int parseUnaryFunc(char *pre, char *suf, int inT, int outT) {
    int t;
    nextToken();
    matchSymbol(TLPAREN, "( expected");
    emitCode(pre);
    t = parseExpression();
    if (inT > 0) checkTypeCompatibility(t, inT);
    emitCode(suf);
    matchSymbol(TRPAREN, ") expected");
    return outT;
}

int parseFactor(void) {
    int tid = 0;
    int argT, t1, t2;
    if (currentSymbol == TNUMBER) {
        emitCode(currentToken);
        if (strchr(currentToken, '.') || strchr(currentToken, 'E')) tid = 3;
        else tid = 1;
        nextToken();
    } else if (currentSymbol == TCHAR) {
        emitCode(currentToken);
        tid = 6;
        nextToken();
    } else if (currentSymbol == TSTRING) {
        emitCode(currentToken);
        tid = 7;
        nextToken();
    } else if (checkLexeme(TLPAREN)) {
        emitCode("(");
        tid = parseExpression();
        matchSymbol(TRPAREN, ") expected");
        emitCode(")");
    } else if (checkLexeme(TNOT)) {
        emitCode("!");
        tid = parseFactor();
        checkTypeCompatibility(tid, 5);
    } else if (currentSymbol == TFABS) {
        nextToken();
        matchSymbol(TLPAREN, "( expected");
        argT = parseExpression();
        if (argT == 3 || argT == 4) emitCode("fabs(");
        else emitCode("abs(");
        matchSymbol(TRPAREN, ") expected");
        emitCode(")");
        tid = argT;
    } else if (currentSymbol == TFODD) {
        tid = parseUnaryFunc("((", ") & 1)", 1, 5);
    } else if (currentSymbol == TFASH) {
        nextToken();
        matchSymbol(TLPAREN, "( expected");
        fprintf(fileC, "%s_ASH(", moduleName);
        t1 = parseExpression();
        checkTypeCompatibility(t1, 1);
        matchSymbol(TCOMMA, ", expected");
        emitCode(", ");
        t2 = parseExpression();
        checkTypeCompatibility(t2, 1);
        matchSymbol(TRPAREN, ") expected");
        emitCode(")");
        tid = t1;
    } else if (currentSymbol == TFASSERT) {
        tid = parseUnaryFunc("if (!(", ")) abort()", 5, 0);
    } else if (currentSymbol == TFORD) {
        tid = parseUnaryFunc("((int)(", "))", 0, 1);
    } else if (currentSymbol == TFCHR) {
        tid = parseUnaryFunc("((char)(", "))", 1, 6);
    } else if (currentSymbol == TFFLOOR) {
        tid = parseUnaryFunc("((long)floor(", "))", 3, 2);
    } else if (currentSymbol == TIDENT) {
        tid = parseDesignator();
        if (currentSymbol == TLPAREN) {
            parseParameters();
        }
    } else {
        fatalError("Factor expected");
    }
    return tid;
}

int parseTerm(void) {
    int t2, t1;
    emitCode("(");
    t1 = parseFactor();
    while (currentSymbol >= TMUL && currentSymbol <= TAND) {
        emitCode(getOperatorString(currentSymbol));
        nextToken();
        t2 = parseFactor();
        checkTypeCompatibility(t1, t2);
    }
    emitCode(")");
    return t1;
}

int parseSimpleExpression(void) {
    int t1, t2;
    if (checkLexeme(TPLUS)) { }
    else if (checkLexeme(TMINUS)) emitCode("-");
    t1 = parseTerm();
    while (currentSymbol >= TPLUS && currentSymbol <= TOR) {
        emitCode(getOperatorString(currentSymbol));
        nextToken();
        t2 = parseTerm();
        checkTypeCompatibility(t1, t2);
    }
    return t1;
}

int parseExpression(void) {
    int t1, t2;
    emitCode("(");
    t1 = parseSimpleExpression();
    emitCode(")");
    if (currentSymbol >= TEQ && currentSymbol <= TGTE) {
        emitCode(getOperatorString(currentSymbol));
        nextToken();
        emitCode("(");
        t2 = parseSimpleExpression();
        checkTypeCompatibility(t1, t2);
        emitCode(")");
        return 5;
    }
    return t1;
}

int parseBasicType(char *prefix, char *suffix) {
    int targetId;
    int tid = 0;
    int isQualified = 0;
    prefix[0] = 0;
    suffix[0] = 0;
    if (currentSymbol == TIDENT) {
        if (symbolTableFind(currentToken)) {
            if (symbolTableFoundType == TSYMIMOD || symbolTableFoundType == TSYMAMOD) {
                if (symbolTableFoundType == TSYMAMOD) {
                    targetId = symbolTableId[symbolTableFoundIndex];
                    strcpy(basicTypeModName, &symbolTableNameBuffer[symbolTable[targetId]]);
                } else {
                    strcpy(basicTypeModName, symbolTableFoundName);
                }
                nextToken();
                matchSymbol(TDOT, ". expected");
                if (currentSymbol != TIDENT) fatalError("Type identifier expected after module");
                sprintf(basicTypeName, "%s_%s", basicTypeModName, currentToken);
                isQualified = 1;
            } else {
                tid = symbolTableDataType[symbolTableFoundIndex];
            }
        }
        if (!isQualified) {
            strcpy(basicTypeName, currentToken);
        }
        strcpy(prefix, basicTypeName);
        nextToken();
    } else if (checkLexeme(TTYPEINT)) {
        strcpy(prefix, "int");
        tid=1;
    } else if (checkLexeme(TTYPELONG)) {
        strcpy(prefix, "long");
        tid=2;
    } else if (checkLexeme(TTYPEREAL)) {
        strcpy(prefix, "float");
        tid=3;
    } else if (checkLexeme(TTYPEDBL)) {
        strcpy(prefix, "double");
        tid=4;
    } else if (checkLexeme(TTYPEBOOL)) {
        strcpy(prefix, "bool");
        tid=5;
    } else if (checkLexeme(TTYPECHAR)) {
        strcpy(prefix, "char");
        tid=6;
    } else {
        fatalError("Type expected");
    }
    return tid;
}

int parseArraySize(char *buf) {
    int len = 0;
    if (currentSymbol == TNUMBER) {
        strcpy(buf, currentToken);
        len = atoi(currentToken);
        nextToken();
    } else if (currentSymbol == TIDENT) {
        if (symbolTableFind(currentToken)) {
            strcpy(buf, moduleName);
            if(symbolTableFoundType == TSYMAMOD) {
                symbolTableFoundIndex = symbolTableId[symbolTableFoundIndex];
                symbolTableFoundName = &symbolTableNameBuffer[symbolTable[symbolTableFoundIndex]];
                symbolTableFoundType = symbolTableType[symbolTableFoundIndex];
            }
            if((symbolTableFoundType == TSYMTHISMOD) || (symbolTableFoundType == TSYMIMOD)) {
                strcpy(buf, symbolTableFoundName);
                matchSymbol(TIDENT, "module name expected");
                matchSymbol(TDOT, ". expected");
            }
            if (symbolTableFoundType == TSYMCONST) {
                strcat(buf, "_");
                strcat(buf, currentToken);
            } else {
                strcpy(buf, currentToken);
            }
            nextToken();
        } else fatalError("Array size expected");
    } else fatalError("Array size expected");
    return len;
}

int parseType(char *prefix, char *suffix) {
    int tid = 0, base;
    prefix[0] = 0;
    suffix[0] = 0;
    if (checkLexeme(TPOINTER)) {
        if (checkLexeme(TTO)) {
            base = parseBasicType(prefix, suffix);
            strcat(prefix, " *");
            tid = typesPtr++;
            typeForm[tid] = 8;
            typeBase[tid] = base;
        } else {
            strcpy(prefix, "void ");
            tid = typesPtr++;
            typeForm[tid] = 8;
            typeBase[tid] = 0;
        }
    } else if (checkLexeme(TARRAY)) {
        int arrLen = parseArraySize(arrSizeBuf);
        matchSymbol(TOF, "OF expected");
        base = parseType(prefix, suffix);
        strcpy(suffix, "[");
        strcat(suffix, arrSizeBuf);
        strcat(suffix, "]");
        tid = typesPtr++;
        typeForm[tid] = 7;
        typeBase[tid] = base;
        typeLen[tid] = arrLen;
    } else if (checkLexeme(TPROC)) {
        strcpy(prefix, "void (");
        strcpy(suffix, ")()");
        tid = 9;
    } else {
        tid = parseBasicType(prefix, suffix);
    }
    return tid;
}

void printVariable(char *tgt, const char *name, const char *prefix, const char *suffix) {
    strcpy(tgt, prefix);
    strcat(tgt, " ");
    strcat(tgt, moduleName);
    strcat(tgt, "_");
    strcat(tgt, name);
    strcat(tgt, suffix);
}

void parseIncDec(int isInc) {
    nextToken();
    matchSymbol(TLPAREN, "( expected");
    parseDesignator();
    if (checkLexeme(TCOMMA)) {
        emitCode(isInc ? " += " : " -= ");
        parseExpression();
    } else {
        emitCode(isInc ? "++" : "--");
    }
    matchSymbol(TRPAREN, ") expected");
    emitCode(";\n");
}

void parseStatement(void) {
    int t, t2;
    if (currentSymbol == TIDENT) {
        stmtLhsBuffer[0] = 0;
        captureBuffer = stmtLhsBuffer;
        t = parseDesignator();
        captureBuffer = NULL;
        if (checkLexeme(TASSIGN)) {
            if (currentSymbol == TSTRING) {
                fprintf(fileC, "strcpy(%s, %s)", stmtLhsBuffer, currentToken);
                nextToken();
            } else {
                fprintf(fileC, "%s = ", stmtLhsBuffer);
                t2 = parseExpression();
                checkTypeCompatibility(t, t2);
            }
        } else if (currentSymbol == TLPAREN) {
            fprintf(fileC, "%s", stmtLhsBuffer);
            parseParameters();
        } else {
            fprintf(fileC, "%s()", stmtLhsBuffer);
        }
        emitCode(";\n");
    } else if (checkLexeme(TIF)) {
        emitCode("if (");
        t = parseExpression();
        checkTypeCompatibility(t, 5);
        emitCode(") {\n");
        matchSymbol(TTHEN, "THEN expected");
        parseStatementSequence();
        while (checkLexeme(TELSIF)) {
            emitCode("} else if (");
            t = parseExpression();
            checkTypeCompatibility(t, 5);
            emitCode(") {\n");
            matchSymbol(TTHEN, "THEN expected");
            parseStatementSequence();
        }
        if (checkLexeme(TELSE)) {
            emitCode("} else {\n");
            parseStatementSequence();
        }
        matchSymbol(TEND, "END expected");
        emitCode("}\n");
    } else if (checkLexeme(TWHILE)) {
        emitCode("while (");
        t = parseExpression();
        checkTypeCompatibility(t, 5);
        emitCode(") {\n");
        matchSymbol(TDO, "DO expected");
        parseStatementSequence();
        matchSymbol(TEND, "END expected");
        emitCode("}\n");
    } else if (checkLexeme(TREPEAT)) {
        emitCode("do {\n");
        parseStatementSequence();
        emitCode("\n} while (!(\n");
        matchSymbol(TUNTIL, "UNTIL expected");
        t = parseExpression();
        checkTypeCompatibility(t, 5);
        emitCode("));\n");
    } else if (checkLexeme(TRETURN)) {
        emitCode("return ");
        if (currentSymbol != TSEMICOL && currentSymbol != TEND && currentSymbol != TELSE && currentSymbol != TELSIF) {
            parseExpression();
        }
        emitCode(";\n");
    } else if (currentSymbol == TINC) {
        parseIncDec(1);
    } else if (currentSymbol == TDEC) {
        parseIncDec(0);
    } else if (checkLexeme(TBREAK)) {
        emitCode("break;\n");
    } else if (checkLexeme(TCONT)) {
        emitCode("continue;\n");
    }
}

void parseStatementSequence(void) {
    while (currentSymbol != TEND && currentSymbol != TELSIF && currentSymbol != TELSE) {
        parseStatement();
        checkLexeme(TSEMICOL);
    }
}

void parseVariableDeclaration(void) {
    int i, startSymbolTablePtr, isExported;
    int tid;
    while (currentSymbol == TIDENT) {
        startSymbolTablePtr = symbolTablePtr;
        do {
            consumeIdentifier(varDeclId);
            isExported = checkLexeme(TMUL);
            if (symbolTableFind(varDeclId)) fatalError("Duplicate identifier");
            symbolTableAdd(varDeclId, isExported, isExported ? TSYMGEVAR : TSYMGVAR, 0);
        } while (checkLexeme(TCOMMA));
        matchSymbol(TCOLON, ": expected");
        tid = parseType(varDeclPrefix, varDeclSuffix);
        matchSymbol(TSEMICOL, "; expected");
        for (i = startSymbolTablePtr; i < symbolTablePtr; i++) {
            symbolTableDataType[i] = tid;
            printVariable(varDeclBuf, &symbolTableNameBuffer[symbolTable[i]], varDeclPrefix, varDeclSuffix);
            if (isGlobalDefinition) {
                if (symbolTableType[i] == TSYMGEVAR) {
                    fprintf(fileC, "%s;\n", varDeclBuf);
                    fprintf(fileHeader, "extern %s;\n", varDeclBuf);
                } else if (symbolTableType[i] == TSYMGVAR) {
                    fprintf(fileC, "static %s;\n", varDeclBuf);
                }
            } else {
                fprintf(fileC, "%s;\n", varDeclBuf);
            }
        }
    }
}

void parseProcedureHeader(int *savedSymbolTablePtr, int *savedSymbolTableNameBufferPtr) {
    int exp = 0, i;
    int startSymbolTablePtr;
    int tid;
    curArgList[0] = 0;
    curRetPrefix[0] = 0;
    curRetSuffix[0] = 0;
    strcpy(curRetPrefix, "void");
    consumeIdentifier(curProcName);
    exp = checkLexeme(TMUL);
    symbolTableAdd(curProcName, 0, TSYMPROC, 0);
    *savedSymbolTablePtr = symbolTablePtr;
    *savedSymbolTableNameBufferPtr = symbolTableNameBufferPtr;
    if (checkLexeme(TLPAREN)) {
        if (currentSymbol != TRPAREN) {
            do {
                if (currentSymbol == TVAR) {
                    fatalError("VAR parameters not supported");
                }
                startSymbolTablePtr = symbolTablePtr;
                do {
                    consumeIdentifier(curParamId);
                    if (symbolTableFind(curParamId)) fatalError("Duplicate parameter");
                    symbolTableAdd(curParamId, 0, TSYMPARAM, 0);
                } while (checkLexeme(TCOMMA));
                matchSymbol(TCOLON, ": expected");
                tid = parseType(curParamPrefix, curParamSuffix);
                for (i = startSymbolTablePtr; i < symbolTablePtr; i++) {
                    symbolTableDataType[i] = tid;
                    if (strlen(curArgList) > 0) strcat(curArgList, ", ");
                    printVariable(curOneArg, &symbolTableNameBuffer[symbolTable[i]], curParamPrefix, curParamSuffix);
                    strcat(curArgList, curOneArg);
                }
            } while (checkLexeme(TSEMICOL));
        }
        matchSymbol(TRPAREN, ") expected");
    }
    if (strlen(curArgList) == 0) strcpy(curArgList, "void");
    if (checkLexeme(TCOLON)) {
        parseType(curRetPrefix, curRetSuffix);
    }
    matchSymbol(TSEMICOL, "; expected");
    fprintf(fileC, "\n%s%s %s_%s(%s)%s", exp ? "" : "static ", curRetPrefix, moduleName, curProcName, curArgList, curRetSuffix);
    if (exp) fprintf(fileHeader, "extern %s %s_%s(%s)%s;\n", curRetPrefix, moduleName, curProcName, curArgList, curRetSuffix);
    emitCode(" {\n");
}

void parseProcedureDeclaration(void) {
    int oldSymbolTablePtr, oldSymbolTableNameBufferPtr;
    parseProcedureHeader(&oldSymbolTablePtr, &oldSymbolTableNameBufferPtr);
    isGlobalDefinition = 0;
    while (checkLexeme(TVAR)) parseVariableDeclaration();
    matchSymbol(TBEGIN, "BEGIN expected");
    parseStatementSequence();
    matchSymbol(TEND, "END expected");
    matchSymbol(TIDENT, "Identifier expected");
    matchSymbol(TSEMICOL, "; expected");
    emitCode("}\n");
    isGlobalDefinition = 1;
    symbolTablePtr = oldSymbolTablePtr;
    symbolTableNameBufferPtr = oldSymbolTableNameBufferPtr;
}

void parseConstantDeclaration(void) {
    int isExported;
    int tid = 0;
    while (currentSymbol == TIDENT) {
        consumeIdentifier(constDeclName);
        isExported = checkLexeme(TMUL);
        matchSymbol(TEQ, "= expected");
        if (currentSymbol == TNUMBER) {
            if (strchr(currentToken, '.') || strchr(currentToken, 'E')) tid = 3;
            else tid = 1;
        } else if (currentSymbol == TSTRING) {
            tid = 7;
        } else if (currentSymbol == TCHAR) {
            tid = 6;
        }
        symbolTableAdd(constDeclName, 0, TSYMCONST, tid);
        fprintf(isExported ? fileHeader : fileC, "#define\t%s_%s\t%s\n", moduleName, constDeclName, currentToken);
        if (currentSymbol == TNUMBER) matchSymbol(TNUMBER, "Number expected");
        else if (currentSymbol == TSTRING) matchSymbol(TSTRING, "String expected");
        else if (currentSymbol == TCHAR) matchSymbol(TCHAR, "Char expected");
        else nextToken();
        matchSymbol(TSEMICOL, "; expected");
    }
}

void initCompiler(void) {
    currentLine = 1;
    isGlobalDefinition = 0;
    captureBuffer = NULL;
    symbolTablePtr = 0;
    symbolTableNameBufferPtr = 0;
    typesPtr = 10;
    typeForm[1]=1;
    typeForm[2]=2;
    typeForm[3]=3;
    typeForm[4]=4;
    typeForm[5]=5;
    typeForm[6]=6;
    typeForm[8]=8;
    symbolTableAdd("MODULE", 0, TMODULE, 0);
    symbolTableAdd("BEGIN", 0, TBEGIN, 0);
    symbolTableAdd("END", 0, TEND, 0);
    symbolTableAdd("IMPORT", 0, TIMPORT, 0);
    symbolTableAdd("CONST", 0, TCONST, 0);
    symbolTableAdd("VAR", 0, TVAR, 0);
    symbolTableAdd("PROCEDURE", 0, TPROC, 0);
    symbolTableAdd("IF", 0, TIF, 0);
    symbolTableAdd("THEN", 0, TTHEN, 0);
    symbolTableAdd("ELSIF", 0, TELSIF, 0);
    symbolTableAdd("ELSE", 0, TELSE, 0);
    symbolTableAdd("WHILE", 0, TWHILE, 0);
    symbolTableAdd("DO", 0, TDO, 0);
    symbolTableAdd("REPEAT", 0, TREPEAT, 0);
    symbolTableAdd("UNTIL", 0, TUNTIL, 0);
    symbolTableAdd("RETURN", 0, TRETURN, 0);
    symbolTableAdd("ARRAY", 0, TARRAY, 0);
    symbolTableAdd("OF", 0, TOF, 0);
    symbolTableAdd("POINTER", 0, TPOINTER, 0);
    symbolTableAdd("TO", 0, TTO, 0);
    symbolTableAdd("INC", 0, TINC, 0);
    symbolTableAdd("DEC", 0, TDEC, 0);
    symbolTableAdd("BREAK", 0, TBREAK, 0);
    symbolTableAdd("CONTINUE", 0, TCONT, 0);
    symbolTableAdd("OR", 0, TOR, 0);
    symbolTableAdd("DIV", 0, TDIV, 0);
    symbolTableAdd("MOD", 0, TMOD, 0);
    symbolTableAdd("TRUE", 0, TTRUE, 5);
    symbolTableAdd("FALSE", 0, TFALSE, 5);
    symbolTableAdd("NIL", 0, TNIL, 8);
    symbolTableAdd("ABS", 0, TFABS, 0);
    symbolTableAdd("ODD", 0, TFODD, 0);
    symbolTableAdd("ASH", 0, TFASH, 0);
    symbolTableAdd("ASSERT", 0, TFASSERT, 0);
    symbolTableAdd("ORD", 0, TFORD, 0);
    symbolTableAdd("CHR", 0, TFCHR, 0);
    symbolTableAdd("FLOOR", 0, TFFLOOR, 0);
    symbolTableAdd("INTEGER", 0, TTYPEINT, 1);
    symbolTableAdd("LONGINT", 0, TTYPELONG, 2);
    symbolTableAdd("REAL", 0, TTYPEREAL, 3);
    symbolTableAdd("LONGREAL", 0, TTYPEDBL, 4);
    symbolTableAdd("BOOLEAN", 0, TTYPEBOOL, 5);
    symbolTableAdd("CHAR", 0, TTYPECHAR, 6);
    fileIn = fileHeader = fileC = NULL;
}

void compileModule(char *src) {
    char *dot;
    int len;
    int moduleId, moduleAliasId;
    initCompiler();
    strcpy(sourceFileName, src);
    fileIn = fopen(sourceFileName, "r");
    if (!fileIn) {
        printf("Error: Cannot open %s\n", sourceFileName);
        exit(1);
    }
    dot = strrchr(sourceFileName, '.');
    len = dot ? (int)(dot - sourceFileName) : (int)strlen(sourceFileName);
    strncpy(outputNameC, sourceFileName, len);
    outputNameC[len] = '\0';
    strcpy(outputNameHeader, outputNameC);
    strcat(outputNameC, ".c");
    strcat(outputNameHeader, ".h");
    fileC = fopen(outputNameC, "rw");
    fileHeader = fopen(outputNameHeader, "rw");
    if (!fileC || !fileHeader) fatalError("Cannot create output files");
    currentChar = fgetc(fileIn);
    nextToken();
    matchSymbol(TMODULE, "MODULE expected");
    consumeIdentifier(moduleName);
    symbolTableAdd(moduleName, 0, TSYMTHISMOD, 0);
    matchSymbol(TSEMICOL, "; expected");
    fprintf(fileHeader, "#ifndef %s_H\n#define %s_H\n\n#include <stdint.h>\n#include <stdbool.h>\n#include <stdlib.h>\n#include <string.h>\n#include <math.h>\n", moduleName, moduleName);
    fprintf(fileHeader, "#define %s_ASH(x, n) ((n) >= 0 ? ((x) << (n)) : ((x) >> -(n)))\n\n", moduleName);
    isGlobalDefinition = 1;
    while (checkLexeme(TIMPORT)) {
        do {
            if (currentSymbol == TIDENT) {
                moduleId = symbolTableAdd(currentToken, 0, TSYMIMOD, 0);
                nextToken();
                if(checkLexeme(TASSIGN)) {
                    moduleAliasId = moduleId;
                    moduleId = symbolTableAdd(currentToken, 0, TSYMIMOD, 0);
                    symbolTableType[moduleAliasId] = TSYMAMOD;
                    symbolTableId[moduleAliasId] = moduleId;
                    matchSymbol(TIDENT, "module name expected");
                }
                fprintf(fileC, "#include \"%s.h\"\n", symbolTableNameBuffer[symbolTable[moduleId]]);
            }
        } while (checkLexeme(TCOMMA));
        matchSymbol(TSEMICOL, "; expected");
    }
    fprintf(fileC, "#include \"%s\"\n\n", outputNameHeader);
    while (1) {
        if (checkLexeme(TCONST)) {
            parseConstantDeclaration();
        } else if (checkLexeme(TVAR)) {
            parseVariableDeclaration();
        } else if(checkLexeme(TPROC)) {
            parseProcedureDeclaration();
        } else {
            break;
        }
    }
    fprintf(fileC, "\nstatic char is_%s_init = 0;\n", moduleName);
    fprintf(fileC, "void mod_%s_init() {\n", moduleName);
    fprintf(fileC, "if(is_%s_init) {\nreturn;\n}\nis_%s_init = 1;\n", moduleName, moduleName);
    if(checkLexeme(TBEGIN)) {
        parseStatementSequence();
    }
    emitCode("}\n");
    fprintf(fileHeader, "\nextern void mod_%s_init();\n", moduleName);
    matchSymbol(TEND, "END expected");
    matchSymbol(TIDENT, "Identifier expected");
    matchSymbol(TDOT, ". expected");
    matchSymbol(TEOF, "EOF expected");
    fprintf(fileHeader, "\n#endif\n", moduleName);
    cleanupFiles();
}

int main(int argc, char **argv) {
    int i;
    if (argc == 1) {
        printf("Usage:\n\t%s filename.mod\n", argv[0]);
        return 1;
    }
    for(i = 1; i < argc; i++) {
        if (strlen(argv[i]) < MAXFNAMELEN) {
            if(argv[i][0] != '-') {
                compileModule(argv[i]);
            }
        }
    }
    return 0;
}
