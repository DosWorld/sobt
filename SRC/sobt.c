/*
 Small Oberon to C99 Translator (Refactored)

 Original by DosWorld (CC0 1.0).
 To view a copy of this mark,
 visit https://creativecommons.org/publicdomain/zero/1.0/

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* -- Configuration & Limits -- */
#define MAXIDLEN      64
#define MAXTYPELEN    32
#define MAXFNAMELEN   256
#define STABSIZE       512
#define STABBUFSIZE   (16 * 1024)

/* -- Tokens -- */
#define TNULL      0
#define TMODULE    1
#define TBEGIN     2
#define TEND       3
#define TIMPORT    4
#define TCONST     5
#define TVAR       6
#define TPROC      7
#define TIF        8
#define TTHEN      9
#define TELSIF     10
#define TELSE      11
#define TWHILE     12
#define TDO        13
#define TREPEAT    14
#define TUNTIL     15
#define TRETURN    16
#define TARRAY     17
#define TOF        18
#define TPOINTER   19
#define TTO        20
#define TINC       21
#define TDEC       22
#define TBREAK     23
#define TCONT      24

/* -- Built-in Types (100-110) -- */
#define TTYPEINT      100
#define TTYPELONG     101
#define TTYPEREAL     102
#define TTYPEDBL      103
#define TTYPEBOOL     104
#define TTYPECHAR     105

#define TIDENT     50
#define TNUMBER    51
#define TCHAR      52
#define TSTRING    53

/* -- Operators -- */
#define TASSIGN    60 /* := */
#define TEQ        61 /* = */
#define TNEQ       62 /* # */
#define TLT        63 /* < */
#define TLTE       64 /* <= */
#define TGT        65 /* > */
#define TGTE       66 /* >= */

#define TPLUS      70 /* + */
#define TMINUS     71 /* - */
#define TOR        72 /* OR */
#define TMUL       73 /* * */
#define TDIV       74 /* DIV */
#define TMOD       75 /* MOD */
#define TAND       76 /* & */

#define TLPAREN    80 /* ( */
#define TRPAREN    81 /* ) */
#define TLBRACK    82 /* [ */
#define TRBRACK    83 /* ] */
#define TCOMMA     84 /* , */
#define TCOLON     85 /* : */
#define TSEMICOL   86 /* ; */
#define TDOT       87 /* . */
#define TEOF       99
#define TNIL       103
#define TTRUE      101
#define TFALSE     102

/* -- Symbol Table Types -- */
#define TSYMTHISMOD  200
#define TSYMIMOD     201
#define TSYMAMOD     202

#define TSYMCONST    300
#define TSYMPROC     301
#define TSYMGVAR     302
#define TSYMGEVAR    303
#define TSYMPARAM    304

/* -- Globals -- */
FILE *fin = NULL;
FILE *fc = NULL;
FILE *fh = NULL;

int curLine = 1;
int cch;
int symbol = 0;
char ctoken[MAXIDLEN];
char modName[MAXIDLEN];

char sourceFileName[MAXFNAMELEN];
char outNameC[MAXFNAMELEN];
char outNameHeader[MAXFNAMELEN];

int isGlobalDef;

char g_procName[MAXIDLEN];
char g_argList[4096];
char g_oneArg[MAXTYPELEN + MAXIDLEN * 2];
char g_retPre[MAXTYPELEN];
char g_retSuf[MAXTYPELEN];
char g_pPre[MAXTYPELEN];
char g_pSuf[MAXTYPELEN];
char g_idBuf[MAXIDLEN];

/* -- Symbol Table Data -- */
int stab[STABSIZE];
int stab_type[STABSIZE];
int stab_id[STABSIZE];

int stab_finded, stab_fid, stab_ftype;
char *stab_fname;

int stab_ptr;
char stab_nbuf[STABBUFSIZE];
int stab_nbuf_ptr;

/* -- Forward Declarations -- */
void next(void);
void expr(void);
void stmt_seq(void);
void parse_type(char *prefix, char *suffix);

