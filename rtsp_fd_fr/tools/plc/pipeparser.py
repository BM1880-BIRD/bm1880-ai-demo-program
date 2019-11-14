#!/usr/bin/env python3

import ply.lex as lex
import ply.yacc as yacc
from enum import Enum

def _parser():
    tokens = (
        'NAME',
        'LPAREN',
        'RPAREN',
        'INTEGER',
        'DECIMAL',
        'STRING',
        'COMMA',
        'LBRACE',
        'RBRACE',
        'LBRACKET',
        'RBRACKET',
        'SHARP',
        'LT',
        'GT',
        'SCOPEOP',
        'ASSIGN',
        'SEMICOLON',
        'AMP'
    )

    t_ignore = " \t"

    def t_newline(t):
        r'\n+'
        t.lexer.lineno += t.value.count('\n')

    def t_error(t):
        t.lexer.skip(1)
        raise ValueError("Illegal character '{}'".format(t.value[0]))

    t_LPAREN     = r'\('
    t_RPAREN     = r'\)'
    t_LBRACKET = r'\['
    t_RBRACKET = r'\]'
    t_SCOPEOP    = r'::'
    t_COMMA      = r','
    t_NAME       = r'[a-zA-Z_][a-zA-Z0-9_]*'
    t_INTEGER    = r'[0-9]+'
    t_DECIMAL    = r'([0-9]|[1-9][0-9]+)\.[0-9]+'
    t_LT         = r'<'
    t_GT         = r'>'
    t_LBRACE     = r'\{'
    t_RBRACE     = r'\}'
    t_SHARP      = r'\#'
    t_ASSIGN     = r'='
    t_SEMICOLON  = r';'
    t_AMP        = r'&'

    def t_STRING(t):
        r'"(\\.|[^"\\])*"'
        return t

    start = 'document'
    symbols = set()

    def p_document(p):
        '''document : includes variables pipelines'''
        p[0] = {'includes': p[1], 'variables': p[2], 'pipelines': p[3]}

    def p_includes_empty(p):
        '''includes : '''
        p[0] = []

    def p_includes_concat(p):
        '''includes : include includes'''
        p[0] = [p[1]] + p[2]

    def p_include(p):
        '''include : SHARP NAME STRING'''
        p[0] = ''.join(p[1:])

    def p_variables_empty(p):
        '''variables : '''
        p[0] = []

    def p_variables_concat(p):
        '''variables : variable variables'''
        p[0] = [p[1]] + p[2]

    def p_variable(p):
        '''variable : NAME ASSIGN component'''

    def p_pipelines_empty(p):
        '''pipelines : '''
        p[0] = []

    def p_pipelines_concat(p):
        '''pipelines : pipeline pipelines'''
        p[0] = [p[1]] + p[2]

    def p_pipeline(p):
        '''pipeline : LBRACE component_seq RBRACE'''
        p[0] = p[2]

    def p_component_seq_empty(p):
        '''component_seq : '''
        p[0] = []

    def p_component_seq(p):
        '''component_seq : component component_seq'''
        p[0] = [p[1]] + p[2]

    def p_component(p):
        'component : class LPAREN argument_list RPAREN'
        p[0] = (p[1], p[3])

    def p_component_lambda(p):
        'component : LBRACKET RBRACKET LPAREN parameter_list RPAREN LBRACE statements RBRACE'
        print('lambda: ', p[1:])
        p[0] = ('', ' '.join(p[1:]))

    def p_parameter_list(p):
        '''parameter_list :
                          | parameters'''
        p[0] = ''.join(p[1:])

    def p_parameters(p):
        '''parameters : parameter
                      | parameter COMMA parameters'''
        p[0] = ''.join(p[1:])

    def p_parameter(p):
        '''parameter : class NAME
                     | class AMP NAME
                     | class AMP AMP NAME'''
        print(p[1:])
        p[0] = p[1] + ' ' + ''.join(p[2:])

    def p_statements_empty(p):
        'statements : '
        p[0] = ''

    def p_statements(p):
        'statements : statement statements'
        p[0] = p[1] + ' ' + p[2]

    def p_statement(p):
        'statement : SEMICOLON'
        p[0] = p[1] + p[2]

    def p_identifier_single(p):
        '''identifier : identifier_cat '''
        symbols.add(p[1])
        p[0] = p[1]

    def p_identifier_cat(p):
        '''identifier_cat : NAME
                          | identifier_cat SCOPEOP NAME'''
        p[0] = ''.join(p[1:])

    def p_class(p):
        '''class : identifier
                 | identifier LT GT
                 | identifier LT template_argument_list GT'''
        p[0] = ''.join(p[1:])

    def p_temp_argument_list(p):
        '''template_argument_list : template_argument
                                  | template_argument COMMA template_argument_list'''
        p[0] = ''.join(p[1:])

    def p_temp_argument(p):
        '''template_argument : class
                             | INTEGER'''
        p[0] = ''.join(p[1:])

    def p_arguments_list_not_null(p):
        '''argument_list :
                         | arguments'''
        p[0] = ''.join(p[1:])

    def p_arguments(p):
        '''arguments : literal
                     | literal COMMA arguments'''
        p[0] = ''.join(p[1:])

    def p_literal(p):
        '''literal : INTEGER
                   | DECIMAL
                   | STRING
                   | initialization'''
        p[0] = p[1]

    def p_initialization(p):
        '''initialization : class LBRACE argument_list RBRACE
                          | LBRACE argument_list RBRACE
                          | identifier'''
        p[0] = ''.join(p[1:])

    def p_error(p):
        raise ValueError("Syntax error at '{}'".format(p))

    lex.lex()
    return symbols, yacc.yacc()

def parse(s):
    mapping, parser = _parser()
    return mapping, parser.parse(s)
