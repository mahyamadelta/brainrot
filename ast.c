/* ast.c */

#include "ast.h"
#include <stdbool.h>
#include <setjmp.h>
#include <string.h>
#include <math.h>
#include <limits.h>

static jmp_buf break_env;

TypeModifiers current_modifiers = {false, false, false, false};
VarType current_var_type = NONE;

Variable symbol_table[MAX_VARS];
int var_count = 0;

// Symbol table functions
bool set_variable(char *name, void *value, VarType type, TypeModifiers mods) {
    // Search for an existing variable
    for (int i = 0; i < var_count; i++) {
        if (strcmp(symbol_table[i].name, name) == 0) {
            symbol_table[i].var_type = type;
            symbol_table[i].modifiers = mods;

            switch (type) {
                case VAR_INT:
                    symbol_table[i].value.ivalue = *(int *)value;
                    break;
                case VAR_FLOAT:
                    symbol_table[i].value.fvalue = *(float *)value;
                    break;
                case VAR_DOUBLE:
                    symbol_table[i].value.dvalue = *(double *)value;
                    break;
                case VAR_BOOL:
                    symbol_table[i].value.bvalue = *(bool *)value;
                    break;
                case VAR_CHAR:
                    symbol_table[i].value.ivalue = *(char *)value;
                    break;
                case VAR_SHORT: //TODO add short type
                    symbol_table[i].value.ivalue = *(short *)value;
                    break;
            }
            return true;
        }
    }

    // Add a new variable if it doesn't exist
    if (var_count < MAX_VARS) {
        symbol_table[var_count].name = strdup(name);
        symbol_table[var_count].var_type = type;
        symbol_table[var_count].modifiers = mods;

        switch (type) {
            case VAR_INT:
                symbol_table[var_count].value.ivalue = *(int *)value;
                break;
            case VAR_FLOAT:
                symbol_table[var_count].value.fvalue = *(float *)value;
                break;
            case VAR_DOUBLE:
                symbol_table[var_count].value.dvalue = *(double *)value;
                break;
            case VAR_BOOL:
                symbol_table[var_count].value.bvalue = *(bool *)value;
                break;
            case VAR_CHAR:
                symbol_table[var_count].value.ivalue = *(char *)value;
                break;
            case VAR_SHORT:
                symbol_table[var_count].value.ivalue = *(short *)value;
                break;
            default:   
                break;

        }
        var_count++;
        return true;
    }
    return false; // Symbol table is full
}

bool set_int_variable(char *name, int value, TypeModifiers mods) {
    return set_variable(name, &value, VAR_INT, mods);
}

bool set_float_variable(char *name, float value, TypeModifiers mods) {
    return set_variable(name, &value, VAR_FLOAT, mods);
}

bool set_double_variable(char *name, double value, TypeModifiers mods) {
    return set_variable(name, &value, VAR_DOUBLE, mods);
}

bool set_bool_variable(char *name, bool value, TypeModifiers mods) {
    return set_variable(name, &value, VAR_BOOL, mods);
}

void reset_modifiers(void)
{
    current_modifiers.is_volatile = false;
    current_modifiers.is_signed = false;
    current_modifiers.is_unsigned = false;
}

TypeModifiers get_current_modifiers(void)
{
    TypeModifiers mods = current_modifiers;
    reset_modifiers(); // Reset for next declaration
    return mods;
}

/* Include the symbol table functions */
extern int get_variable(char *name);
extern void yyerror(const char *s);
extern void ragequit(int exit_code);
extern void chill(unsigned int seconds);
extern void yapping(const char *format, ...);
extern void yappin(const char *format, ...);
extern void baka(const char *format, ...);
extern TypeModifiers get_variable_modifiers(const char *name);
extern int yylineno;

/* Function implementations */

bool check_and_mark_identifier(ASTNode *node, const char *contextErrorMessage)
{
    if (!node->alreadyChecked)
    {
        node->alreadyChecked = true;
        node->isValidSymbol = false;

        // Do the table lookup
        for (int i = 0; i < var_count; i++)
        {
            if (strcmp(symbol_table[i].name, node->data.name) == 0)
            {
                node->isValidSymbol = true;
                break;
            }
        }

        if (!node->isValidSymbol)
        {
            yylineno = yylineno - 2;
            yyerror(contextErrorMessage);
        }
    }

    return node->isValidSymbol;
}

void execute_switch_statement(ASTNode *node)
{
    int switch_value = evaluate_expression(node->data.switch_stmt.expression);
    CaseNode *current_case = node->data.switch_stmt.cases;
    int matched = 0;

    if (setjmp(break_env) == 0)
    {
        while (current_case)
        {
            if (current_case->value)
            {
                int case_value = evaluate_expression(current_case->value);
                if (case_value == switch_value || matched)
                {
                    matched = 1;
                    execute_statements(current_case->statements);
                }
            }
            else
            {
                // Default case
                if (matched || !matched)
                {
                    execute_statements(current_case->statements);
                    break;
                }
            }
            current_case = current_case->next;
        }
    }
    else
    {
        // Break encountered; do nothing
    }
}