/* -- Utility Functions -- */

void cleanup_files() {
    if (fc) fclose(fc);
    if (fh) fclose(fh);
    if (fin) fclose(fin);
    fc = fh = fin = NULL;
}

void error(const char *msg) {
    printf("%s:%d: %s\n", sourceFileName, curLine, msg);
    cleanup_files();
    remove(outNameC);
    remove(outNameHeader);
    exit(1);
}

void match(int s, const char *msg) {
    if (symbol == s) next();
    else error(msg);
}

int isLex(int s) {
    if (symbol == s) {
        next();
        return 1;
    }
    return 0;
}

void emit(const char *s) {
    fprintf(fc, "%s", s);
}

const char* get_op_str(int t) {
    if (t == TASSIGN) {
        return " = ";
    } else if (t == TEQ) {
        return " == ";
    } else if (t == TNEQ) {
        return " != ";
    } else if (t == TLT) {
        return " < ";
    } else if (t == TLTE) {
        return " <= ";
    } else if (t == TGT) {
        return " > ";
    } else if (t == TGTE) {
        return " >= ";
    } else if (t == TPLUS) {
        return " + ";
    } else if (t == TMINUS) {
        return " - ";
    } else if (t == TOR) {
        return " || ";
    } else if (t == TMUL) {
        return " * ";
    } else if (t == TDIV) {
        return " / ";
    } else if (t == TMOD) {
        return " % ";
    } else if (t == TAND) {
        return " && ";
    } else {
        return NULL;
    }
}

/* -- Symbol Table Functions -- */

int stab_add(char *name, int sid, int stype) {
    if (stab_ptr >= STABSIZE) error("Symbol table full");
    if (stab_nbuf_ptr + strlen(name) + 1 >= STABBUFSIZE) error("Symbol table name buffer full");

    stab[stab_ptr] = stab_nbuf_ptr;
    strcpy(&stab_nbuf[stab_nbuf_ptr], name);
    stab_nbuf_ptr += strlen(name) + 1;
    stab_type[stab_ptr] = stype;
    stab_id[stab_ptr] = sid;
    stab_ptr++;
    return stab_ptr - 1;
}

int stab_find(char *name) {
    /* Search backwards to handle scope shadowing correctly */
    int i;
    for (i = stab_ptr - 1; i >= 0; i--) {
        if (strcmp(&stab_nbuf[stab[i]], name) == 0) {
            stab_fname = &stab_nbuf[stab[i]];
            stab_fid = stab_id[i];
            stab_ftype = stab_type[i];
            stab_finded = i;
            return 1;
        }
    }
    return 0;
}

/* -- Lexer -- */

