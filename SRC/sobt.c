/*
 Small Oberon to C99 Translator (Fixed)

 Original by DosWorld (CC0 1.0).
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_ID_LEN  64
#define MAX_FNAME_LEN  256
#define MAX_VARS    64

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

#define T_IDENT     50
#define T_NUMBER    51
#define T_CHAR      52
#define T_STRING    53

/* Operators */
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

#define STAB_SIZE 128

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
int g_ch;
int symbol = 0;
char g_id[MAX_ID_LEN];
char modName[MAX_ID_LEN];

char sourceFileName[MAX_FNAME_LEN], outNameC[MAX_FNAME_LEN], outNameHeader[MAX_FNAME_LEN];

int isGlobalDef;
int isExportDef;

/* -- Keyword Mapping -- */
#define KW_COUNT 33

const char *kw_str[] = {
    "MODULE", "BEGIN", "END", "IMPORT", "CONST", "VAR", "PROCEDURE",
    "IF", "THEN", "ELSIF", "ELSE", "WHILE", "DO", "REPEAT", "UNTIL", "RETURN", "ARRAY",
    "OF", "POINTER", "TO", "INC", "DEC", "BREAK", "CONTINUE", "Adr",
    "OR", "DIV", "MOD", "TRUE", "FALSE", "SHL", "SHR", "NIL"
};

const int kw_tok[] = {
    T_MODULE, T_BEGIN, T_END, T_IMPORT, T_CONST, T_VAR, T_PROC,
    T_IF, T_THEN, T_ELSIF, T_ELSE, T_WHILE, T_DO, T_REPEAT, T_UNTIL, T_RETURN, T_ARRAY,
    T_OF, T_POINTER, T_TO, T_INC, T_DEC, T_BREAK, T_CONT, T_ADR,
    T_OR, T_DIV, T_MOD, T_NUMBER, T_NUMBER, T_SHL, T_SHR, T_NUMBER
};

int stab[STAB_SIZE], stab_type[STAB_SIZE], stab_id[STAB_SIZE];
int stab_finded, stab_fid, stab_ftype;
char *stab_fname;

int stab_ptr;
char stab_nbuf[8*1024];
int stab_nbuf_ptr;

void next(void);
void expr(void);
void stat_seq(void);

void cleanup_files() {
    if (fc) fclose(fc);
    if (fh) fclose(fh);
    if (fin) fclose(fin);
    remove(outNameHeader);
    remove(outNameC);
}

