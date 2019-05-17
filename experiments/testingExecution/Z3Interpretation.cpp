
#include "Z3Interpretation.h"


Z3_string Z3Interpretation::getModel(std::string z3_string) {
	Z3_config config = Z3_mk_config();
    Z3_context context = Z3_mk_context(config);
	Z3_solver solver = Z3_mk_solver(context);

	// convert z3_string into const char * z3_code
	char *z3_code = new char[z3_string.length() + 1];
	strcpy(z3_code, z3_string.c_str());

	Z3_ast fs = Z3_parse_smtlib2_string(context, z3_code, 0, 0, 0, 0, 0, 0);
	 Z3_solver_assert(context, solver, fs);
	 int result_solver = Z3_solver_check ((Z3_context) context, (Z3_solver) solver);
	 Z3_model model = Z3_solver_get_model ((Z3_context) context,  (Z3_solver) solver);


	 delete [] z3_code;
	 return Z3_model_to_string ((Z3_context) context,  model);
}


void Z3Interpretation::setJumpSymbol_one(std::string &z3_code) {
	 std::string jump_symbol_one = " (assert (= jump_symbol #b1))";
	 z3_code.insert(z3_code.size(), jump_symbol_one);
	// z3_code.insert(64, jump_symbol_one);
};

void Z3Interpretation::setJumpSymbol_zero(std::string &z3_code) {
	std::string jump_symbol_zero = " (assert (= jump_symbol #b0))";
	z3_code.insert(z3_code.size(), jump_symbol_zero);
	// z3_code.insert(64, jump_symbol_one);
}

void Z3Interpretation::setResult_toBe_Decimal(std::string &z3_code) {
	std::string condition = " (set-option :pp.bv-literals false)";
	z3_code.insert(z3_code.size(), condition);
}

std::map<std::string, int> Z3Interpretation::getModelResultsInDecimals(std::string z3_string) {
	setResult_toBe_Decimal(z3_string);
	
	std::map<std::string, int> resultMap;
	Z3_config config = Z3_mk_config();
    Z3_context context = Z3_mk_context(config);
	Z3_solver solver = Z3_mk_solver(context);


	Z3_ast fs = Z3_parse_smtlib2_string(context, z3_string.c_str(), 0, 0, 0, 0, 0, 0);
	Z3_solver_assert(context, solver, fs);
	int result_solver = Z3_solver_check ((Z3_context) context, (Z3_solver) solver);
	Z3_model model = Z3_solver_get_model ((Z3_context) context,  (Z3_solver) solver);
	
	int found_jumpSymbol = 0;
	std::string jump_symbol_function("(declare-fun jump_symbol () (_ BitVec 1))");
	unsigned n = Z3_model_get_num_consts(context, model);
		for(unsigned i = 0; i < n; i++) {
			//unsigned Z3_api rezultat = Z3_model_get_num_funcs_decl(context, model, i);
			Z3_func_decl fd = Z3_model_get_const_decl(context, model, i);
			if (Z3_model_has_interp(context, model, fd))
			{
				Z3_ast s = Z3_model_get_const_interp(context, model, fd);

				Z3_string modelFunction = Z3_func_decl_to_string (context, fd);
				Z3_string modelFunction_value = Z3_ast_to_string(context, s);

				std::string modelFunction_string = modelFunction;//copy the content into a new address
				std::string modelFunction_value_string = modelFunction_value;
				
				// before insert the modelFunction and value in the map, let make some changes on them
				if(found_jumpSymbol == 0 && modelFunction_string.compare(jump_symbol_function) == 0) {
					
					size_t found = modelFunction_string.find("jump_symbol");
					if(found != std::string::npos) {
						size_t position = modelFunction_value_string.find("bv");
						std::string newString =modelFunction_value_string.substr(position+2);
						int value = atoi(newString.c_str());
						resultMap.insert(std::pair<std::string, int>("jump_symbol", value));	
						found_jumpSymbol = 1;
					}
				}else {
					size_t found = modelFunction_string.find("|");
					std::string newString = modelFunction_string.substr(found+1);
					if(found != std::string::npos) {
						//substract the function
						size_t nextFound = newString.find("|");
						newString = newString.substr(0, nextFound);

						// substract the value
						size_t position = modelFunction_value_string.find("bv");
						std::string newString2 =modelFunction_value_string.substr(position+2);
						int value = atoi(newString2.c_str());
						resultMap.insert(std::pair<std::string, int>(newString, value));	
					}
				}
			}

		}
	return resultMap;
}