void next(void) {
    int i = 0, q = 0, level = 0, isHex;
    while (isspace(cch)) {
        if (cch == '\n') curLine++;
        cch = fgetc(fin);
    }

    if (cch == EOF) {
        symbol = TEOF;
        return;
    }

    /* Identifiers, Keywords, and Types */
    if (isalpha(cch)) {
        i = 0;
        while (isalnum(cch)) {
            if (i < MAXIDLEN - 1) ctoken[i++] = (char)cch;
            cch = fgetc(fin);
        }
        ctoken[i] = 0;
        symbol = TIDENT;

        if (stab_find(ctoken)) {
            if (stab_ftype < 200) {
                symbol = stab_ftype;
                if (symbol == TTRUE) {
                    symbol = TNUMBER;
                    strcpy(ctoken, "1");
                } else if (symbol == TFALSE) {
                    symbol = TNUMBER;
                    strcpy(ctoken, "0");
                } else if (symbol == TNIL) {
                    symbol = TNUMBER;
                    strcpy(ctoken, "NULL");
                }
            }
        }
        return;
    }

    /* Numbers (Dec/Hex) */
    if (isdigit(cch) || cch == '$') {
        isHex = (cch == '$');
        if (isHex) cch = fgetc(fin);
        i = 0;
        if (isHex) {
            if (i < MAXIDLEN - 2) {
                ctoken[i++]='0';
                ctoken[i++]='x';
            }
        }
        while ((isHex && isxdigit(cch)) || (!isHex && isdigit(cch))) {
            if (i < MAXIDLEN - 1) ctoken[i++] = (char)cch;
            cch = fgetc(fin);
        }
        if(!isHex && (cch == '.')) {
            if (i < MAXIDLEN - 1) ctoken[i++] = (char)cch;
            cch = fgetc(fin);
            while (isdigit(cch)) {
                if (i < MAXIDLEN - 1) ctoken[i++] = (char)cch;
                cch = fgetc(fin);
            }
        }
        ctoken[i] = 0;
        symbol = TNUMBER;
        return;
    }

    /* Strings / Chars */
    if (cch == '"' || cch == '\'') {
        q = cch;
        i = 0;
        ctoken[i++] = (char)q;
        cch = fgetc(fin);
        while (cch != q && cch != EOF) {
            if (i < MAXIDLEN - 2) ctoken[i++] = (char)cch;
            cch = fgetc(fin);
        }
        if (cch == q) cch = fgetc(fin);
        ctoken[i++] = (char)q;
        ctoken[i] = '\0';
        symbol = (q == '"') ? TSTRING : TCHAR;
        return;
    }

    /* Symbols */
    if (cch == '(') {
        cch = fgetc(fin);
        if (cch == '*') {
            cch = fgetc(fin);
            level = 1;
            while (level > 0 && cch != EOF) {
                if (cch == '(') {
                    cch = fgetc(fin);
                    if (cch == '*') {
                        level++;
                        cch = fgetc(fin);
                    }
                } else if (cch == '*') {
                    cch = fgetc(fin);
                    if (cch == ')') {
                        level--;
                        cch = fgetc(fin);
                    }
                } else {
                    cch = fgetc(fin);
                }
            }
            next();
            return;
        }
        symbol = TLPAREN;
    } else if (cch == ')') {
        cch = fgetc(fin);
        symbol = TRPAREN;
    } else if (cch == '[') {
        cch = fgetc(fin);
        symbol = TLBRACK;
    } else if (cch == ']') {
        cch = fgetc(fin);
        symbol = TRBRACK;
    } else if (cch == ';') {
        cch = fgetc(fin);
        symbol = TSEMICOL;
    } else if (cch == ',') {
        cch = fgetc(fin);
        symbol = TCOMMA;
    } else if (cch == '.') {
        cch = fgetc(fin);
        symbol = TDOT;
    } else if (cch == '=') {
        cch = fgetc(fin);
        symbol = TEQ;
    } else if (cch == '#') {
        cch = fgetc(fin);
        symbol = TNEQ;
    } else if (cch == '+') {
        cch = fgetc(fin);
        symbol = TPLUS;
    } else if (cch == '-') {
        cch = fgetc(fin);
        symbol = TMINUS;
    } else if (cch == '*') {
        cch = fgetc(fin);
        symbol = TMUL;
    } else if (cch == '&') {
        cch = fgetc(fin);
        symbol = TAND;
    } else if (cch == ':') {
        cch = fgetc(fin);
        if (cch == '=') {
            cch = fgetc(fin);
            symbol = TASSIGN;
        } else {
            symbol = TCOLON;
        }
    } else if (cch == '<') {
        cch = fgetc(fin);
        if (cch == '=') {
            cch = fgetc(fin);
            symbol = TLTE;
        } else {
            symbol = TLT;
        }
    } else if (cch == '>') {
        cch = fgetc(fin);
        if (cch == '=') {
            cch = fgetc(fin);
            symbol = TGTE;
        } else {
            symbol = TGT;
        }
    } else {
        printf("Lexer error: Unknown char '%c'\n", cch);
        cleanup_files();
        exit(1);
    }
}

void consume_id(char *buf) {
    strncpy(buf, ctoken, MAXIDLEN - 1);
    buf[MAXIDLEN - 1] = '\0';
    match(TIDENT, "Identifier expected");
}

