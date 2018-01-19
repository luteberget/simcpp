#ifndef __TRAINDYNAMICS_H
#define __TRAINDYNAMICS_H

#include <cmath>
#include <vector>
#include <limits>
#include <utility>
using std::vector;
using std::pair;

struct LinearTrainParams {
  double max_acc;
  double max_brk;
  double max_vel;
  double length;
};

struct LinearTrainSpeedRestrictions {
  double vmax;                        // Current limitation
  vector<pair<double, double>> ahead; // Limitations ahead (dx,v)
};

struct LinearTrainStep {
  enum class Action { Accel, Brake, Coast };
  Action action;
  double dt;
};

pair<double, double> trainUpdate(const LinearTrainParams &p, double v,
                                 const LinearTrainStep &s);
LinearTrainStep trainStep(const LinearTrainParams &p, double max_x, double v,
                          const LinearTrainSpeedRestrictions &r);

#endif