std::map<std::string, std::string> Z3Interpretation::getModelResults(std::string z3_string) {
	std::map<std::string, std::string> resultMap;
	Z3_config config = Z3_mk_config();
    Z3_context context = Z3_mk_context(config);
	Z3_solver solver = Z3_mk_solver(context);


	Z3_ast fs = Z3_parse_smtlib2_string(context, z3_string.c_str(), 0, 0, 0, 0, 0, 0);
	Z3_solver_assert(context, solver, fs);
	int result_solver = Z3_solver_check ((Z3_context) context, (Z3_solver) solver);
	Z3_model model = Z3_solver_get_model ((Z3_context) context,  (Z3_solver) solver);
	
	int found_jumpSymbol = 0;
	unsigned n = Z3_model_get_num_consts(context, model);
		for(unsigned i = 0; i < n; i++) {
			//unsigned Z3_api rezultat = Z3_model_get_num_funcs_decl(context, model, i);
			Z3_func_decl fd = Z3_model_get_const_decl(context, model, i);
			if (Z3_model_has_interp(context, model, fd))
			{
				Z3_ast s = Z3_model_get_const_interp(context, model, fd);

				Z3_string modelFunction = Z3_func_decl_to_string (context, fd);
				Z3_string modelFunction_value = Z3_ast_to_string(context, s);

				std::string modelFunction_string = modelFunction;
				std::string modelFunction_value_string = modelFunction_value;
				
				// before insert the modelFunction and value in the map, let make some changes on them

				// first we verify if modelFunction is the jump_symbol, else others
				
				if(found_jumpSymbol == 0) { // mean we did not found yet the jump_symbol
					size_t found = modelFunction_string.find("jump_symbol");
					if(found != std::string::npos) {
						resultMap.insert(std::pair<std::string, std::string>("jump_symbol", modelFunction_value_string));	
						found_jumpSymbol = 1;
					}
				}else {
					size_t found = modelFunction_string.find("|");
					std::string newString = modelFunction_string.substr(found+1);
					if(found != std::string::npos) {
						//substract the function
						size_t nextFound = newString.find("|");
						newString = newString.substr(0, nextFound);

						resultMap.insert(std::pair<std::string, std::string>(newString, modelFunction_value_string));	
					}
				}
			}

		}
	return resultMap;
}

std::map<std::string, int> Z3Interpretation::get_Values_For_True_Branch(std::string z3_string) {
	std::string temp_code = z3_string;
	this->setJumpSymbol_one(temp_code);
	return getModelResultsInDecimals(temp_code);
}

std::map<std::string, int> Z3Interpretation::get_Values_For_False_Branch(std::string z3_string) {
	std::string temp_code = z3_string;
	this->setJumpSymbol_zero(temp_code);
	return getModelResultsInDecimals(temp_code);
}


void Z3Interpretation::test() {
	char *result = "(set-info :status sat) (declare-fun jump_symbol () (_ BitVec 1)) (declare-fun |s[0]| () (_ BitVec 8)) (assert (= (ite (= |s[0]| (_ bv65 8)) (_ bv1 1) (_ bv0 1)) jump_symbol))";	
	std::string z3_string(result);

	//setJumpSymbol_one(z3_string);
	//setResult_toBe_Decimal(z3_string);

	std::map<std::string, int> map1 = this->getModelResultsInDecimals(z3_string);
	std::map<std::string, std::string> map2 = this->getModelResults(z3_string);
}