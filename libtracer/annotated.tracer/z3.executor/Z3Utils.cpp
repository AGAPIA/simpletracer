#include "Z3Utils.h"
#include <stdio.h>
#include <stdarg.h>

Logger Z3Utils::s_globalLogger;
bool Z3Utils::m_symOption = false;

// TODO: check format options !!
void Z3Utils::PrintAST(Z3_context context, Z3_ast ast) 
{
	// Z3_set_ast_print_mode(context, (Z3_ast_print_mode)2);
	// Z3_set_ast_print_mode(context, Z3_PRINT_SMTLIB2_COMPLIANT);
    if (m_symOption)
    {
	    s_globalLogger.Log("AST in text format:%s\n", Z3_ast_to_string(context, ast));
    }
}

void Z3Utils::PrintInfo(const bool fromSymEngine, const char* format, ...)
{
    if (fromSymEngine && m_symOption == false)
        return;

    va_list arglist;

    va_start( arglist, format );
    s_globalLogger.Log(true, arglist, format);
    va_end( arglist );
}

void Z3Utils::initLoggingSystem(FILE* outFile, bool symOption)
{
    m_symOption = symOption;

    s_globalLogger.EnableLog();
    s_globalLogger.SetLoggingToFile(outFile);
}