static ASTNode *create_node(NodeType type, VarType var_type, TypeModifiers modifiers)
{
    ASTNode *node = malloc(sizeof(ASTNode));
    if (!node)
    {
        yyerror("Error: Memory allocation failed for ASTNode.\n");
        exit(EXIT_FAILURE);
    }
    node->type = type;
    node->var_type = var_type;
    node->modifiers = modifiers;
    node->alreadyChecked = false;
    node->isValidSymbol = false;
    return node;
}


ASTNode *create_int_node(int value)
{
    ASTNode *node = create_node(NODE_INT, VAR_INT, current_modifiers);
    SET_DATA_INT(node, value);
    return node;
}

ASTNode *create_float_node(float value)
{
    ASTNode *node = create_node(NODE_FLOAT, VAR_FLOAT, current_modifiers);
    SET_DATA_FLOAT(node, value);
    return node;
}

ASTNode *create_char_node(char value)
{
    ASTNode *node = create_node(NODE_CHAR, VAR_CHAR, current_modifiers);
    SET_DATA_INT(node, value); // Store char as integer
    return node;
}

ASTNode *create_boolean_node(bool value)
{
    ASTNode *node = create_node(NODE_BOOLEAN, VAR_BOOL, current_modifiers);
    SET_DATA_BOOL(node, value);
    return node;
}

ASTNode *create_identifier_node(char *name)
{
    ASTNode *node = create_node(NODE_IDENTIFIER, NONE, current_modifiers);
    SET_DATA_NAME(node, name);
    return node;
}

ASTNode *create_assignment_node(char *name, ASTNode *expr)
{
    ASTNode *node = create_node(NODE_ASSIGNMENT, NONE, get_current_modifiers());
    SET_DATA_OP(node, create_identifier_node(name), expr, OP_ASSIGN);
    return node;
}

ASTNode *create_operation_node(OperatorType op, ASTNode *left, ASTNode *right)
{
    ASTNode *node = create_node(NODE_OPERATION, NONE, current_modifiers);
    SET_DATA_OP(node, left, right, op);
    return node;
}

ASTNode *create_unary_operation_node(OperatorType op, ASTNode *operand)
{
    ASTNode *node = create_node(NODE_UNARY_OPERATION, NONE, current_modifiers);
    SET_DATA_UNARY_OP(node, operand, op);
    return node;
}

ASTNode *create_for_statement_node(ASTNode *init, ASTNode *cond, ASTNode *incr, ASTNode *body)
{
    ASTNode *node = create_node(NODE_FOR_STATEMENT, NONE, current_modifiers);
    SET_DATA_FOR(node, init, cond, incr, body);
    return node;
}

ASTNode *create_while_statement_node(ASTNode *cond, ASTNode *body)
{
    ASTNode *node = create_node(NODE_WHILE_STATEMENT, NONE, current_modifiers);
    SET_DATA_WHILE(node, cond, body);
    return node;
}

ASTNode *create_function_call_node(char *func_name, ArgumentList *args)
{
    ASTNode *node = create_node(NODE_FUNC_CALL, NONE, current_modifiers);
    SET_DATA_FUNC_CALL(node, func_name, args);
    return node;
}

ASTNode *create_double_node(double value)
{
    ASTNode *node = create_node(NODE_DOUBLE, VAR_DOUBLE, current_modifiers);
    SET_DATA_DOUBLE(node, value);
    return node;
}

ASTNode *create_sizeof_node(char *name)
{
    ASTNode *node = create_node(NODE_SIZEOF, NONE, current_modifiers);
    SET_DATA_NAME(node, name);
    return node;
}