void designator(void) {
    char mname[MAXIDLEN];
    char name[MAXIDLEN];

    strcpy(mname, modName);

    if(stab_ftype == TSYMAMOD) {
        stab_finded = stab_id[stab_finded];
        stab_fname = &stab_nbuf[stab[stab_finded]];
        stab_ftype = stab_type[stab_finded];
    }

    if((stab_ftype == TSYMTHISMOD) || (stab_ftype == TSYMIMOD)) {
        strcpy(mname, stab_fname);
        match(TIDENT, "module name expected");
        match(TDOT, ". expected");
    }

    consume_id(name);

    fprintf(fc, "%s_%s", mname, name);

    if(isLex(TLBRACK)) {
        emit("[");
        expr();
        match(TRBRACK, "] expected");
        emit("]");
    }
}

void params(void) {
    match(TLPAREN, "( expected");
    emit("(");
    if (symbol != TRPAREN) {
        do {
            expr();
            if (isLex(TCOMMA)) emit(", ");
            else break;
        } while (1);
    }
    match(TRPAREN, ") expected");
    emit(")");
}

void factor(void) {
    if (symbol == TNUMBER || symbol == TCHAR || symbol == TSTRING) {
        emit(ctoken);
        next();
    } else if (isLex(TLPAREN)) {
        emit("(");
        expr();
        match(TRPAREN, ") expected");
        emit(")");
    } else if (symbol == TIDENT) {
        designator();
        if (symbol == TLPAREN) params();
    } else {
        error("Factor expected");
    }
}

void term(void) {
    emit("(");
    factor();
    while (symbol >= TMUL && symbol <= TAND) {
        emit(get_op_str(symbol));
        next();
        factor();
    }
    emit(")");
}

void simple_expr(void) {
    if (isLex(TPLUS)) { /* unary plus ignored */ }
    else if (isLex(TMINUS)) emit("-");

    term();
    while (symbol >= TPLUS && symbol <= TOR) {
        emit(get_op_str(symbol));
        next();
        term();
    }
}

void expr(void) {
    emit("(");
    simple_expr();
    emit(")");
    if (symbol >= TEQ && symbol <= TGTE) {
        emit(get_op_str(symbol));
        next();
        emit("(");
        simple_expr();
        emit(")");
    }
}

void parse_basic_type(char *prefix, char *suffix) {
    prefix[0] = 0;
    suffix[0] = 0;

    if (symbol == TIDENT) {
        strcpy(prefix, ctoken);
        next();
    }
    else if (isLex(TTYPEINT)) {
        strcpy(prefix, "int");
    }
    else if (isLex(TTYPELONG)) {
        strcpy(prefix, "long");
    }
    else if (isLex(TTYPEREAL)) {
        strcpy(prefix, "float");
    }
    else if (isLex(TTYPEDBL)) {
        strcpy(prefix, "double");
    }
    else if (isLex(TTYPEBOOL)) {
        strcpy(prefix, "char");
    }
    else if (isLex(TTYPECHAR)) {
        strcpy(prefix, "char");
    }
    else {
        error("Type expected");
    }
}

void parse_type(char *prefix, char *suffix) {
    char size[MAXIDLEN];
    prefix[0] = 0;
    suffix[0] = 0;

    if (isLex(TPOINTER)) {
        if (isLex(TTO)) {
            parse_basic_type(prefix, suffix);
            strcat(prefix, " *");
        } else {
            strcpy(prefix, "void *");
        }
    }
    else if (isLex(TARRAY)) {
        if (symbol == TNUMBER || symbol == TIDENT) {
            strcpy(size, ctoken);
            next();
        } else error("Array size expected");

        match(TOF, "OF expected");

        if (symbol == TARRAY) {
            error("Multidimensional arrays not supported");
        }

        parse_basic_type(prefix, suffix);

        strcpy(suffix, "[");
        strcat(suffix, size);
        strcat(suffix, "]");
    }
    else if (isLex(TPROC)) {
        strcpy(prefix, "void (*");
        strcpy(suffix, ")()");
    }
    else {
        parse_basic_type(prefix, suffix);
    }
}

