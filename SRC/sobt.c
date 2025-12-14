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
#define MAX_ID_LEN      64
#define MAX_TYPE_LEN    32
#define MAX_FNAME_LEN   256
#define STAB_SIZE       512
#define STAB_BUF_SIZE   (16 * 1024)

/* -- Tokens -- */
#define T_NULL      0
#define T_MODULE    1
#define T_BEGIN     2
#define T_END       3
#define T_IMPORT    4
#define T_CONST     5
#define T_VAR       6
#define T_PROC      7
#define T_IF        8
#define T_THEN      9
#define T_ELSIF     10
#define T_ELSE      11
#define T_WHILE     12
#define T_DO        13
#define T_REPEAT    14
#define T_UNTIL     15
#define T_RETURN    16
#define T_ARRAY     17
#define T_OF        18
#define T_POINTER   19
#define T_TO        20
#define T_INC       21
#define T_DEC       22
#define T_BREAK     23
#define T_CONT      24
#define T_ADR       25

/* -- Built-in Types (100-110) -- */
#define T_TYPE_INT      100
#define T_TYPE_LONG     101
#define T_TYPE_REAL     102
#define T_TYPE_DBL      103
#define T_TYPE_BOOL     104
#define T_TYPE_CHAR     105

#define T_IDENT     50
#define T_NUMBER    51
#define T_CHAR      52
#define T_STRING    53

/* -- Operators -- */
#define T_ASSIGN    60 /* := */
#define T_EQ        61 /* = */
#define T_NEQ       62 /* # */
#define T_LT        63 /* < */
#define T_LTE       64 /* <= */
#define T_GT        65 /* > */
#define T_GTE       66 /* >= */

#define T_PLUS      70 /* + */
#define T_MINUS     71 /* - */
#define T_OR        72 /* OR */
#define T_MUL       73 /* * */
#define T_DIV       74 /* DIV */
#define T_MOD       75 /* MOD */
#define T_AND       76 /* & */
#define T_SHL       77 /* SHL */
#define T_SHR       78 /* SHR */

#define T_LPAREN    80 /* ( */
#define T_RPAREN    81 /* ) */
#define T_LBRACK    82 /* [ */
#define T_RBRACK    83 /* ] */
#define T_COMMA     84 /* , */
#define T_COLON     85 /* : */
#define T_SEMICOL   86 /* ; */
#define T_DOT       87 /* . */
#define T_EOF       99
#define T_NIL       103
#define T_TRUE      101
#define T_FALSE     102

/* -- Symbol Table Types -- */
#define T_SYM_THISMOD  200
#define T_SYM_IMOD     201
#define T_SYM_AMOD     202

#define T_SYM_CONST    300
#define T_SYM_PROC     301
#define T_SYM_GVAR     302
#define T_SYM_GEVAR    303
#define T_SYM_PARAM    304

/* -- Globals -- */
FILE *fin = NULL;
FILE *fc = NULL;
FILE *fh = NULL;

int curLine = 1;
int cch;
int symbol = 0;
char ctoken[MAX_ID_LEN];
char modName[MAX_ID_LEN];

char sourceFileName[MAX_FNAME_LEN];
char outNameC[MAX_FNAME_LEN];
char outNameHeader[MAX_FNAME_LEN];

int isGlobalDef;

char g_procName[MAX_ID_LEN];
char g_argList[4096];
char g_oneArg[MAX_TYPE_LEN + MAX_ID_LEN * 2];
char g_retPre[MAX_TYPE_LEN];
char g_retSuf[MAX_TYPE_LEN]; 
char g_pPre[MAX_TYPE_LEN];
char g_pSuf[MAX_TYPE_LEN];
char g_idBuf[MAX_ID_LEN];

/* -- Symbol Table Data -- */
int stab[STAB_SIZE];
int stab_type[STAB_SIZE];
int stab_id[STAB_SIZE];

int stab_finded, stab_fid, stab_ftype;
char *stab_fname;

int stab_ptr;
char stab_nbuf[STAB_BUF_SIZE];
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