float evaluate_expression_float(ASTNode *node)
{
    if (!node)
        return 0.0f;

    switch (node->type)
    {
    case NODE_FLOAT:
        return node->data.fvalue;
    case NODE_DOUBLE:
        return (float)node->data.dvalue;
    case NODE_INT:
        return (float)node->data.ivalue;
    case NODE_IDENTIFIER:
    {
        char *name = node->data.name;
        for (int i = 0; i < var_count; i++)
        {
            if (strcmp(symbol_table[i].name, name) == 0)
            {
                if (symbol_table[i].var_type == VAR_DOUBLE)
                {
                    return (float)symbol_table[i].value.dvalue;
                }
                else if (symbol_table[i].var_type == VAR_FLOAT)
                {
                    return symbol_table[i].value.fvalue;
                }
                else
                {
                    return (float)symbol_table[i].value.ivalue;
                }
            }
        }
        yyerror("Undefined variable");
        return 0.0f;
    }
    case NODE_OPERATION:
    {
        float left = evaluate_expression_float(node->data.op.left);
        float right = evaluate_expression_float(node->data.op.right);

        switch (node->data.op.op)
        {
        case OP_PLUS:
            return left + right;
        case OP_MINUS:
            return left - right;
        case OP_TIMES:
            return left * right;
        case OP_DIVIDE:
            if (fabsf(right) < __FLT_MIN__)
            {
                if (fabsf(left) < __FLT_MIN__)
                {
                    return 0.0f / 0.0f; // NaN
                }
                return left > 0 ? __FLT_MAX__ : -__FLT_MAX__;
            }
            return left / right;
        case OP_LT:
        {
            return (left - right) < -__FLT_EPSILON__ ? 1.0f : 0.0f;
        }
        case OP_GT:
        {
            return (left - right) > __FLT_EPSILON__ ? 1.0f : 0.0f;
        }
        case OP_LE:
        {
            return (left - right) <= __FLT_EPSILON__ ? 1.0f : 0.0f;
        }
        case OP_GE:
        {
            return (left - right) >= -__FLT_EPSILON__ ? 1.0f : 0.0f;
        }
        case OP_EQ:
        {
            return fabsf(left - right) <= __FLT_EPSILON__ ? 1.0f : 0.0f;
        }
        case OP_NE:
        {
            return fabsf(left - right) > __FLT_EPSILON__ ? 1.0f : 0.0f;
        }
        default:
            yyerror("Invalid operator for float operation");
            return 0.0f;
        }
    }
    case NODE_UNARY_OPERATION:
    {
        float operand = evaluate_expression_float(node->data.unary.operand);
        switch (node->data.unary.op)
        {
        case OP_NEG:
            return -operand;
        case OP_POST_DEC:{
            float old_value =operand;
            set_variable(node->data.unary.operand->data.name,operand - 1, get_variable_modifiers(node->data.unary.operand->data.name));
            return old_value;
                         }
        case OP_POST_INC:{
            float old_value =operand;
            set_variable(node->data.unary.operand->data.name,operand + 1, get_variable_modifiers(node->data.unary.operand->data.name));
            return old_value;

                         }
        case OP_PRE_DEC:
            set_variable(node->data.unary.operand->data.name,operand - 1, get_variable_modifiers(node->data.unary.operand->data.name));
            return operand - 1;
        case OP_PRE_INC:
            set_variable(node->data.unary.operand->data.name,operand + 1, get_variable_modifiers(node->data.unary.operand->data.name));
            return operand + 1;
        default:
            yyerror("Unknown unary operator for float");
            return 0.0f;
        }
    }
    default:
        yyerror("Invalid float expression");
        return 0.0f;
    }
}

double evaluate_expression_double(ASTNode *node)
{
    if (!node)
        return 0.0L;

    switch (node->type)
    {
    case NODE_DOUBLE:
        return node->data.dvalue;
    case NODE_FLOAT:
        return (double)node->data.fvalue;
    case NODE_INT:
        return (double)node->data.ivalue;
    case NODE_IDENTIFIER:
    {
        char *name = node->data.name;
        for (int i = 0; i < var_count; i++)
        {
            if (strcmp(symbol_table[i].name, name) == 0)
            {
                if (symbol_table[i].var_type == VAR_DOUBLE)
                {
                    return symbol_table[i].value.dvalue;
                }
                else if (symbol_table[i].var_type == VAR_FLOAT)
                {
                    return (double)symbol_table[i].value.fvalue;
                }
                else
                {
                    return (double)symbol_table[i].value.ivalue;
                }
            }
        }
        yyerror("Undefined variable");
        return 0.0L;
    }
    case NODE_OPERATION:
    {
        double left = evaluate_expression_double(node->data.op.left);
        double right = evaluate_expression_double(node->data.op.right);

        switch (node->data.op.op)
        {
        case OP_PLUS:
            return left + right;
        case OP_MINUS:
            return left - right;
        case OP_TIMES:
            return left * right;
        case OP_DIVIDE:
            if (fabs(right) < __DBL_MIN__)
            {
                if (fabs(left) < __DBL_MIN__)
                {
                    return 0.0 / 0.0; // NaN
                }
                return left > 0 ? __DBL_MAX__ : -__DBL_MAX__;
            }
            return left / right;
        case OP_LT:
        {
            return (left - right) < -__FLT_EPSILON__ ? 1.0L : 0.0L;
        }
        case OP_GT:
        {
            return (left - right) > __DBL_EPSILON__ ? 1.0L : 0.0L;
        }
        case OP_LE:
        {
            return (left - right) <= __DBL_EPSILON__ ? 1.0L : 0.0L;
        }
        case OP_GE:
        {
            return (left - right) >= -__DBL_EPSILON__ ? 1.0L : 0.0L;
        }
        case OP_EQ:
        {
            return fabs(left - right) <= __DBL_EPSILON__ ? 1.0L : 0.0L;
        }
        case OP_NE:
        {
            return fabs(left - right) > __DBL_EPSILON__ ? 1.0L : 0.0L;
        }
        default:
            yyerror("Invalid operator for double operation");
            return 0.0L;
        }
    }
    case NODE_UNARY_OPERATION:
    {
        float operand = evaluate_expression_double(node->data.unary.operand);
        switch (node->data.unary.op)
        {
        case OP_NEG:
            return -operand;
        case OP_POST_DEC:{
            double old_value =operand;
            set_variable(node->data.unary.operand->data.name,operand - 1, get_variable_modifiers(node->data.unary.operand->data.name));
            return old_value;
                         }
        case OP_POST_INC:{
            double old_value =operand;
            set_variable(node->data.unary.operand->data.name,operand + 1, get_variable_modifiers(node->data.unary.operand->data.name));
            return old_value;

                         }
        case OP_PRE_DEC:
            set_variable(node->data.unary.operand->data.name,operand - 1, get_variable_modifiers(node->data.unary.operand->data.name));
            return operand - 1;
        case OP_PRE_INC:
            set_variable(node->data.unary.operand->data.name,operand + 1, get_variable_modifiers(node->data.unary.operand->data.name));
            return operand + 1;
        default:
            yyerror("Unknown unary operator for double");
            return 0.0L;
        }
    }
    default:
        yyerror("Invalid double expression");
        return 0.0L;
    }
}

