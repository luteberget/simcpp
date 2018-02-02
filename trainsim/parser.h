#include "inputs.h"
#include <string>
using std::string;

InfrastructureSpec parse_infrastructure(std::istream& input);
Plan parse_plan(std::istream& input);
SimulatorInput parse_simulatorinput(std::istream& input);


string parse_string(std::istream& input);
bool parse_string_optional(std::istream& input, string& s);
int parse_int(std::istream& input);
size_t parse_size(std::istream& input);
double parse_double(std::istream& input);
void expect(std::istream& input, string s);
Direction parse_dir(std::istream& input);
SwitchState parse_switchstate(std::istream& input);