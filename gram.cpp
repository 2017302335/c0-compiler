/*
1. return�����Ƿ���ȷ         OK
2. ���ú��������Ƿ���ȷ       OK
3. Ϊchar��ֵ�Ƿ���ȷ         OK
4. �Ƿ�return��               OK
5. �Ƿ���main����             OK
6. ��������ֺ�������������Լ��ڲ��ľֲ��������������   OK
7. ����Խ���ж�               OK
8. ���ú����Ƿ���ȷ           OK

��ǩ��[FUNCNAME]_func_[i]_[j]_[k]

*/

#include "lexical.h"
#include <cstdlib>
#include "vars.h"
#include "gram.h"
#include "item.h"
#include <iostream>

using namespace std;

Symbol symbol;
VAR_MAP global_vars;
CONST_MAP global_consts;
FUNC_MAP funcs;
string name;
FuncItem* this_func = NULL;
Type type;
bool returned;
bool skip_type_ident = false;

int const_value;
bool certain;

Type item(int*, bool*);
Type factor(int*, bool*);
Type expr(int*, bool*);

typedef enum {
    CONST_STATE, VAR_STATE, FUNC_STATE, FUNC_CONST_STATE, FUNC_VAR_STATE, FUNC_STATEMENT_STATE
}State;

void output_info(string info) {
    // cout << info << endl;
}

void record_name() {
    token[token_len] = 0;
    name = token;
}

void getsym_check() {
    if (!getsym()) {
        error("unfinished program");
    }
}

void mate(Symbol sym, void (*handle_ptr)() = NULL) {
    if (symbol != sym) {
        error((string)"got " + symbol2string(symbol) + " expected " + symbol2string(sym));
    } else {
        if ((*handle_ptr) != NULL) (*handle_ptr)();
        getsym_check();
    }
}

void mate_idents() {

}

Type expr(int* value, bool* certain) {
    Type ex_type;
    int cur_value;
    bool cur_certain;
    Symbol cur_op = ADD;
    *value = 0;
    *certain = true;
    if (symbol == ADD || symbol == SUB) {
        cur_op = symbol;
        ex_type = INT;
        getsym_check();
    }
    do {
        ex_type = item(&cur_value, &cur_certain);
        *certain &= cur_certain;
        if (certain) {
            switch (cur_op) {
            case ADD:
                *value += cur_value;
                break;
            case SUB:
                *value -= cur_value;
                break;
            default: error_debug("expr");
            }
        }
        if (symbol == ADD || symbol == SUB) {
            cur_op = symbol;
            ex_type = INT;
            getsym_check();
        } else {
            break;
        }
    } while (true);
    return ex_type;
}

Type item(int* value, bool* certain) {
    Type it_type;
    int cur_value;
    bool cur_certain;
    Symbol cur_op = MUL;
    *value = 1;
    *certain = true;
    do {
        it_type = factor(&cur_value, &cur_certain);
        *certain &= cur_certain;
        if (*certain) {
            switch(cur_op) {
            case MUL:
                *value *= cur_value;
                break;
            case DIV:
                *value /= cur_value;
                break;
            default: error_debug("item");
            }
        }
        if (symbol == MUL || symbol == DIV) {
            cur_op = symbol;
            it_type = INT;
            getsym_check();
        } else {
            break;
        }
    } while (true);
    return it_type;
}

VarItem* get_global_var(string name) {
    VAR_MAP::iterator it = global_vars.find(name);
    if (it != global_vars.end()) {
        return it->second;
    } else {
        return NULL;
    }
}

ConstItem* get_global_const(string name) {
    CONST_MAP::iterator it = global_consts.find(name);
    if (it != global_consts.end()) {
        return it->second;
    } else {
        return NULL;
    }
}

FuncItem* get_func(string name) {
    FUNC_MAP::iterator it = funcs.find(name);
    if (it != funcs.end()) {
        return it->second;
    } else {
        return NULL;
    }
}

Item* get_item(string name) {
    Item* item;
    if ((item = this_func->get_const(name)) != NULL) return item;
    if ((item = this_func->get_var(name)) != NULL) return item;
    if ((item = get_global_const(name)) != NULL) return item;
    if ((item = get_global_var(name)) != NULL) return item;
    if ((item = get_func(name)) != NULL) return item;
    return NULL;
}