int evaluate_expression_int(ASTNode *node)
{
    if (!node)
        return 0;

    switch (node->type)
    {
    case NODE_INT:
        return node->data.ivalue;
    case NODE_BOOLEAN:
        return node->data.bvalue; // Already 1 or 0
    case NODE_CHAR:               // Add explicit handling for characters
        return node->data.ivalue;
    case NODE_FLOAT:
        yyerror("Cannot use float in integer context");
        return (int)node->data.fvalue;
    case NODE_DOUBLE:
        yyerror("Cannot use double in integer context");
        return (int)node->data.dvalue;
    case NODE_SIZEOF:
    {
        char *name = node->data.name;
        for (int i = 0; i < var_count; i++)
        {
            if (strcmp(symbol_table[i].name, name) == 0)
            {
                if (symbol_table[i].var_type == VAR_FLOAT)
                {
                    return sizeof(float);
                }
                else if (symbol_table[i].var_type == VAR_DOUBLE)
                {
                    return sizeof(double);
                }
                else if (symbol_table[i].modifiers.is_unsigned && symbol_table[i].var_type == VAR_INT)
                {
                    return sizeof(unsigned int);
                }
                else if (symbol_table[i].var_type == VAR_BOOL)
                {
                    return sizeof(bool);
                }
                else if (symbol_table[i].var_type == VAR_INT)
                {
                    return sizeof(int);
                }
                else
                {
                    yyerror("Undefined variable in sizeof");
                }
            }
        }
        yyerror("Undefined variable in sizeof");
        return 0;
    }
    case NODE_IDENTIFIER:
    {
        if (!check_and_mark_identifier(node, "Undefined variable"))
            exit(1);

        char *name = node->data.name;
        for (int i = 0; i < var_count; i++)
        {
            if (strcmp(symbol_table[i].name, name) == 0)
            {
                if (symbol_table[i].var_type == VAR_FLOAT)
                {
                    yyerror("Cannot use float variable in integer context");
                    return (int)symbol_table[i].value.fvalue;
                }
                if (symbol_table[i].var_type == VAR_DOUBLE)
                {
                    yyerror("Cannot use double variable in integer context");
                    return (int)symbol_table[i].value.dvalue;
                }
                return symbol_table[i].value.ivalue;
            }
        }
        yyerror("Undefined variable");
        return 0;
    }
    case NODE_OPERATION:
    {
        // Special handling for logical operations
        if (node->data.op.op == OP_AND || node->data.op.op == OP_OR)
        {
            int left = evaluate_expression_int(node->data.op.left);
            int right = evaluate_expression_int(node->data.op.right);

            switch (node->data.op.op)
            {
            case OP_AND:
                return left && right;
            case OP_OR:
                return left || right;
            default:
                break;
            }
        }

        // Regular integer operations
        int left = evaluate_expression_int(node->data.op.left);
        int right = evaluate_expression_int(node->data.op.right);


        switch (node->data.op.op)
        {
        case OP_PLUS:
            return left + right;
        case OP_MINUS:
            return left - right;
        case OP_TIMES:
            return left * right;
        case OP_DIVIDE:
            if (right == 0)
            {
                yyerror("Division by zero");
                return 0;
            }
            return left / right;
        case OP_MOD:
            if (right == 0)
            {
                yyerror("Division by zero");
                return 0;
            }
            // Explicitly handle unsigned modulo
            if (node->modifiers.is_unsigned)
            {
                unsigned int ul = (unsigned int)left;
                unsigned int ur = (unsigned int)right;
                return ul % ur;
            }
            return left % right;
        case OP_LT:
            return left < right;
        case OP_GT:
            return left > right;
        case OP_LE:
            return left <= right;
        case OP_GE:
            return left >= right;
        case OP_EQ:
            return left == right;
        case OP_NE:
            return left != right;
        default:
            yyerror("Unknown operator");
            return 0;
        }
    }
    case NODE_UNARY_OPERATION:
    {
        int operand = evaluate_expression_int(node->data.unary.operand);
        switch (node->data.unary.op)
        {
        case OP_NEG:
            return -operand;
        case OP_POST_DEC:{
            int old_value =operand;
            set_variable(node->data.unary.operand->data.name,operand - 1, get_variable_modifiers(node->data.unary.operand->data.name));
            return old_value;
                         }
        case OP_POST_INC:{
            int old_value =operand;
            set_variable(node->data.unary.operand->data.name,operand + 1, get_variable_modifiers(node->data.unary.operand->data.name));
            return old_value;

                         }
        case OP_PRE_DEC:
            set_variable(node->data.unary.operand->data.name,operand - 1, get_variable_modifiers(node->data.unary.operand->data.name));
            return operand - 1;
        case OP_PRE_INC:
            set_variable(node->data.unary.operand->data.name,operand + 1, get_variable_modifiers(node->data.unary.operand->data.name));
            return operand + 1;
        default:
            yyerror("Unknown unary operator");
            return 0;
        }
    }
    default:
        yyerror("Invalid integer expression");
        return 0;
    }
}