void consume_id(char *buf) {
    strncpy(buf, ctoken, MAX_ID_LEN - 1);
    buf[MAX_ID_LEN - 1] = '\0';
    match(T_IDENT, "Identifier expected");
}

const char* get_op_str(int t) {
    switch(t) {
    case T_ASSIGN:
        return " = ";
    case T_EQ:
        return " == ";
    case T_NEQ:
        return " != ";
    case T_LT:
        return " < ";
    case T_LTE:
        return " <= ";
    case T_GT:
        return " > ";
    case T_GTE:
        return " >= ";
    case T_PLUS:
        return " + ";
    case T_MINUS:
        return " - ";
    case T_OR:
        return " || ";
    case T_MUL:
        return " * ";
    case T_DIV:
        return " / ";
    case T_MOD:
        return " % ";
    case T_AND:
        return " && ";
    case T_SHL:
        return " << ";
    case T_SHR:
        return " >> ";
    }
    return NULL;
}

/* -- Symbol Table Functions -- */

int stab_add(char *name, int sid, int stype) {
    if (stab_ptr >= STAB_SIZE) error("Symbol table full");
    if (stab_nbuf_ptr + strlen(name) + 1 >= STAB_BUF_SIZE) error("Symbol table name buffer full");

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
        symbol = T_EOF;
        return;
    }

    /* Identifiers, Keywords, and Types */
    if (isalpha(cch)) {
        i = 0;
        while (isalnum(cch)) {
            if (i < MAX_ID_LEN - 1) ctoken[i++] = (char)cch;
            cch = fgetc(fin);
        }
        ctoken[i] = 0;
        symbol = T_IDENT;

        if (stab_find(ctoken)) {
            if (stab_ftype < 200) {
                symbol = stab_ftype;
                if (symbol == T_TRUE) {
                    symbol = T_NUMBER;
                    strcpy(ctoken, "1");
                } else if (symbol == T_FALSE) {
                    symbol = T_NUMBER;
                    strcpy(ctoken, "0");
                } else if (symbol == T_NIL) {
                    symbol = T_NUMBER;
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
            if (i < MAX_ID_LEN - 2) {
                ctoken[i++]='0';
                ctoken[i++]='x';
            }
        }
        while ((isHex && isxdigit(cch)) || (!isHex && isdigit(cch))) {
            if (i < MAX_ID_LEN - 1) ctoken[i++] = (char)cch;
            cch = fgetc(fin);
        }
        if(!isHex && (cch == '.')) {
            if (i < MAX_ID_LEN - 1) ctoken[i++] = (char)cch;
            cch = fgetc(fin);
            while (isdigit(cch)) {
                if (i < MAX_ID_LEN - 1) ctoken[i++] = (char)cch;
                cch = fgetc(fin);
            }
        }
        ctoken[i] = 0;
        symbol = T_NUMBER;
        return;
    }

    /* Strings / Chars */
    if (cch == '"' || cch == '\'') {
        q = cch;
        i = 0;
        ctoken[i++] = (char)q;
        cch = fgetc(fin);
        while (cch != q && cch != EOF) {
            if (i < MAX_ID_LEN - 2) ctoken[i++] = (char)cch;
            cch = fgetc(fin);
        }
        if (cch == q) cch = fgetc(fin);
        ctoken[i++] = (char)q;
        ctoken[i] = '\0';
        symbol = (q == '"') ? T_STRING : T_CHAR;
        return;
    }

    /* Symbols */
    switch (cch) {
    case '(':
        cch = fgetc(fin);
        if (cch == '*') {
            cch = fgetc(fin);
            /* Nested comments support */
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
        symbol = T_LPAREN;
        break;
    case ')':
        cch = fgetc(fin);
        symbol = T_RPAREN;
        break;
    case '[':
        cch = fgetc(fin);
        symbol = T_LBRACK;
        break;
    case ']':
        cch = fgetc(fin);
        symbol = T_RBRACK;
        break;
    case ';':
        cch = fgetc(fin);
        symbol = T_SEMICOL;
        break;
    case ',':
        cch = fgetc(fin);
        symbol = T_COMMA;
        break;
    case '.':
        cch = fgetc(fin);
        symbol = T_DOT;
        break;
    case '=':
        cch = fgetc(fin);
        symbol = T_EQ;
        break;
    case '#':
        cch = fgetc(fin);
        symbol = T_NEQ;
        break;
    case '+':
        cch = fgetc(fin);
        symbol = T_PLUS;
        break;
    case '-':
        cch = fgetc(fin);
        symbol = T_MINUS;
        break;
    case '*':
        cch = fgetc(fin);
        symbol = T_MUL;
        break;
    case '&':
        cch = fgetc(fin);
        symbol = T_AND;
        break;
    case ':':
        cch = fgetc(fin);
        if (cch == '=') {
            cch = fgetc(fin);
            symbol = T_ASSIGN;
        }
        else symbol = T_COLON;
        break;
    case '<':
        cch = fgetc(fin);
        if (cch == '=') {
            cch = fgetc(fin);
            symbol = T_LTE;
        }
        else symbol = T_LT;
        break;
    case '>':
        cch = fgetc(fin);
        if (cch == '=') {
            cch = fgetc(fin);
            symbol = T_GTE;
        }
        else symbol = T_GT;
        break;
    default:
        printf("Lexer error: Unknown char '%c'\n", cch);
        cleanup_files();
        exit(1);
    }
}

void designator(void) {
    char name[MAX_ID_LEN];
    consume_id(name);

    if (isLex(T_DOT)) {
        if (symbol != T_IDENT) error("Field identifier expected");
        fprintf(fc, "%s_%s", name, ctoken);
        next();
    } else {
        fprintf(fc, "%s_%s", modName, name);
    }

    while (1) {
        if(isLex(T_LBRACK)) {
            emit("[");
            expr();
            match(T_RBRACK, "] expected");
            emit("]");
        } else if (isLex(T_DOT)) {
            emit(".");
            emit(ctoken);
            match(T_IDENT, "Ident expected");
        } else if (symbol == T_LPAREN) {
            break;
        } else {
            break;
        }
    }
}

void params(void) {
    match(T_LPAREN, "( expected");
    emit("(");
    if (symbol != T_RPAREN) {
        do {
            expr();
            if (isLex(T_COMMA)) emit(", ");
            else break;
        } while (1);
    }
    match(T_RPAREN, ") expected");
    emit(")");
}

void factor(void) {
    if (symbol == T_NUMBER || symbol == T_CHAR || symbol == T_STRING) {
        emit(ctoken);
        next();
    } else if (isLex(T_LPAREN)) {
        emit("(");
        expr();
        match(T_RPAREN, ") expected");
        emit(")");
    } else if (isLex(T_ADR)) {
        match(T_LPAREN, "( expected");
        emit("(&");
        designator();
        emit(")");
        match(T_RPAREN, ") expected");
    } else if (symbol == T_IDENT) {
        designator();
        if (symbol == T_LPAREN) params();
    } else {
        error("Factor expected");
    }
}

void term(void) {
    emit("(");
    factor();
    while (symbol >= T_MUL && symbol <= T_SHR) {
        emit(get_op_str(symbol));
        next();
        factor();
    }
    emit(")");
}

void simple_expr(void) {
    if (isLex(T_PLUS)) { /* unary plus ignored */ }
    else if (isLex(T_MINUS)) emit("-");

    term();
    while (symbol >= T_PLUS && symbol <= T_OR) {
        emit(get_op_str(symbol));
        next();
        term();
    }
}

void expr(void) {
    emit("(");
    simple_expr();
    emit(")");
    if (symbol >= T_EQ && symbol <= T_GTE) {
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

    if (symbol == T_IDENT) {
        strcpy(prefix, ctoken);
        next();
    }
    else if (isLex(T_TYPE_INT)) {
        strcpy(prefix, "int");
    }
    else if (isLex(T_TYPE_LONG)) {
        strcpy(prefix, "long");
    }
    else if (isLex(T_TYPE_REAL)) {
        strcpy(prefix, "float");
    }
    else if (isLex(T_TYPE_DBL)) {
        strcpy(prefix, "double");
    }
    else if (isLex(T_TYPE_BOOL)) {
        strcpy(prefix, "char");
    }
    else if (isLex(T_TYPE_CHAR)) {
        strcpy(prefix, "char");
    }
    else {
        error("Type expected");
    }
}

void parse_type(char *prefix, char *suffix) {
    char size[MAX_ID_LEN];
    prefix[0] = 0;
    suffix[0] = 0;

    if (isLex(T_POINTER)) {
        if (isLex(T_TO)) {
            parse_basic_type(prefix, suffix);
            strcat(prefix, " *");
        } else {
            strcpy(prefix, "void *");
        }
    }
    else if (isLex(T_ARRAY)) {
        if (symbol == T_NUMBER || symbol == T_IDENT) {
            strcpy(size, ctoken);
            next();
        } else error("Array size expected");

        match(T_OF, "OF expected");

        if (symbol == T_ARRAY) {
            error("Multidimensional arrays not supported");
        }

        parse_basic_type(prefix, suffix);

        strcpy(suffix, "[");
        strcat(suffix, size);
        strcat(suffix, "]");
    }
    else if (isLex(T_PROC)) {
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
    if (symbol == T_IDENT) {
        designator();
        if (isLex(T_ASSIGN)) {
            emit(" = ");
            expr();
        } else if (symbol == T_LPAREN) {
            params();
        } else {
            emit("()");
        }
        emit(";\n");
    } else if (isLex(T_IF)) {
        emit("if (");
        expr();
        emit(") {\n");
        match(T_THEN, "THEN expected");
        stmt_seq();
        while (isLex(T_ELSIF)) {
            emit("} else if (");
            expr();
            emit(") {\n");
            match(T_THEN, "THEN expected");
            stmt_seq();
        }
        if (isLex(T_ELSE)) {
            emit("} else {\n");
            stmt_seq();
        }
        match(T_END, "END expected");
        emit("}\n");
    } else if (isLex(T_WHILE)) {
        emit("while (");
        expr();
        emit(") {\n");
        match(T_DO, "DO expected");
        stmt_seq();
        match(T_END, "END expected");
        emit("}\n");
    } else if (isLex(T_REPEAT)) {
        emit("do {\n");
        stmt_seq();
        emit("\n} while (!(\n");
        match(T_UNTIL, "UNTIL expected");
        expr();
        emit("));\n");
    } else if (isLex(T_RETURN)) {
        emit("return ");
        if (symbol != T_SEMICOL && symbol != T_END && symbol != T_ELSE && symbol != T_ELSIF) {
            expr();
        }
        emit(";\n");
    } else if (symbol == T_INC || symbol == T_DEC) {
        int is_inc = (symbol == T_INC);
        next();
        match(T_LPAREN, "( expected");
        designator();
        if (isLex(T_COMMA)) {
            emit(is_inc ? " += " : " -= ");
            expr();
        } else {
            emit(is_inc ? "++" : "--");
        }
        match(T_RPAREN, ") expected");
        emit(";\n");
    } else if (isLex(T_BREAK)) {
        emit("break;\n");
    } else if (isLex(T_CONT)) {
        emit("continue;\n");
    }
}

void stmt_seq(void) {
    while (symbol != T_END && symbol != T_ELSIF && symbol != T_ELSE) {
        stmt();
        isLex(T_SEMICOL);
    }
}

void var_decl(void) {
    char idBuf[MAX_ID_LEN];
    int i, start_stab, isExp;
    char typePre[MAX_TYPE_LEN], typeSuf[MAX_TYPE_LEN];
    char declBuf[MAX_TYPE_LEN + MAX_ID_LEN * 2];

    while (symbol == T_IDENT) {
        start_stab = stab_ptr;
        do {
            consume_id(idBuf);
            isExp = isLex(T_MUL);

            if (stab_find(idBuf)) error("Duplicate identifier");
            stab_add(idBuf, isExp, isExp ? T_SYM_GEVAR : T_SYM_GVAR);

        } while (isLex(T_COMMA));

        match(T_COLON, ": expected");
        parse_type(typePre, typeSuf);
        match(T_SEMICOL, "; expected");

        for (i = start_stab; i < stab_ptr; i++) {
            print_var(declBuf, &stab_nbuf[stab[i]], typePre, typeSuf);

            if (isGlobalDef) {
                if (stab_type[i] == T_SYM_GEVAR) {
                    fprintf(fc, "%s;\n", declBuf);
                    fprintf(fh, "extern %s;\n", declBuf);
                } else if (stab_type[i] == T_SYM_GVAR) {
                    fprintf(fc, "static %s;\n", declBuf);
                }
            } else {
                /* Local variable */
                fprintf(fc, "%s;\n", declBuf);
            }
        }
    }
}

void parse_proc_decl(int *saved_stab_ptr, int *saved_nbuf_ptr) {
    /* Using globals g_... defined at top to save stack */
    int exp = 0, i;
    int start_stab;

    g_argList[0] = 0;
    g_retPre[0] = 0; 
    g_retSuf[0] = 0;
    strcpy(g_retPre, "void");

    consume_id(g_procName);
    exp = isLex(T_MUL);

    stab_add(g_procName, 0, T_SYM_PROC);

    *saved_stab_ptr = stab_ptr;
    *saved_nbuf_ptr = stab_nbuf_ptr;

    if (isLex(T_LPAREN)) {
        if (symbol != T_RPAREN) {
            do {
                if (symbol == T_VAR) {
                    error("VAR parameters not supported");
                }

                start_stab = stab_ptr;
                do {
                    consume_id(g_idBuf);
                    if (stab_find(g_idBuf)) error("Duplicate parameter");
                    stab_add(g_idBuf, 0, T_SYM_PARAM);
                } while (isLex(T_COMMA));

                match(T_COLON, ": expected");
                parse_type(g_pPre, g_pSuf);

                for (i = start_stab; i < stab_ptr; i++) {
                    if (strlen(g_argList) > 0) strcat(g_argList, ", ");
                    print_var(g_oneArg, &stab_nbuf[stab[i]], g_pPre, g_pSuf);
                    strcat(g_argList, g_oneArg);
                }
            } while (isLex(T_SEMICOL));
        }
        match(T_RPAREN, ") expected");
    }

    if (strlen(g_argList) == 0) strcpy(g_argList, "void");

    if (isLex(T_COLON)) {
        parse_type(g_retPre, g_retSuf);
    }
    match(T_SEMICOL, "; expected");

    fprintf(fc, "\n%s%s %s_%s(%s)%s", exp ? "" : "static ", g_retPre, modName, g_procName, g_argList, g_retSuf);
    if (exp) fprintf(fh, "extern %s %s_%s(%s)%s;\n", g_retPre, modName, g_procName, g_argList, g_retSuf);
    emit(" {\n");
}

void proc_decl(void) {
    int old_stab_ptr, old_stab_nbuf_ptr;

    parse_proc_decl(&old_stab_ptr, &old_stab_nbuf_ptr);

    isGlobalDef = 0;
    while (isLex(T_VAR)) var_decl();

    match(T_BEGIN, "BEGIN expected");
    stmt_seq();
    match(T_END, "END expected");

    match(T_IDENT, "Identifier expected");
    match(T_SEMICOL, "; expected");

    emit("}\n");
    isGlobalDef = 1;

    stab_ptr = old_stab_ptr;
    stab_nbuf_ptr = old_stab_nbuf_ptr;
}

void const_decl(void) {
    char cName[MAX_ID_LEN];
    int isExp;
    while (symbol == T_IDENT) {
        consume_id(cName);
        isExp = isLex(T_MUL);
        match(T_EQ, "= expected");
        stab_add(cName, 0, T_SYM_CONST);
        fprintf(isExp ? fh : fc, "#define\t%s_%s\t%s\n", modName, cName, ctoken);
        match(T_NUMBER, "Number expected");
        match(T_SEMICOL, "; expected");
    }
}

void module(char *src) {
    char *dot;
    int len;

    curLine = 1;
    isGlobalDef = 0;

    stab_ptr = 0;
    stab_nbuf_ptr = 0;

    stab_add("MODULE", 0, T_MODULE);
    stab_add("BEGIN", 0, T_BEGIN);
    stab_add("END", 0, T_END);
    stab_add("IMPORT", 0, T_IMPORT);
    stab_add("CONST", 0, T_CONST);
    stab_add("VAR", 0, T_VAR);
    stab_add("PROCEDURE", 0, T_PROC);

    stab_add("IF", 0, T_IF);
    stab_add("THEN", 0, T_THEN);
    stab_add("ELSIF", 0, T_ELSIF);
    stab_add("ELSE", 0, T_ELSE);
    stab_add("WHILE", 0, T_WHILE);
    stab_add("DO", 0, T_DO);
    stab_add("REPEAT", 0, T_REPEAT);
    stab_add("UNTIL", 0, T_UNTIL);
    stab_add("RETURN", 0, T_RETURN);
    stab_add("ARRAY", 0, T_ARRAY);

    stab_add("OF", 0, T_OF);
    stab_add("POINTER", 0, T_POINTER);
    stab_add("TO", 0, T_TO);
    stab_add("INC", 0, T_INC);
    stab_add("DEC", 0, T_DEC);
    stab_add("BREAK", 0, T_BREAK);
    stab_add("CONTINUE", 0, T_CONT);
    stab_add("Adr", 0, T_ADR);

    stab_add("OR", 0, T_OR);
    stab_add("DIV", 0, T_DIV);
    stab_add("MOD", 0, T_MOD);
    stab_add("TRUE", 0, T_TRUE);
    stab_add("FALSE", 0, T_FALSE);
    stab_add("SHL", 0, T_SHL);
    stab_add("SHR", 0, T_SHR);
    stab_add("NIL", 0, T_NIL);

    stab_add("INTEGER", 0, T_TYPE_INT);
    stab_add("LONGINT", 0, T_TYPE_LONG);
    stab_add("REAL", 0, T_TYPE_REAL);
    stab_add("LONGREAL", 0, T_TYPE_DBL);
    stab_add("BOOLEAN", 0, T_TYPE_BOOL);
    stab_add("CHAR", 0, T_TYPE_CHAR);

    fin = fh = fc = NULL;

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
    match(T_MODULE, "MODULE expected");
    consume_id(modName);
    stab_add(modName, 0, T_SYM_THISMOD);
    match(T_SEMICOL, "; expected");

    fprintf(fh, "#ifndef %s_H\n#define %s_H\n\n#include <stdint.h>\n\n", modName, modName);
    isGlobalDef = 1;

    while (isLex(T_IMPORT)) {
        do {
            if (symbol == T_IDENT) {
                fprintf(fc, "#include \"%s.h\"\n", ctoken);
                stab_add(ctoken, 0, T_SYM_IMOD);
                next();
            }
        } while (isLex(T_COMMA));
        match(T_SEMICOL, "; expected");
    }

    fprintf(fc, "#include \"%s\"\n\n", outNameHeader);

    while (1) {
        if (isLex(T_CONST)) {
            const_decl();
        } else if (isLex(T_VAR)) {
            var_decl();
        } else if(isLex(T_PROC)) {
            proc_decl();
        } else {
            break;
        }
    }

    fprintf(fc, "\nstatic char is_%s_init = 0;\n", modName);
    fprintf(fc, "void mod_%s_init() {\n", modName);
    fprintf(fc, "if(is_%s_init) {\nreturn;\n}\nis_%s_init = 1;\n", modName, modName);
    if(isLex(T_BEGIN)) {
        stmt_seq();
    }
    emit("}\n");
    fprintf(fh, "\nextern void mod_%s_init();\n", modName);

    match(T_END, "END expected");
    match(T_IDENT, "Identifier expected");
    match(T_DOT, ". expected");
    match(T_EOF, "EOF expected");

    fprintf(fh, "\n#endif\n", modName);
    cleanup_files();
}

int main(int argc, char **argv) {
    int i;
    if (argc == 1) {
        printf("Usage: %s filename.mod\n", argv[0]);
        return 1;
    }
    for(i = 1; i < argc; i++) {
        if (strlen(argv[i]) < MAX_FNAME_LEN) {
            if(argv[i][0] != '-') {
                module(argv[i]);
            }
        }
    }
    return 0;
}