Item* read_ident() {
    Item* item = NULL;
    name = token;
    if ((item = get_item(name)) == NULL) {
        error((string)"unexpected sign \'" + name + "\'");
        return NULL;
    }
    getsym_check();

    // is array
    if (symbol == LBKT) { // '['
        getsym_check();
        if (item->get_kind() != VAR || !((VarItem*)item)->isarray()) {
            error((string)"\'" + name + "\' is not an array type");
        }
        expr(&const_value, &certain);
        int len = ((VarItem*)item)->get_len();
        if (certain && (const_value < 0 || const_value >= len)) {
            error((string)"index of array \'" + name + "[]\' out of range");
        }
        /* out of range judgment */
        mate(RBKT); // ']'

    // is function with paras
    } else if (symbol == LPAR) {    // '('
        getsym_check();
        vector<Type> para_types;
        do {
            para_types.push_back(expr(&const_value, &certain));
            if (symbol != COMMA) { // ','
                break;
            }
            getsym_check();
        } while (true);
        if (item->get_kind() != FUNC || !((FuncItem*)item)->para_check(para_types)) {
            error((string)"parameters to function \'" + item->get_name() +"\' do not match");
        }
        mate(RPAR); // ')'
    }
    return item;
}

Type factor(int* value, bool* certain) {
    Type fac_type;
    Item* item = NULL;
    *certain = false;
    switch(symbol){
        case IDENT:
            item = read_ident();
            fac_type = item->get_type();
            if (item->get_type() == VOID) {
                error("void value not ignored as it ought to be");
            }
            if (item->get_kind() == CONST) {
                *value = ((ConstItem*)item)->get_value();
                *certain = true;
            }
            break;

        // '(' function with paras
        case LPAR:
            getsym_check();
            fac_type = expr(value, certain);
            mate(RPAR); // ')'
            break;

        case ADD:
            getsym_check();
            fac_type = INT;
            mate(INTCON);
            *value = num;
            *certain = true;
            break;

        case SUB:
            getsym_check();
            fac_type = INT;
            mate(INTCON);
            *value = -1 * num;
            *certain = true;
            break;

        case INTCON:
            fac_type = INT;
            *value = num;
            *certain = true;
            getsym_check();
            break;

        case CHARCON:
            fac_type = CHAR;
            *value = num;
            *certain = true;
            getsym_check();
            break;

        case ZERO:
            fac_type = INT;
            *value = 0;
            *certain = true;
            getsym_check();
            break;

        default:
            error((string)"unexpected symbol " + symbol2string(symbol) + " in [factor]");
    }
    return fac_type;
}

void cond() {
    expr(&const_value, &certain);
    switch (symbol){
    case GT:
        getsym_check();
        break;
    case GE:
        getsym_check();
        break;
    case LT:
        getsym_check();
        break;
    case LE:
        getsym_check();
        break;
    case EQ:
        getsym_check();
        break;
    case NE:
        getsym_check();
        break;
    default:
        return; // only expression
    }
    expr(&const_value, &certain);
}