bool evaluate_expression_bool(ASTNode *node)
{
    if (!node)
        return 0;

    switch (node->type)
    {
    case NODE_INT:
        return (bool)node->data.ivalue;
    case NODE_BOOLEAN:
        return node->data.bvalue;
    case NODE_CHAR:
        return (bool)node->data.ivalue;
    case NODE_FLOAT:
        return (bool)node->data.fvalue;
    case NODE_DOUBLE:
        return (bool)node->data.dvalue;
    case NODE_IDENTIFIER:
    {
        if (!check_and_mark_identifier(node, "Undefined variable"))
            exit(1);

        char *name = node->data.name;
        for (int i = 0; i < var_count; i++)
        {
            if (strcmp(symbol_table[i].name, name) == 0)
            {
                if (symbol_table[i].var_type == VAR_INT)
                {
                    return (bool)symbol_table[i].value.ivalue;
                }
                if (symbol_table[i].var_type == VAR_FLOAT)
                {
                    return (bool)symbol_table[i].value.fvalue;
                }
                if (symbol_table[i].var_type == VAR_DOUBLE)
                {
                    return (bool)symbol_table[i].value.dvalue;
                }
                return symbol_table[i].value.bvalue;
            }
        }
        yyerror("Undefined variable");
        return 0;
    }
    case NODE_OPERATION:
    {
        // Special handling for logical operations
        if (node->data.op.op == OP_AND || node->data.op.op == OP_OR)
        {
            bool left = evaluate_expression_bool(node->data.op.left);
            bool right = evaluate_expression_bool(node->data.op.right);

            switch (node->data.op.op)
            {
            case OP_AND:
                return left && right;
            case OP_OR:
                return left || right;
            default:
                break;
            }
        }

        // Regular integer operations
        bool left = evaluate_expression_bool(node->data.op.left);
        bool right = evaluate_expression_bool(node->data.op.right);

        switch (node->data.op.op)
        {
        case OP_PLUS:
            return left + right;
        case OP_MINUS:
            return left - right;
        case OP_TIMES:
            return left * right;
        case OP_DIVIDE:
            if (right == 0)
            {
                yyerror("Division by zero");
                return 0;
            }
            return left / right;
        case OP_MOD:
            if (right == 0)
            {
                yyerror("Division by zero");
                return 0;
            }
            return left % right;
        case OP_LT:
            return left < right;
        case OP_GT:
            return left > right;
        case OP_LE:
            return left <= right;
        case OP_GE:
            return left >= right;
        case OP_EQ:
            return left == right;
        case OP_NE:
            return left != right;
        default:
            yyerror("Unknown operator");
            return 0;
        }
    }
    case NODE_UNARY_OPERATION:
    {
        bool operand = evaluate_expression_bool(node->data.unary.operand);
        switch (node->data.unary.op)
        {
        case OP_NEG:
            return -operand;
        default:
            yyerror("Unknown unary operator");
            return 0;
        }
    }
    default:
        yyerror("Invalid boolean expression");
        return 0;
    }
}

ArgumentList *create_argument_list(ASTNode *expr, ArgumentList *existing_list)
{
    ArgumentList *new_node = malloc(sizeof(ArgumentList));
    new_node->expr = expr;
    new_node->next = NULL;

    if (!existing_list)
    {
        return new_node;
    }
    else
    {
        /* Append to the end of existing_list */
        ArgumentList *temp = existing_list;
        while (temp->next)
        {
            temp = temp->next;
        }
        temp->next = new_node;
        return existing_list;
    }
}

ASTNode *create_print_statement_node(ASTNode *expr)
{
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = NODE_PRINT_STATEMENT;
    node->data.op.left = expr;
    return node;
}

ASTNode *create_error_statement_node(ASTNode *expr)
{
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = NODE_ERROR_STATEMENT;
    node->data.op.left = expr;
    return node;
}

ASTNode *create_statement_list(ASTNode *statement, ASTNode *existing_list)
{
    if (!existing_list)
    {
        // If there's no existing list, create a new one
        ASTNode *node = malloc(sizeof(ASTNode));
        node->type = NODE_STATEMENT_LIST;
        node->data.statements = malloc(sizeof(StatementList));
        node->data.statements->statement = statement;
        node->data.statements->next = NULL;
        return node;
    }
    else
    {
        // Append at the end of existing_list
        StatementList *sl = existing_list->data.statements;
        while (sl->next)
        {
            sl = sl->next;
        }
        // Now sl is the last element; append the new statement
        StatementList *new_item = malloc(sizeof(StatementList));
        new_item->statement = statement;
        new_item->next = NULL;
        sl->next = new_item;
        return existing_list;
    }
}