void print_var(char *tgt, const char *name, const char *prefix, const char *suffix) {
    strcpy(tgt, prefix);
    strcat(tgt, " ");
    strcat(tgt, modName);
    strcat(tgt, "_");
    strcat(tgt, name);
    strcat(tgt, suffix);
}

void stmt(void) {
    if (symbol == TIDENT) {
        designator();
        if (isLex(TASSIGN)) {
            emit(" = ");
            expr();
        } else if (symbol == TLPAREN) {
            params();
        } else {
            emit("()");
        }
        emit(";\n");
    } else if (isLex(TIF)) {
        emit("if (");
        expr();
        emit(") {\n");
        match(TTHEN, "THEN expected");
        stmt_seq();
        while (isLex(TELSIF)) {
            emit("} else if (");
            expr();
            emit(") {\n");
            match(TTHEN, "THEN expected");
            stmt_seq();
        }
        if (isLex(TELSE)) {
            emit("} else {\n");
            stmt_seq();
        }
        match(TEND, "END expected");
        emit("}\n");
    } else if (isLex(TWHILE)) {
        emit("while (");
        expr();
        emit(") {\n");
        match(TDO, "DO expected");
        stmt_seq();
        match(TEND, "END expected");
        emit("}\n");
    } else if (isLex(TREPEAT)) {
        emit("do {\n");
        stmt_seq();
        emit("\n} while (!(\n");
        match(TUNTIL, "UNTIL expected");
        expr();
        emit("));\n");
    } else if (isLex(TRETURN)) {
        emit("return ");
        if (symbol != TSEMICOL && symbol != TEND && symbol != TELSE && symbol != TELSIF) {
            expr();
        }
        emit(";\n");
    } else if (symbol == TINC || symbol == TDEC) {
        int is_inc = (symbol == TINC);
        next();
        match(TLPAREN, "( expected");
        designator();
        if (isLex(TCOMMA)) {
            emit(is_inc ? " += " : " -= ");
            expr();
        } else {
            emit(is_inc ? "++" : "--");
        }
        match(TRPAREN, ") expected");
        emit(";\n");
    } else if (isLex(TBREAK)) {
        emit("break;\n");
    } else if (isLex(TCONT)) {
        emit("continue;\n");
    }
}

void stmt_seq(void) {
    while (symbol != TEND && symbol != TELSIF && symbol != TELSE) {
        stmt();
        isLex(TSEMICOL);
    }
}

void var_decl(void) {
    char idBuf[MAXIDLEN];
    int i, start_stab, isExp;
    char typePre[MAXTYPELEN], typeSuf[MAXTYPELEN];
    char declBuf[MAXTYPELEN + MAXIDLEN * 2];

    while (symbol == TIDENT) {
        start_stab = stab_ptr;
        do {
            consume_id(idBuf);
            isExp = isLex(TMUL);

            if (stab_find(idBuf)) error("Duplicate identifier");
            stab_add(idBuf, isExp, isExp ? TSYMGEVAR : TSYMGVAR);

        } while (isLex(TCOMMA));

        match(TCOLON, ": expected");
        parse_type(typePre, typeSuf);
        match(TSEMICOL, "; expected");

        for (i = start_stab; i < stab_ptr; i++) {
            print_var(declBuf, &stab_nbuf[stab[i]], typePre, typeSuf);

            if (isGlobalDef) {
                if (stab_type[i] == TSYMGEVAR) {
                    fprintf(fc, "%s;\n", declBuf);
                    fprintf(fh, "extern %s;\n", declBuf);
                } else if (stab_type[i] == TSYMGVAR) {
                    fprintf(fc, "static %s;\n", declBuf);
                }
            } else {
                fprintf(fc, "%s;\n", declBuf);
            }
        }
    }
}

