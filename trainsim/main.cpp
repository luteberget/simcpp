
#include "parser.h"
#include "simulate.h"

#include <iostream>
using std::cout;
using std::endl;

int main()
{
    try
    {
        auto p = parse_simulatorinput(std::cin);
        cout << "parsed " << p.infrastructure.driveGraph.size() << " objects" << endl;
        cout << "parsed " << p.infrastructure.routes.size() << " routes" << endl;
        cout << "parsed " << p.plan.items.size() << " plan items" << endl;

        auto output = simulate(p.infrastructure,p.plan);

        return 0;
    }
    catch (string s)
    {
        cout << s << endl;
        return 1;
    }
}