bool is_float_expression(ASTNode *node)
{
    if (!node)
        return false;

    switch (node->type)
    {
    case NODE_FLOAT:
        return true;
    case NODE_INT:
        return false;
    case NODE_DOUBLE:
        return false;
    case NODE_IDENTIFIER:
    {
        if (!check_and_mark_identifier(node, "Undefined variable in type check"))
            exit(1);
        for (int i = 0; i < var_count; i++)
        {
            if (strcmp(symbol_table[i].name, node->data.name) == 0)
            {
                return symbol_table[i].var_type == VAR_FLOAT;
            }
        }
        yyerror("Undefined variable in type check");
        return false;
    }
    case NODE_OPERATION:
    {
        // If either operand is float, result is float
        return is_float_expression(node->data.op.left) ||
               is_float_expression(node->data.op.right);
    }
    default:
        return false;
    }
}

bool is_double_expression(ASTNode *node)
{
    if (!node)
        return false;

    switch (node->type)
    {
    case NODE_DOUBLE:
        return true;
    case NODE_FLOAT:
        return false;
    case NODE_INT:
        return false;
    case NODE_IDENTIFIER:
    {
        if (!check_and_mark_identifier(node, "Undefined variable in type check"))
            exit(1);
        for (int i = 0; i < var_count; i++)
        {
            if (strcmp(symbol_table[i].name, node->data.name) == 0)
            {
                return symbol_table[i].var_type == VAR_DOUBLE;
            }
        }
        yyerror("Undefined variable in type check");
        return false;
    }
    case NODE_OPERATION:
    {
        // If either operand is double, result is double
        return is_double_expression(node->data.op.left) ||
               is_double_expression(node->data.op.right);
    }
    default:
        return false;
    }
}

int evaluate_expression(ASTNode *node)
{
    if (is_float_expression(node))
    {
        return (int)evaluate_expression_float(node);
    }
    if (is_double_expression(node))
    {
        return (int)evaluate_expression_double(node);
    }
    return evaluate_expression_int(node);
}

void execute_assignment(ASTNode *node)
{
    if (node->type != NODE_ASSIGNMENT)
    {
        yyerror("Expected assignment node");
        return;
    }

    char *name = node->data.op.left->data.name;
    ASTNode *value_node = node->data.op.right;
    TypeModifiers mods = node->modifiers;

    // Handle type conversion for float to int
    if (value_node->type == NODE_FLOAT || is_float_expression(value_node))
    {
        float value = evaluate_expression_float(value_node);
        if (node->data.op.left->type == NODE_INT)
        {
            // Check for overflow
            if (value > INT_MAX || value < INT_MIN)
            {
                yyerror("Float to int conversion overflow");
                value = INT_MAX;
            }
            if (!set_int_variable(name, (int)value, mods))
            {
                yyerror("Failed to set integer variable");
            }
            return;
        }
    }

    if (is_float_expression(value_node))
    {
        float value = evaluate_expression_float(value_node);
        if (!set_float_variable(name, value, mods))
        {
            yyerror("Failed to set float variable");
        }
    }
    else if (is_double_expression(value_node))
    {
        double value = evaluate_expression_double(value_node);
        if (!set_double_variable(name, value, mods))
        {
            yyerror("Failed to set double variable");
        }
    }
    else
    {
        int value = evaluate_expression_int(value_node);
        if (!set_int_variable(name, value, mods))
        {
            yyerror("Failed to set integer variable");
        }
    }
}