void parse_proc_decl(int *saved_stab_ptr, int *saved_nbuf_ptr) {
    int exp = 0, i;
    int start_stab;

    g_argList[0] = 0;
    g_retPre[0] = 0;
    g_retSuf[0] = 0;
    strcpy(g_retPre, "void");

    consume_id(g_procName);
    exp = isLex(TMUL);

    stab_add(g_procName, 0, TSYMPROC);

    *saved_stab_ptr = stab_ptr;
    *saved_nbuf_ptr = stab_nbuf_ptr;

    if (isLex(TLPAREN)) {
        if (symbol != TRPAREN) {
            do {
                if (symbol == TVAR) {
                    error("VAR parameters not supported");
                }

                start_stab = stab_ptr;
                do {
                    consume_id(g_idBuf);
                    if (stab_find(g_idBuf)) error("Duplicate parameter");
                    stab_add(g_idBuf, 0, TSYMPARAM);
                } while (isLex(TCOMMA));

                match(TCOLON, ": expected");
                parse_type(g_pPre, g_pSuf);

                for (i = start_stab; i < stab_ptr; i++) {
                    if (strlen(g_argList) > 0) strcat(g_argList, ", ");
                    print_var(g_oneArg, &stab_nbuf[stab[i]], g_pPre, g_pSuf);
                    strcat(g_argList, g_oneArg);
                }
            } while (isLex(TSEMICOL));
        }
        match(TRPAREN, ") expected");
    }

    if (strlen(g_argList) == 0) strcpy(g_argList, "void");

    if (isLex(TCOLON)) {
        parse_type(g_retPre, g_retSuf);
    }
    match(TSEMICOL, "; expected");

    fprintf(fc, "\n%s%s %s_%s(%s)%s", exp ? "" : "static ", g_retPre, modName, g_procName, g_argList, g_retSuf);
    if (exp) fprintf(fh, "extern %s %s_%s(%s)%s;\n", g_retPre, modName, g_procName, g_argList, g_retSuf);
    emit(" {\n");
}

void proc_decl(void) {
    int old_stab_ptr, old_stab_nbuf_ptr;

    parse_proc_decl(&old_stab_ptr, &old_stab_nbuf_ptr);

    isGlobalDef = 0;
    while (isLex(TVAR)) var_decl();

    match(TBEGIN, "BEGIN expected");
    stmt_seq();
    match(TEND, "END expected");

    match(TIDENT, "Identifier expected");
    match(TSEMICOL, "; expected");

    emit("}\n");
    isGlobalDef = 1;

    stab_ptr = old_stab_ptr;
    stab_nbuf_ptr = old_stab_nbuf_ptr;
}

void const_decl(void) {
    char cName[MAXIDLEN];
    int isExp;
    while (symbol == TIDENT) {
        consume_id(cName);
        isExp = isLex(TMUL);
        match(TEQ, "= expected");
        stab_add(cName, 0, TSYMCONST);
        fprintf(isExp ? fh : fc, "#define\t%s_%s\t%s\n", modName, cName, ctoken);
        match(TNUMBER, "Number expected");
        match(TSEMICOL, "; expected");
    }
}