void error(const char *msg) {
    printf("%s:%d: %s\n", sourceFileName, curLine, msg);
    cleanup_files();
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
    strncpy(buf, g_id, MAX_ID_LEN - 1);
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

int stab_add(char *name, int sid, int stype) {
    stab[stab_ptr] = stab_nbuf_ptr;
    strcpy(&stab_nbuf[stab_nbuf_ptr], name);
    stab_nbuf_ptr += strlen(name) + 1;
    stab_type[stab_ptr] = stype;
    stab_id[stab_ptr] = sid;
    stab_ptr++;
    return stab_ptr - 1;
}

int stab_find(char *name) {
    stab_fid = -1;
    stab_fname = NULL;
    stab_finded = 0;
    stab_ftype = -1;
    while(stab_finded < stab_ptr) {
        if(strcmp(&stab_nbuf[stab[stab_finded]], name) == 0) {
            stab_fname = &stab_nbuf[stab[stab_finded]];
            stab_fid = stab_id[stab_finded];
            stab_ftype = stab_type[stab_finded];
            return 1;
        }
        stab_finded++;
    }
    stab_finded = -1;
    return 0;
}

/* -- Lexer -- */

void next(void) {
    int i = 0, k = 0, q = 0, level = 0, isHex;
    while (isspace(g_ch)) {
        if (g_ch == '\n') curLine++;
        g_ch = fgetc(fin);
    }

    if (g_ch == EOF) {
        symbol = T_EOF;
        return;
    }

    /* Identifiers */
    if (isalpha(g_ch)) {
        i = 0;
        while (isalnum(g_ch)) {
            if (i < MAX_ID_LEN - 1) g_id[i++] = (char)g_ch;
            g_ch = fgetc(fin);
        }
        g_id[i] = 0;
        symbol = T_IDENT;

        for (k = 0; k < KW_COUNT; k++) {
            if (strcmp(g_id, kw_str[k]) == 0) {
                symbol = kw_tok[k];
                if (strcmp(g_id, "TRUE") == 0) strcpy(g_id, "1");
                else if (strcmp(g_id, "FALSE") == 0) strcpy(g_id, "0");
                else if (strcmp(g_id, "NIL") == 0) strcpy(g_id, "NULL");
                return;
            }
        }
        return;
    }

    if (isdigit(g_ch) || g_ch == '$') {
        isHex = (g_ch == '$');
        if (isHex) g_ch = fgetc(fin);
        i = 0;
        if (isHex) {
            g_id[i++]='0';
            g_id[i++]='x';
        }
        while ((isHex && isxdigit(g_ch)) || (!isHex && isdigit(g_ch))) {
            if (i < 127) g_id[i++] = (char)g_ch;
            g_ch = fgetc(fin);
        }
        if(!isHex && (g_ch == '.')) {
            if (i < 127) g_id[i++] = (char)g_ch;
            g_ch = fgetc(fin);
            while (isdigit(g_ch)) {
                if (i < 127) g_id[i++] = (char)g_ch;
                g_ch = fgetc(fin);
            }
        }
        g_id[i] = 0;
        symbol = T_NUMBER;
        return;
    }

    if (g_ch == '"' || g_ch == '\'') {
        q = g_ch;
        i = 0;
        g_id[i++] = (char)q;
        g_ch = fgetc(fin);
        while (g_ch != q && g_ch != EOF) {
            if (i < MAX_ID_LEN - 2) g_id[i++] = (char)g_ch;
            g_ch = fgetc(fin);
        }
        if (g_ch == q) g_ch = fgetc(fin);
        g_id[i++] = (char)q;
        g_id[i] = '\0';
        symbol = (q == '"') ? T_STRING : T_CHAR;
        return;
    }

    switch (g_ch) {
    case '(':
        g_ch = fgetc(fin);
        if (g_ch == '*') {
            g_ch = fgetc(fin);

            if (g_ch == '#') {
                g_ch = fgetc(fin);
                while (g_ch != EOF) {
                    if (g_ch == '*') {
                        g_ch = fgetc(fin);
                        if (g_ch == ')') {
                            g_ch = fgetc(fin);
                            break;
                        }
                        fputc('*', fh);
                    } else {
                        fputc(g_ch, fh);
                        g_ch = fgetc(fin);
                    }
                }
                next();
                return;
            } else if (g_ch == '{') {
                g_ch = fgetc(fin);
                while (g_ch != EOF) {
                    if (g_ch == '*') {
                        g_ch = fgetc(fin);
                        if (g_ch == ')') {
                            g_ch = fgetc(fin);
                            break;
                        }
                        fputc('*', fc);
                    } else {
                        fputc(g_ch, fc);
                        g_ch = fgetc(fin);
                    }
                }
                next();
                return;
            }

            level = 1;
            while (level > 0 && g_ch != EOF) {
                if (g_ch == '(') {
                    g_ch = fgetc(fin);
                    if (g_ch == '*') {
                        level++;
                        g_ch = fgetc(fin);
                    }
                } else if (g_ch == '*') {
                    g_ch = fgetc(fin);
                    if (g_ch == ')') {
                        level--;
                        g_ch = fgetc(fin);
                    }
                } else {
                    g_ch = fgetc(fin);
                }
            }
            next();
            return;
        }
        symbol = T_LPAREN;
        break;
    case ')':
        g_ch = fgetc(fin);
        symbol = T_RPAREN;
        break;
    case '[':
        g_ch = fgetc(fin);
        symbol = T_LBRACK;
        break;
    case ']':
        g_ch = fgetc(fin);
        symbol = T_RBRACK;
        break;
    case ';':
        g_ch = fgetc(fin);
        symbol = T_SEMICOL;
        break;
    case ',':
        g_ch = fgetc(fin);
        symbol = T_COMMA;
        break;
    case '.':
        g_ch = fgetc(fin);
        symbol = T_DOT;
        break;
    case '=':
        g_ch = fgetc(fin);
        symbol = T_EQ;
        break;
    case '#':
        g_ch = fgetc(fin);
        symbol = T_NEQ;
        break;
    case '+':
        g_ch = fgetc(fin);
        symbol = T_PLUS;
        break;
    case '-':
        g_ch = fgetc(fin);
        symbol = T_MINUS;
        break;
    case '*':
        g_ch = fgetc(fin);
        symbol = T_MUL;
        break;
    case '&':
        g_ch = fgetc(fin);
        symbol = T_AND;
        break;
    case ':':
        g_ch = fgetc(fin);
        if (g_ch == '=') {
            g_ch = fgetc(fin);
            symbol = T_ASSIGN;
        }
        else symbol = T_COLON;
        break;
    case '<':
        g_ch = fgetc(fin);
        if (g_ch == '=') {
            g_ch = fgetc(fin);
            symbol = T_LTE;
        }
        else symbol = T_LT;
        break;
    case '>':
        g_ch = fgetc(fin);
        if (g_ch == '=') {
            g_ch = fgetc(fin);
            symbol = T_GTE;
        }
        else symbol = T_GT;
        break;
    default:
        printf("Lexer error: Unknown char '%c'\n", g_ch);
        cleanup_files();
        exit(1);
    }
}

void designator(void) {
    char name[MAX_ID_LEN];
    consume_id(name);

    if (isLex(T_DOT)) {
        if (symbol != T_IDENT) error("Field identifier expected");
        fprintf(fc, "%s_%s", name, g_id);
        next();
    } else {
        fprintf(fc, "%s_%s", modName, name);
    }

    if(isLex(T_LBRACK)) {
        emit("[");
        expr();
        match(T_RBRACK, "] expected");
        emit("]");
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
        emit(g_id);
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
    if (!isLex(T_PLUS)) if (isLex(T_MINUS)) emit("-");

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

void type(char *prefix, char *suffix) {
    char size[64], subPre[64], subSuf[64];
    *prefix = '\0';
    *suffix = '\0';

    if (symbol == T_IDENT) {
        if (strcmp(g_id, "INTEGER") == 0) strcpy(prefix, "int");
        else if (strcmp(g_id, "LONGINT") == 0) strcpy(prefix, "long");
        else if (strcmp(g_id, "REAL") == 0) strcpy(prefix, "float");
        else if (strcmp(g_id, "LONGREAL") == 0) strcpy(prefix, "double");
        else if (strcmp(g_id, "BOOLEAN") == 0) strcpy(prefix, "int");
        else if (strcmp(g_id, "CHAR") == 0) strcpy(prefix, "char");
        else strcpy(prefix, g_id);
        next();
    } else if (isLex(T_POINTER)) {
        if (isLex(T_TO)) {
            type(prefix, suffix);
            strcat(prefix, "*");
        } else {
            strcpy(prefix, "void *");
        }
    } else if (isLex(T_ARRAY)) {
        if (symbol == T_NUMBER || symbol == T_IDENT) {
            strcpy(size, g_id);
            next();
        } else error("Array size expected");

        match(T_OF, "OF expected");

        if (symbol == T_ARRAY) error("Multidimensional arrays not supported");

        type(subPre, subSuf);
        strcpy(prefix, subPre);
        sprintf(suffix, "[%s]%s", size, subSuf);
    } else if (isLex(T_PROC)) {
        strcpy(prefix, "void (*");
        strcpy(suffix, ")()");
    } else {
        error("Type expected");
    }
}

void stat(void) {
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
        stat_seq();
        while (isLex(T_ELSIF)) {
            emit("} else if (");
            expr();
            emit(") {\n");
            match(T_THEN, "THEN expected");
            stat_seq();
        }
        if (isLex(T_ELSE)) {
            emit("} else {\n");
            stat_seq();
        }
        match(T_END, "END expected");
        emit("}\n");
    } else if (isLex(T_WHILE)) {
        emit("while (");
        expr();
        emit(") {\n");
        match(T_DO, "DO expected");
        stat_seq();
        match(T_END, "END expected");
        emit("}\n");
    } else if (isLex(T_REPEAT)) {
        emit("do {\n");
        stat_seq();
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

void stat_seq(void) {
    while (symbol != T_END && symbol != T_ELSIF && symbol != T_ELSE) {
        stat();
        isLex(T_SEMICOL);
    }
}

void var_decl(void) {
    char idBuf[MAX_ID_LEN];
    int i, start_stab, isExp;
    char tp[64], ts[64];

    while (symbol == T_IDENT) {
        start_stab = stab_ptr;
        do {
            consume_id(idBuf);

            if (isLex(T_MUL)) isExp = 1;
            else isExp = 0;

            if (stab_find(idBuf)) error("Duplicate identifier");
            stab_add(idBuf, isExp, isExp ? T_SYM_GEVAR : T_SYM_GVAR);

        } while (isLex(T_COMMA));

        match(T_COLON, ": expected");
        type(tp, ts);
        match(T_SEMICOL, "; expected");

        for (i = start_stab; i < stab_ptr; i++) {
            if (isGlobalDef) {
                if (stab_type[i] == T_SYM_GEVAR) {
                    fprintf(fc, "%s %s_%s%s;\n", tp, modName, &stab_nbuf[stab[i]], ts);
                    fprintf(fh, "extern %s %s_%s%s;\n", tp, modName, &stab_nbuf[stab[i]], ts);
                } else if (stab_type[i] == T_SYM_GVAR) {
                    fprintf(fc, "static %s %s_%s%s;\n", tp, modName, &stab_nbuf[stab[i]], ts);
                }
            } else {
                fprintf(fc, "%s %s_%s%s;\n", tp, modName, &stab_nbuf[stab[i]], ts);
            }
        }
    }
}

void proc_decl(void) {
    char procName[MAX_ID_LEN];
    char args[1024] = "";
    char retType[64] = "void";
    char pPre[64], pSuf[64], tmpParam[128], rSuf[64];
    char idBuf[MAX_ID_LEN];
    int exp = 0, i;
    int old_stab_ptr, old_stab_nbuf_ptr;
    int start_stab;

    consume_id(procName);
    exp = isLex(T_MUL);

    stab_add(procName, 0, T_SYM_PROC);

    old_stab_ptr = stab_ptr;
    old_stab_nbuf_ptr = stab_nbuf_ptr;

    if (isLex(T_LPAREN)) {
        if (symbol != T_RPAREN) {
            do {
                start_stab = old_stab_ptr;
                do {
                    consume_id(idBuf);
                    if (stab_find(idBuf)) error("Duplicate parameter");
                    stab_add(idBuf, 0, T_SYM_PARAM);
                } while (isLex(T_COMMA));

                match(T_COLON, ": expected");
                type(pPre, pSuf);

                for (i = start_stab; i < stab_ptr; i++) {
                    if (strlen(args) > 0) strcat(args, ", ");
                    sprintf(tmpParam, "%s %s_%s%s", pPre, modName, &stab_nbuf[stab[i]], pSuf);
                    strcat(args, tmpParam);
                }
            } while (isLex(T_SEMICOL));
        }
        match(T_RPAREN, ") expected");
    }

    if (strlen(args) == 0) strcpy(args, "void");

    if (isLex(T_COLON)) type(retType, rSuf);
    match(T_SEMICOL, "; expected");

    fprintf(fc, "\n%s%s %s_%s(%s)", exp ? "" : "static ", retType, modName, procName, args);
    if (exp) fprintf(fh, "extern %s %s_%s(%s);\n", retType, modName, procName, args);
    emit(" {\n");

    isGlobalDef = 0;
    while (isLex(T_VAR)) var_decl();

    match(T_BEGIN, "BEGIN expected");
    stat_seq();
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
        fprintf(isExp ? fh : fc, "#define\t%s_%s\t%s\n", modName, cName, g_id);
        match(T_NUMBER, "Number expected");
        match(T_SEMICOL, "; expected");
    }
}

void translate(char *src) {
    char *dot;
    int len;

    isGlobalDef = 0;
    isExportDef = 0;

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
    stab_add("TRUE", 0, T_NUMBER);
    stab_add("FALSE", 0, T_NUMBER);
    stab_add("SHL", 0, T_SHL);
    stab_add("SHR", 0, T_SHR);
    stab_add("NIL", 0, T_NUMBER);

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

    g_ch = fgetc(fin);
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
                fprintf(fc, "#include \"%s.h\"\n", g_id);
                stab_add(g_id, 0, T_SYM_IMOD);
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

    if(isLex(T_BEGIN)) {
        fprintf(fc, "\nvoid mod_%s_init() {\n", modName);
        stat_seq();
        emit("}\n");
        fprintf(fh, "\nextern void mod_%s_init();\n", modName);
    }

    match(T_END, "END expected");
    match(T_IDENT, "Identifier expected");
    match(T_DOT, ". expected");
    match(T_EOF, "EOF expected");

    fprintf(fh, "\n#endif\n", modName);
    fclose(fc);
    fclose(fh);
    fclose(fin);
}

int main(int argc, char **argv) {
    int i;
    if (argc == 1) {
        printf("Usage: %s filename.mod\n", argv[0]);
        return 1;
    }
    for(i = 1; i < argc; i++) {
        if (strlen(argv[i]) >= MAX_FNAME_LEN) {
            printf("Error: Filename %s is too long\n", argv[i]);
            return 1;
        }
        translate(argv[i]);
    }
    return 0;
}