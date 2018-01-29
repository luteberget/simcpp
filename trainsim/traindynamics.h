#ifndef __TRAINDYNAMICS_H
#define __TRAINDYNAMICS_H

#include <cmath>
#include <vector>
#include <limits>
#include <utility>
#include "inputs.h"

using std::vector;
using std::pair;

struct LinearTrainSpeedRestrictions {
  double vmax;                        // Current limitation
  vector<pair<double, double>> ahead; // Limitations ahead (dx,v)
};

struct LinearTrainStep {
  TrainAction action;
  double dt;
};

pair<double, double> trainUpdate(const LinearTrainParams &p, double v,
                                 const LinearTrainStep &s);
LinearTrainStep trainStep(const LinearTrainParams &p, double max_x, double v,
                          const LinearTrainSpeedRestrictions &r);

#endif
