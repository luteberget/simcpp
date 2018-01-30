#ifndef __TRAINDYNAMICS_H
#define __TRAINDYNAMICS_H

#include <cmath>
#include <vector>
#include <limits>
#include <utility>
#include "inputs.h"

using std::vector;
using std::pair;

struct PosVel
{
    double dist;
    double velocity;
};

struct LinearTrainSpeedRestrictions {
  double vmax;                        // Current limitation
  vector<PosVel> ahead; // Limitations ahead (dx,v)
};

struct LinearTrainStep {
  TrainAction action;
  double dt;
};

PosVel trainUpdate(const LinearTrainParams &p, double v,
                                 const LinearTrainStep &s);
LinearTrainStep trainStep(const LinearTrainParams &p, double max_x, double v,
                          const LinearTrainSpeedRestrictions &r);

#endif
