#ifndef Z3UTILS_H
#define Z3UTILS_H

#include "z3.h"
#include "Logger.h"

class Z3Utils
{
    public:
        static void PrintAST(Z3_context context, Z3_ast ast);
        //fromSymEngine must be true if this is a symbolic engine expression, otherwise it can be false (e.g. general info)
        static void PrintInfo(const bool fromSymEngine, const char* format, ...);

        static void initLoggingSystem(FILE* fileToOutputLog, bool symOption=false);
    public:
        static Logger s_globalLogger;
        static bool m_symOption; // Whether to print the costly ASTs or not
};

#ifdef IS_DEBUG_BUILD
#define PRINT_DEBUG_SYMBOLIC
#endif

#ifndef PRINT_DEBUG_SYMBOLIC
#define PRINTF_SYM
#define PRINTF_INFO
#define PRINT_AST
#else
#define PRINTF_SYM(format, ...) {Z3Utils::PrintInfo(true, (format), ##__VA_ARGS__);}
#define PRINTF_INFO(format, ...) {Z3Utils::PrintInfo(false, (format), ##__VA_ARGS__);}
#define PRINT_AST(context, z3ast) { Z3Utils::PrintAST(context, z3ast); }
#endif


#endif 
