#include "interpreter.h"
#include <regex.h>   
#include <stdint.h>
#include "customstring.h"
#include "entry.h"

variable variables[VARIABLES_COUNT];
int variables_count = 0;

extern int has_printed;

int assign_variable(char* id, int type, char* svalue, double dvalue) {

	for (int i = 0; i < variables_count; i++) {
		if (strcmp(variables[i].id, id) == 0) {
			variables[i].type = type;
			variables[i].svalue = concat_string(svalue, NULL);
			variables[i].dvalue = 0;
			if (type == NUM_VARIABLE)  variables[i].dvalue = dvalue;
			return 0;
		}
	}
	variables[variables_count].id = concat_string(id, NULL);
	variables[variables_count].type = type;
	variables[variables_count].svalue = concat_string(svalue, NULL);
	variables[variables_count].dvalue = 0;
	if (type == NUM_VARIABLE) variables[variables_count].dvalue = dvalue;
	variables_count++;
	return 0;
}

int assign_field(int field_number, char* value) {
	char vname[3];
	vname[0] = '$'; vname[1] = (uint8_t)(field_number) + (uint8_t)'0'; vname[2] = '\0';
	assign_variable(vname, STR_VARIABLE, value, 0);
}

int get_variable(char* id, variable** var) {
	for (int i = 0; i < variables_count; i++) {
		if (strcmp(variables[i].id, id) == 0) {
			*var = &(variables[i]);
			return 0;
		}
	}
	
	die(concat_string("No such variable ", id), VAR_NOT_FOUND_ERROR);
	return -1;
}

char* get_string_from_variable(char* id) {
	for (int i = 0; i < variables_count; i++) {
		if (strcmp(variables[i].id, id) == 0) {
			return variables[i].svalue;

		}
	}
	
	die(concat_string("No such variable ", id), VAR_NOT_FOUND_ERROR);
	return "";
}

int print_variables() {
	for (int i = 0; i < variables_count; i++) {
		if (variables[i].type == NUM_VARIABLE) {
			printstr(variables[i].id); printstr("\t"); 
			printdouble(variables[i].dvalue);
		}
		if (variables[i].type == STR_VARIABLE) {
			printstr(variables[i].id); printstr("\t"); 
			printstr(variables[i].svalue);
		}
		printstr("\n");
	}
}

int get_num_from_value(entry* value, double* store) {
	if (((entry*)(value->argv)[0])->type == none) {
		char* s = ((entry*)(value->argv)[0])->text;
		*store = atof(s);
		return 0;
	}
	if (((entry*)(value->argv)[0])->type == id) {
		variable* var;
		get_variable(((entry*)(value->argv)[0])->text, &var);
		if (var->type == NUM_VARIABLE) *store = var->dvalue;
		else {
			//warning("string to number conversion");
			*store = 0;
			return -1;
		}
		return 0;
	}
}
int get_str_from_value(entry* value, char** store) {
	if (((entry*)(value->argv)[0])->type != id) {
		char* s = ((entry*)(value->argv)[0])->text;
		*store = s;
		return 0;
	}
	if (((entry*)(value->argv)[0])->type == id) {
		variable* var;
		get_variable(((entry*)(value->argv)[0])->text, &var);
		if (var->type == STR_VARIABLE) *store = var->svalue;
		else {
			warning("number to string conversion");
			char output[50];

			snprintf(output, 50, "%f", var->dvalue);
			*store = output;
		}
		return 0;
	}
}
int calculate_op(entry* cur, double* result) {
	double left_side = 0;
	double right_side = 0;

	if (cur->type == value) {

		int res = get_num_from_value(cur, &left_side);
		if (((entry*)(cur->argv)[0])->type == strvalue) {
			//have string right now
			assign_variable("!!strres", STR_VARIABLE, ((entry*)(cur->argv)[0])->text, 0);
			return -1;
		}
		if (res < 0) {
			//have string over there
			char* tmp;
			get_str_from_value(cur, &tmp);
			assign_variable("!!strres", STR_VARIABLE, tmp, 0);
			return -1;
		}

		get_num_from_value(cur, &left_side);
		*result = left_side;

		return 0;
	}
	if (cur->type == unaryop) {
		calculate_op((cur->argv)[0], &left_side);
		if (cur->op == '-') *result = -1 * left_side;
		if (cur->op == '!') *result = (left_side != 0) ? 0 : 1;
		return 0;
	}
	if (cur->type == binaryop) {
		
		char* left_str = NULL;
		char* right_str = NULL;
		int left_side_res = calculate_op((cur->argv)[0], &left_side);
		if (left_side_res < 0) left_str = get_string_from_variable("!!strres");
		int right_side_res = calculate_op((cur->argv)[1], &right_side);
		if (right_side_res < 0) right_str = get_string_from_variable("!!strres");
		
		*result = 0;
		if (cur->op == '+') {
			*result = left_side + right_side;
			if ((left_side_res < 0) && (right_side_res < 0)) {
				assign_variable("!!strres", STR_VARIABLE, concat_string(left_str, right_str), 0);
				*result = 1;
				return -1;
			}
		}
		if (cur->op == '-') *result = left_side - right_side;
		if (cur->op == '*') *result = left_side * right_side;
		if (cur->op == '/') *result = left_side / right_side;
		
		if (cur->op == '>') {
			if (cur->op2 == '=') *result = (left_side >= right_side) ? 1 : 0;
								else *result = (left_side > right_side) ? 1 : 0;
		}	
		if (cur->op == '<') {
			if (cur->op2 == '=') *result = (left_side <= right_side) ? 1 : 0;
								else *result = (left_side < right_side) ? 1 : 0;
		}
		if ((cur->op == '=') && (cur->op2 == '=')) {
			if ((left_side_res < 0) && (right_side_res < 0)) {
				if (strcmp(left_str, right_str) == 0) *result = 1; else result = 0;
			}
			else *result = (left_side == right_side) ? 1 : 0;
		}
		if ((cur->op == '!') && (cur->op2 == '=')) *result = (left_side != right_side) ? 1 : 0;
		
		if ((cur->op == '&') && (cur->op2 == '&')) *result = ((left_side != 0) && (right_side != 0)) ? 1 : 0;
		if ((cur->op == '|') && (cur->op2 == '|')) *result = ((left_side != 0) || (right_side != 0)) ? 1 : 0;
		
		if (cur->op == '~') {

			//(cur->argv)[0] for id of string, (cur->argv)[1] for regex itself
			regex_t regex;
			int reti;

			variable* var;
			get_variable(((entry*)(cur->argv)[0])->text, &var);
			
			reti = regcomp(&regex, ((entry*)(cur->argv)[1])->text, 0);
			if (reti) {
				die("Could not compile regex", REGEX_ERROR);
			}
			*result = 0;
			reti = regexec(&regex, var->svalue, 0, NULL, 0);
			if (!reti) {
				*result = 1;
			}
			else if (reti == REG_NOMATCH) {
				*result = 0;
			}
		}
		
		return 0;
	}
	
	if (cur->type == funop) {
		char* id = ((entry*)(cur->argv)[0])->text;
		double val;
		int res = calculate_op((cur->argv)[1], &val);
		//math stuff
		if (strcmp(id, "sin") == 0) { *result = sin(val); return 0; }
		if (strcmp(id, "cos") == 0) { *result = cos(val); return 0; }
		if (strcmp(id, "exp") == 0) { *result = exp(val); return 0; }
		if (strcmp(id, "log") == 0) { *result = log(val); return 0; }
		if (strcmp(id, "sqrt") == 0) { *result = sqrt(val); return 0; }
		if (strcmp(id, "int") == 0) { 
			*result = (int)val; 
			if (res < 0) *result = (int)(atof(get_string_from_variable("!!strres")));
			return 0; 
		} 
		die(concat_string("No such function", id), SUB_NOT_FOUND_ERROR);
		return 1;
	}
}

