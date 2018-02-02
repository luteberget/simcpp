#ifndef __VISUALIZE__PARSER__H
#define __VISUALIZE__PARSER__H
#include "imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"
#include <iostream>
#include "trainplot.h"

TrainPlot parse_plot(std::istream &in);

#endif