void statement() {
    switch(symbol){
    // if (cond) statement else statement
    case IFSY:
        output_info("If statement begins!");
        // if block
        getsym_check();
        mate(LPAR); // '('
        cond(); // identify condition
        mate(RPAR); // ')'
        statement();    // statement among if

        output_info("Else statement begins!");
        // else block
        mate(ELSY); // 'else'
        statement();    // statement among else

        output_info("Else statement over!");
        break;

    // switch(expr){case 0:statement [default: statement]}
    case SWTSY:
        output_info("Switch statement begins!");
        getsym_check();
        mate(LPAR); // '('
        type = expr(&const_value, &certain); // expression to switch
        mate(RPAR); // ')'
        mate(LBRACE);   // '{'
        mate(CASESY);   // case
        // cases
        do {
            output_info("Case statement begins!");
            if (symbol == INTCON || symbol == ZERO) {    // int
                if (type == CHAR) {
                    error((string)"expected type \'int\', got \'char\'");
                }
                getsym_check();
            } else if (symbol == CHARCON) { // char
                if (type == INT) {
                    error((string)"expected type \'char\', got \'int\'");
                }
                getsym_check();
            } else {
                error((string)"unexpected symbol " + symbol2string(symbol) + " after [case]");
            }
            mate(COLON);    // :
            statement();
            if (symbol != CASESY) {
                break;
            }
            getsym_check();
        } while (true);
        if (symbol == DFTSY) {
            output_info("Default statement begins!");
            getsym_check();
            mate(COLON);    // :
            statement();
        }
        mate(RBRACE);   // ';'
        output_info("Switch statement over!");
        break;

    // while(cond)statement
    case WHILESY:
        output_info("While statement begins!");
        getsym_check();
        mate(LPAR); // '('
        cond(); // identify condition
        mate(RPAR); // ')'
        statement();    // statement among if
        output_info("While statement over!");
        break;

    // '{'
    case LBRACE:
        getsym_check();
        while (symbol != RBRACE) {
            statement();
        } // '}'
        getsym_check();
        break;

    // return[(expr)];
    case RTNSY:
        output_info("This is a return statement!");
        returned = true;
        getsym_check();
        if (symbol != SEMI) {
            mate(LPAR);
            Type ex_type = expr(&const_value, &certain);
            if (ex_type != this_func->get_type()) {
                error((string)"expected return type \'" + type2string(this_func->get_type()) +
                       "\', actual '" + type2string(ex_type) + "\'");
            }
            mate(RPAR);
            mate(SEMI);
        } else {
            getsym_check();
        }
        break;

    // read
    case PRTFST:
        output_info("This is a print statement!");
        getsym_check();
        mate(LPAR); // '('
        if (symbol == STRCON) { // string
            getsym_check();
            if (symbol == COMMA) {  // ','
                getsym_check();
                expr(&const_value, &certain);
            }
        } else {
            expr(&const_value, &certain);
        }
        mate(RPAR); // ')'
        mate(SEMI);
        break;

    // write
    case SCFSY:
        output_info("This is a read statement!");
        getsym_check();
        mate(LPAR);
        do {
            mate(IDENT, &record_name);
            Item* item = get_item(name);
            if (item == NULL) {
                error("unexpected identifier \'" + name + "\'");
            } else if (item->get_kind() != VAR || ((VarItem*)item)->isarray()) {
                error("can only write to variables");
            }
            if (symbol != COMMA) { // ','
                break;
            }
            getsym_check();
        } while (true);
        mate(RPAR);
        mate(SEMI);
        break;

    // assignment | function
    case IDENT: {
        Item* item = read_ident();
        if (item == NULL) {
            break;  // not found
        }
        if (SEMI == symbol) {
            output_info("This is a function invoking statement!");
            if (item->get_kind() != FUNC) {
                error((string)"meaning less " + kind2string(item->get_kind()) + " \'" + item->get_name() + "\'");
            }
        } else if (ASS == symbol) {
            output_info("This is a assign statement!");
            getsym_check();
            if (item->get_kind() != VAR) {
                error((string)"assignment of non-var \'" + item->get_name() + "\'");
            }
            if (expr(&const_value, &certain) == INT && item->get_type() == CHAR) {
                error((string)"cannot convert 'int' to 'char'");
            }
        }
        mate(SEMI); // ';'
        break;
    }

    // ;
    case SEMI:
        getsym_check();
        break;

    default:
        error((string)"unexpected symbol " + symbol2string(symbol) + " in the beginning of the statement");
        break;
    }
}

bool defined(string name) {
    CONST_MAP::iterator itc = global_consts.find(name);
    if (itc != global_consts.end()) {
        error("redefinition of '" + name + "\'");
        return true;
    }
    VAR_MAP::iterator itv = global_vars.find(name);
    if (itv != global_vars.end()) {
        error("redefinition of '" + name + "\'");
        return true;
    }
    return false;
}

void put_global_const(string name, Type type, int value) {
    if (!defined(name))
    global_consts.insert(CONST_MAP::value_type(name, new ConstItem(name, type, value)));
}

void put_global_var(string name, Type type, int len = 0) {
    if (!defined(name))
    global_vars.insert(VAR_MAP::value_type(name, new VarItem(name, type, len)));
}

void declare_const(FuncItem* func = NULL) {
    while (symbol == CONSTSY) {
        getsym_check();
        switch (symbol) {
        case INTSY:
            getsym_check();
            do {
                mate(IDENT, &record_name);  // record name
                mate(ASS);
                if (symbol == ZERO) {
                    getsym_check();
                    if (func == NULL) {
                        put_global_const(name, INT, 0);
                    } else {
                        if (func->get_name() == name) {
                            error((string)"conflicting declaration with function name");
                        }
                        func->put_const(name, INT, 0);
                    }

                } else {
                    int sign = 1;
                    if (symbol == ADD) {
                        sign = 1;
                        getsym_check();
                    } else if (symbol == SUB) {
                        sign = -1;
                        getsym_check();
                    }
                    mate(INTCON);
                    if (func == NULL) {
                        put_global_const(name, INT, sign * num);
                    } else {
                        if (func->get_name() == name) {
                            error((string)"conflicting declaration with function name");
                        }
                        func->put_const(name, INT, sign * num);
                    }

                }
                if (symbol != COMMA) {
                    break;
                }
                getsym_check();
            } while (true);
            break;
        case CHARSY:
            getsym_check();
            do {
                mate(IDENT, &record_name);
                mate(ASS);
                mate(CHARCON);
                if (func == NULL) {
                    put_global_const(name, CHAR, num);
                } else {
                    if (func->get_name() == name) {
                        error((string)"conflicting declaration with function name");
                    }
                    func->put_const(name, CHAR, num);
                }

                if (symbol != COMMA) {
                    break;
                }
                getsym_check();
            } while (true);
            break;
        default:
            error((string)"unexpected const type " + symbol2string(symbol));
        }
        mate(SEMI);
    }
}