int run_block(entry* cur) {
	if (cur->type == blockop) {
		printrun("run_blockop");
		for (int i = 0; i < cur->argc; i++) run_block((cur->argv)[i]);
		return 0;
	}  
	if (cur->type == exprop) {
		printrun("run_exprblock");
		return run_block((cur->argv)[0]);
	}
	if (cur->type == assignop) {
		printrun("run_assign");
		char* id = ((entry*)(cur->argv)[0])->text;

		double val;
		int res = calculate_op((cur->argv)[1], &val);
		if (res == 0) assign_variable(id, NUM_VARIABLE, "", val); //number
		if (res < 0) {
			variable* calcres;
			get_variable("!!strres", &calcres);
			assign_variable(id, STR_VARIABLE, calcres->svalue, 0);
		}
		return 0;
	}
	if (cur->type == ifop) {
		printrun("run_if");
		double result;
		calculate_op((cur->argv)[0], &result);
		if (result != 0) run_block((cur->argv)[1]);
				else
					if ((cur->argv)[2]!= NULL) run_block((cur->argv)[2]);
		return 0;
	}
	if (cur->type == whileop) {
		printrun("run_while");
		double result;
		calculate_op((cur->argv)[0], &result);
		while (result != 0) {
			run_block((cur->argv)[1]);
			calculate_op((cur->argv)[0], &result);
		}
		return 0;
	}
	if (cur->type == funop) {
		char* id = ((entry*)(cur->argv)[0])->text;
		//sout << run_func >" << id << "<\n";
		if (strcmp(id, "print") == 0) {
			double val;
			int res = calculate_op((cur->argv)[1], &val);
			if (res == 0) printdouble(val); //number
			if (res < 0) {
				variable* calcres;
				get_variable("!!strres", &calcres);
				printstr(calcres->svalue);
				
				
			}
			printstr(get_string_from_variable("OFS"));
			has_printed = 1;
			return 0;
		} 
		if (strcmp(id, "println") == 0) {
			double val;
			int res = calculate_op((cur->argv)[1], &val);
			if (res == 0) printdouble(val); //number
			if (res < 0) {
				variable* calcres;
				get_variable("!!strres", &calcres);
				printstr(calcres->svalue);
				has_printed = 1;
			}
			return 0;
		}
		if ((strcmp(id, "sub") == 0) || (strcmp(id, "gsub") == 0)) {
			if (cur->argc != 4) die("Incorrect regex function", REGEX_ERROR);
			int globsub = (strcmp(id, "gsub") == 0) ? 1 : 0;
			assign_variable("!!strres", STR_VARIABLE, "", 0);
			double res;
			calculate_op((cur->argv)[3], &res);
			char* repl_with = get_string_from_variable("!!strres");
			char* var_id = ((entry*)(cur->argv)[1])->text;
			char* var_id_val = get_string_from_variable(var_id);
			
			char* subres = NULL;
			if (!globsub) subres = regexp_substitute_string(var_id_val, ((entry*)(cur->argv)[2])->text, repl_with);
			else subres = regexp_global_substitute_string(var_id_val, ((entry*)(cur->argv)[2])->text, repl_with);
			
			assign_variable(var_id, STR_VARIABLE, subres, 0);
			return 0;
		}
		die(concat_string("No such method ", id), SUB_NOT_FOUND_ERROR);
	}
}
