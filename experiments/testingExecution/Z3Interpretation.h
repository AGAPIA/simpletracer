#include "z3/include/z3.h"
#include <string>
#include <iostream>
#include <stdio.h>
#include <iterator> 
#include <cstring>
#include <map>


/*This Class is used to operate with Z3 framework functions */
class Z3Interpretation {
    public:

        // method will take a Z3 code and will return a model
        Z3_string getModel(std::string z3_string);

        // method will take Z3_code and add an jumpSymbol = 1 assert to it
        void setJumpSymbol_one(std::string &z3_code);
        // method will take Z3_code and add an jumpSymbol = 0 assert to it
        void setJumpSymbol_zero(std::string &z3_code);

        // this method add to Z3_code just a condition so that Z3_code to retun decimals in the model
        void setResult_toBe_Decimal(std::string &z3_code);

        // This method get a Z3_code as a string, and return a map with model, s[0] = 1, .....
        std:: map<std::string, int> getModelResultsInDecimals(std::string z3_string);

        // this method just return the model for a Z3_code
        std::map<std::string, std::string> getModelResults(std::string z3_string);

        // this method will take Z3_code, use the method setJumpsymol_one and the the values in decimal for the model
        std::map<std::string, int> get_Values_For_True_Branch(std::string z3_string);
        // this method will take Z3_code, use the method setJumpsymol_zero and the the values in decimal for the model
        std::map<std::string, int> get_Values_For_False_Branch(std::string z3_string);

        void test();



};