void execute_statement(ASTNode *node)
{
    if (!node)
        return;
    switch (node->type)
    {
    case NODE_ASSIGNMENT:
    {
        char *name = node->data.op.left->data.name;
        ASTNode *value_node = node->data.op.right;
        TypeModifiers mods = node->modifiers;

        if (value_node->type == NODE_CHAR)
        {
            // Handle character assignments directly
            if (!set_int_variable(name, value_node->data.ivalue, mods))
            {
                yyerror("Failed to set character variable");
            }
        }
        else if (value_node->type == NODE_BOOLEAN)
        {
            if (!set_bool_variable(name, value_node->data.bvalue, mods))
            {
                yyerror("Failed to set boolean variable");
            }
        }
        else if (is_float_expression(value_node))
        {
            float value = evaluate_expression_float(value_node);
            if (!set_float_variable(name, value, mods))
            {
                yyerror("Failed to set float variable");
            }
        }
        else if (is_double_expression(value_node))
        {
            double value = evaluate_expression_double(value_node);
            if (!set_double_variable(name, value, mods))
            {
                yyerror("Failed to set double variable");
            }
        }
        else
        {
            int value = evaluate_expression_int(value_node);
            if (!set_int_variable(name, value, mods))
            {
                yyerror("Failed to set integer variable");
            }
        }
        break;
    }
    case NODE_OPERATION:
    case NODE_UNARY_OPERATION:
    case NODE_INT:
    case NODE_CHAR:
    case NODE_IDENTIFIER:
        evaluate_expression(node);
        break;
    case NODE_FUNC_CALL:
        if (strcmp(node->data.func_call.function_name, "yapping") == 0)
        {
            execute_yapping_call(node->data.func_call.arguments);
        }
        else if (strcmp(node->data.func_call.function_name, "yappin") == 0)
        {
            execute_yappin_call(node->data.func_call.arguments);
        }
        else if (strcmp(node->data.func_call.function_name, "baka") == 0)
        {
            execute_baka_call(node->data.func_call.arguments);
        }
        else if (strcmp(node->data.func_call.function_name, "ragequit") == 0)
        {
            execute_ragequit_call(node->data.func_call.arguments);
        }
        else if (strcmp(node->data.func_call.function_name, "chill") == 0)
        {
            execute_chill_call(node->data.func_call.arguments);
        }
        break;
    case NODE_FOR_STATEMENT:
        execute_for_statement(node);
        break;
    case NODE_WHILE_STATEMENT:
        execute_while_statement(node);
        break;
    case NODE_PRINT_STATEMENT:
    {
        ASTNode *expr = node->data.op.left;
        if (expr->type == NODE_STRING_LITERAL)
        {
            yapping("%s\n", expr->data.name);
        }
        else
        {
            int value = evaluate_expression(expr);
            yapping("%d\n", value);
        }
        break;
    }
    case NODE_ERROR_STATEMENT:
    {
        ASTNode *expr = node->data.op.left;
        if (expr->type == NODE_STRING_LITERAL)
        {
            baka("%s\n", expr->data.name);
        }
        else
        {
            int value = evaluate_expression(expr);
            baka("%d\n", value);
        }
        break;
    }
    case NODE_STATEMENT_LIST:
        execute_statements(node);
        break;
    case NODE_IF_STATEMENT:
        if (evaluate_expression(node->data.if_stmt.condition))
        {
            execute_statement(node->data.if_stmt.then_branch);
        }
        else if (node->data.if_stmt.else_branch)
        {
            execute_statement(node->data.if_stmt.else_branch);
        }
        break;
    case NODE_SWITCH_STATEMENT:
        execute_switch_statement(node);
        break;
    case NODE_BREAK_STATEMENT:
        // Signal to break out of the current loop/switch
        longjmp(break_env, 1);
        break;
    default:
        yyerror("Unknown statement type");
        break;
    }
}

void execute_statements(ASTNode *node)
{
    if (!node)
        return;
    if (node->type != NODE_STATEMENT_LIST)
    {
        execute_statement(node);
        return;
    }
    StatementList *current = node->data.statements;
    while (current)
    {
        execute_statement(current->statement);
        current = current->next;
    }
}

void execute_for_statement(ASTNode *node)
{
    // Execute initialization once
    if (node->data.for_stmt.init)
    {
        execute_statement(node->data.for_stmt.init);
    }

    while (1)
    {
        // Evaluate condition
        if (node->data.for_stmt.cond)
        {
            int cond_result = evaluate_expression(node->data.for_stmt.cond);
            if (!cond_result)
            {
                break;
            }
        }

        // Execute body
        if (node->data.for_stmt.body)
        {
            execute_statement(node->data.for_stmt.body);
        }

        // Execute increment
        if (node->data.for_stmt.incr)
        {
            execute_statement(node->data.for_stmt.incr);
        }
    }
}

void execute_while_statement(ASTNode *node)
{
    while (evaluate_expression(node->data.while_stmt.cond))
    {
        execute_statement(node->data.while_stmt.body);
    }
}

ASTNode *create_if_statement_node(ASTNode *condition, ASTNode *then_branch, ASTNode *else_branch)
{
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = NODE_IF_STATEMENT;
    node->data.if_stmt.condition = condition;
    node->data.if_stmt.then_branch = then_branch;
    node->data.if_stmt.else_branch = else_branch;
    return node;
}

ASTNode *create_string_literal_node(char *string)
{
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = NODE_STRING_LITERAL;
    node->data.name = string;
    return node;
}

ASTNode *create_switch_statement_node(ASTNode *expression, CaseNode *cases)
{
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = NODE_SWITCH_STATEMENT;
    node->data.switch_stmt.expression = expression;
    node->data.switch_stmt.cases = cases;
    return node;
}

CaseNode *create_case_node(ASTNode *value, ASTNode *statements)
{
    CaseNode *node = malloc(sizeof(CaseNode));
    node->value = value;
    node->statements = statements;
    node->next = NULL;
    return node;
}

CaseNode *create_default_case_node(ASTNode *statements)
{
    return create_case_node(NULL, statements); // NULL value indicates default case
}

CaseNode *append_case_list(CaseNode *list, CaseNode *case_node)
{
    if (!list)
        return case_node;
    CaseNode *current = list;
    while (current->next)
        current = current->next;
    current->next = case_node;
    return list;
}

ASTNode *create_break_node()
{
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = NODE_BREAK_STATEMENT;
    node->data.break_stmt = NULL;
    return node;
}