void mod_init() {
    curLine = 1;
    isGlobalDef = 0;

    stab_ptr = 0;
    stab_nbuf_ptr = 0;

    stab_add("MODULE", 0, TMODULE);
    stab_add("BEGIN", 0, TBEGIN);
    stab_add("END", 0, TEND);
    stab_add("IMPORT", 0, TIMPORT);
    stab_add("CONST", 0, TCONST);
    stab_add("VAR", 0, TVAR);
    stab_add("PROCEDURE", 0, TPROC);

    stab_add("IF", 0, TIF);
    stab_add("THEN", 0, TTHEN);
    stab_add("ELSIF", 0, TELSIF);
    stab_add("ELSE", 0, TELSE);
    stab_add("WHILE", 0, TWHILE);
    stab_add("DO", 0, TDO);
    stab_add("REPEAT", 0, TREPEAT);
    stab_add("UNTIL", 0, TUNTIL);
    stab_add("RETURN", 0, TRETURN);
    stab_add("ARRAY", 0, TARRAY);

    stab_add("OF", 0, TOF);
    stab_add("POINTER", 0, TPOINTER);
    stab_add("TO", 0, TTO);
    stab_add("INC", 0, TINC);
    stab_add("DEC", 0, TDEC);
    stab_add("BREAK", 0, TBREAK);
    stab_add("CONTINUE", 0, TCONT);

    stab_add("OR", 0, TOR);
    stab_add("DIV", 0, TDIV);
    stab_add("MOD", 0, TMOD);
    stab_add("TRUE", 0, TTRUE);
    stab_add("FALSE", 0, TFALSE);
    stab_add("NIL", 0, TNIL);

    stab_add("INTEGER", 0, TTYPEINT);
    stab_add("LONGINT", 0, TTYPELONG);
    stab_add("REAL", 0, TTYPEREAL);
    stab_add("LONGREAL", 0, TTYPEDBL);
    stab_add("BOOLEAN", 0, TTYPEBOOL);
    stab_add("CHAR", 0, TTYPECHAR);

    fin = fh = fc = NULL;
}

void module(char *src) {
    char *dot;
    int len;
    int modId, modAliasId;

    mod_init();

    strcpy(sourceFileName, src);
    fin = fopen(sourceFileName, "r");
    if (!fin) {
        printf("Error: Cannot open %s\n", sourceFileName);
        exit(1);
    }

    dot = strrchr(sourceFileName, '.');
    len = dot ? (int)(dot - sourceFileName) : (int)strlen(sourceFileName);
    strncpy(outNameC, sourceFileName, len);
    outNameC[len] = '\0';
    strcpy(outNameHeader, outNameC);
    strcat(outNameC, ".c");
    strcat(outNameHeader, ".h");

    fc = fopen(outNameC, "w");
    fh = fopen(outNameHeader, "w");
    if (!fc || !fh) error("Cannot create output files");

    cch = fgetc(fin);
    next();
    match(TMODULE, "MODULE expected");
    consume_id(modName);
    stab_add(modName, 0, TSYMTHISMOD);
    match(TSEMICOL, "; expected");

    fprintf(fh, "#ifndef %s_H\n#define %s_H\n\n#include <stdint.h>\n\n", modName, modName);
    isGlobalDef = 1;

    while (isLex(TIMPORT)) {
        do {
            if (symbol == TIDENT) {
                modId = stab_add(ctoken, 0, TSYMIMOD);
                next();
                if(isLex(TASSIGN)) {
                    modAliasId = modId;
                    modId = stab_add(ctoken, 0, TSYMIMOD);
                    stab_type[modAliasId] = TSYMAMOD;
                    stab_id[modAliasId] = modId;
                    match(TIDENT, "module name expected");
                }
                fprintf(fc, "#include \"%s.h\"\n", stab_nbuf[stab[modId]]);
            }
        } while (isLex(TCOMMA));
        match(TSEMICOL, "; expected");
    }

    fprintf(fc, "#include \"%s\"\n\n", outNameHeader);

    while (1) {
        if (isLex(TCONST)) {
            const_decl();
        } else if (isLex(TVAR)) {
            var_decl();
        } else if(isLex(TPROC)) {
            proc_decl();
        } else {
            break;
        }
    }

    fprintf(fc, "\nstatic char is_%s_init = 0;\n", modName);
    fprintf(fc, "void mod_%s_init() {\n", modName);
    fprintf(fc, "if(is_%s_init) {\nreturn;\n}\nis_%s_init = 1;\n", modName, modName);
    if(isLex(TBEGIN)) {
        stmt_seq();
    }
    emit("}\n");
    fprintf(fh, "\nextern void mod_%s_init();\n", modName);

    match(TEND, "END expected");
    match(TIDENT, "Identifier expected");
    match(TDOT, ". expected");
    match(TEOF, "EOF expected");

    fprintf(fh, "\n#endif\n", modName);
    cleanup_files();
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
                module(argv[i]);
            }
        }
    }
    return 0;
}
