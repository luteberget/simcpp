#ifndef __SIMULATE__H
#define __SIMULATE__H

#include <functional>
#include "inputs.h"
#include "history.h"
using std::pair;

void simulate(InfrastructureSpec &infrastructure, Plan &plan,
			  std::function<bool(pair<double, HistoryItem>)> output);

#endif