void execute_yapping_call(ArgumentList *args)
{
    if (!args)
    {
        yyerror("No arguments provided for yapping function call");
        exit(EXIT_FAILURE);
    }

    ASTNode *formatNode = args->expr;
    if (formatNode->type != NODE_STRING_LITERAL)
    {
        yyerror("First argument to yapping must be a string literal");
        return;
    }

    ArgumentList *cur = args->next;
    if (!cur)
    {
        yapping("%s", formatNode->data.name);
        return;
    }

    ASTNode *expr = cur->expr;

    // Handle float expressions
    if (is_float_expression(expr))
    {
        float val = evaluate_expression_float(expr);
        yapping(formatNode->data.name, val);
        return;
    }

    // Handle double expressions
    if (is_double_expression(expr))
    {
        double val = evaluate_expression_double(expr);
        yapping(formatNode->data.name, val);
        return;
    }

    // Check if we're dealing with an unsigned value
    bool is_unsigned = false;
    bool is_bool = false;

    if (expr->type == NODE_BOOLEAN)
    {
        is_bool = true;
    }
    if (expr->type == NODE_IDENTIFIER)
    {
        TypeModifiers mods = get_variable_modifiers(expr->data.name);
        is_unsigned = mods.is_unsigned;
        if (expr->var_type == VAR_BOOL)
        {
            is_bool = true;
        }
    }
    else
    {
        // Check if the node itself has unsigned modifier
        is_unsigned = expr->modifiers.is_unsigned;
    }

    if (strstr(formatNode->data.name, "%b") != NULL)
    {
        bool val = evaluate_expression_bool(expr);

        // 1) Build a new format string that changes "%b" -> "%s"
        char newFormat[256];
        strncpy(newFormat, formatNode->data.name, sizeof(newFormat));
        newFormat[sizeof(newFormat) - 1] = '\0';

        // replace the first "%B" with "%s"
        char *pos = strstr(newFormat, "%b");
        if (pos)
        {
            pos[1] = 's'; // i.e. 'b' -> 's'
        }

        // 2) Call yapping with that new format & "W"/"L"
        yapping(newFormat, val ? "W" : "L");
        return;
    }
    if (is_unsigned)
    {
        unsigned int val = (unsigned int)evaluate_expression_int(expr);
        if (strstr(formatNode->data.name, "%lu") != NULL)
        {
            yapping(formatNode->data.name, (unsigned long)val);
        }
        else if (strstr(formatNode->data.name, "%u") != NULL)
        {
            yapping(formatNode->data.name, val);
        }
        else
        {
            yapping("%u", val);
        }
        return;
    }

    // Handle regular integers
    int val = evaluate_expression_int(expr);
    yapping(formatNode->data.name, val);
}

void execute_yappin_call(ArgumentList *args)
{
    if (!args)
    {
        yyerror("No arguments provided for yappin function call");
        exit(EXIT_FAILURE);
    }

    ASTNode *formatNode = args->expr;
    if (formatNode->type != NODE_STRING_LITERAL)
    {
        yyerror("First argument to yappin must be a string literal");
        exit(EXIT_FAILURE);
    }

    ArgumentList *cur = args->next;
    if (!cur)
    {
        yappin("%s", formatNode->data.name);
        return;
    }

    ASTNode *expr = cur->expr;

    // Check if it's a boolean value
    if (expr->type == NODE_BOOLEAN ||
        (expr->type == NODE_IDENTIFIER && expr->var_type == VAR_BOOL))
    {
        int val = evaluate_expression_int(expr);
        if (strstr(formatNode->data.name, "%d") != NULL)
        {
            yappin(formatNode->data.name, val);
        }
        else
        {
            yappin(val ? "W" : "L");
        }
        return;
    }

    if (is_float_expression(expr))
    {
        float val = evaluate_expression_float(expr);
        yappin(formatNode->data.name, val);
        return;
    }

    if (is_double_expression(expr))
    {
        double val = evaluate_expression_double(expr);
        yappin(formatNode->data.name, val);
        return;
    }

    int val = evaluate_expression_int(expr);
    yappin(formatNode->data.name, val);
}

void execute_baka_call(ArgumentList *args)
{
    if (!args)
    {
        baka("\n");
        return;
    }
    // parse the first argument as a format string
    // parse subsequent arguments as integers, etc.
    // call "baka(formatString, val, ...)"
}

void execute_ragequit_call(ArgumentList *args)
{
    if (!args)
    {
        yyerror("No arguments provided for ragequit function call");
        exit(EXIT_FAILURE);
    }

    ASTNode *formatNode = args->expr;
    if (formatNode->type != NODE_INT)
    {
        yyerror("First argument to ragequit must be a integer");
        exit(EXIT_FAILURE);
    }

    ragequit(formatNode->data.ivalue);
}

void execute_chill_call(ArgumentList *args)
{
    if (!args)
    {
        yyerror("No arguments provided for chill function call");
        exit(EXIT_FAILURE);
    }

    ASTNode *formatNode = args->expr;
    if (formatNode->type != NODE_INT && !formatNode->modifiers.is_unsigned)
    {
        yyerror("First argument to chill must be a unsigned integer");
        exit(EXIT_FAILURE);
    }

    chill(formatNode->data.ivalue);
}