void declare_var(FuncItem* func = NULL) {
    bool is_first = true;
    while (true) {
        is_first = true;
        switch (symbol) {
        case INTSY:
            type = INT;
            break;
        case CHARSY:
            type = CHAR;
            break;
        default:
            return;
        }
        getsym_check();

        do {
            mate(IDENT, &record_name);
            // array '['
            if (is_first) {
                if (LPAR == symbol || LBRACE == symbol) {
                    if (func == NULL) {
                        skip_type_ident = true;
                        return;
                    } else {
                        break;
                    }
                }
                is_first = false;
            }
            if (LBKT == symbol) {
                getsym_check();
                mate(INTCON);
                if (func == NULL) {
                    put_global_var(name, type, num);
                } else {
                    if (func->get_name() == name) {
                        error((string)"conflicting declaration with function name");
                    }
                    func->put_var(name, type, num);
                }

                mate(RBKT); // ']'

            } else {
                if (func == NULL) {
                    put_global_var(name, type);
                } else {
                    if (func->get_name() == name) {
                        error((string)"conflicting declaration with function name");
                    }
                    func->put_var(name, type);
                }
            }

            if (COMMA != symbol) {  // ','
                break;
            }
            getsym_check();
        } while (true);
        mate(SEMI);
    }
}

void comp_statement(FuncItem* func) {
    mate(LBRACE);   // '{'
    declare_const(func);
    declare_var(func);
    returned = false;
    while (symbol != RBRACE) {  // '}'
        statement();
    }
    if (!returned && func->get_type() != VOID) {
        error((string)"non-void return type function without a return statement");
    }
    if (func->get_name() == "main") {
        if (!next_end()) {
            error("there\'re something behind \'main\'");
        }

    } else {
        if (!getsym()) {
            error("undefined reference \'main\'");
        }
    }

}

void declare_func() {
    this_func = NULL;
    while (true) {
        if (skip_type_ident) {
            skip_type_ident = false;
        } else {
            switch (symbol) {
            case VOIDSY:
                getsym_check();
                if (MAINSY == symbol) {
                    getsym_check();
                    mate(LPAR);
                    mate(RPAR); // '()'
                    this_func = new FuncItem("main", VOID);
                    funcs.insert(FUNC_MAP::value_type("main", this_func));
                    comp_statement(this_func);

                    return; // finish
                } else {
                    type = VOID;
                }
                break;
            case INTSY:
                getsym_check();
                type = INT;
                break;
            case CHARSY:
                getsym_check();
                type = CHAR;
                break;
            default:
                error((string)"unexpected return type " + symbol2string(symbol));
                return;
            }
            mate(IDENT, &record_name);
        }
        this_func = new FuncItem(name, type);
        funcs.insert(FUNC_MAP::value_type(name, this_func));

        if (symbol == LPAR) {
            getsym_check();
            do {
                switch (symbol) {
                case INTSY:
                    type = INT;
                    getsym_check();
                    break;
                case CHARSY:
                    type = CHAR;
                    getsym_check();
                    break;
                default:
                    error((string)"Unexpected parameter type " + symbol2string(symbol));
                }
                mate(IDENT, &record_name);
                this_func->put_para(name, type);
                if (symbol != COMMA) { // ','
                    break;
                }
                getsym_check();
            } while (true);
            mate(RPAR);
        }
        comp_statement(this_func);
    }
}

int gram_main() {
    getsym_check();
     cout << "=== CONST BEGIN ===" << endl;
    declare_const();
     cout << "=== VAR BEGIN ===" << endl;
    declare_var();
     cout << "=== FUNC BEGIN ===" << endl;
    declare_func();
     cout << "=== SUCCESS! ===" << endl;
     return 0;
}
