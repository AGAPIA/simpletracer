#include "z3/include/z3.h"
#include <string>
#include <iostream>
#include <stdio.h>
#include <iterator> 
#include <cstring>
#include <map>



class Z3Interpretation {
    public:

        Z3_string getModel(std::string z3_string);
        void setJumpSymbol_one(std::string &z3_code);
        void setJumpSymbol_zero(std::string &z3_code);
        void setResult_toBe_Decimal(std::string &z3_code);
        std:: map<std::string, int> getModelResultsInDecimals(std::string z3_string);
        std::map<std::string, std::string> getModelResults(std::string z3_string);


        std::map<std::string, int> get_Values_For_True_Branch(std::string z3_string);
        std::map<std::string, int> get_Values_For_False_Branch(std::string z3_string);

        void test();



};
