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
                                 const LinearTrainStep &s) {
  if (s.action == LinearTrainStep::Action::Accel) {
    double dx = v * s.dt + 0.5 * p.max_acc * s.dt * s.dt;
    return {dx, v + s.dt * p.max_acc};
  } else if (s.action == LinearTrainStep::Action::Brake) {
    double dx = v * s.dt - 0.5 * p.max_brk * s.dt * s.dt;
    return {dx, v - s.dt * p.max_brk};
  } else if (s.action == LinearTrainStep::Action::Coast) {
    return {v * s.dt, v};
  }
  return {0, 0}; // impossible
}

LinearTrainStep trainStep(const LinearTrainParams &p, double max_x, double v,
                          const LinearTrainSpeedRestrictions &r) {

  // Can we accelerate?
  if (v + 1e-10 < r.vmax) {
    // Acceleration to maximum speed
    double a_dt = (r.vmax - v) / p.max_acc;
    double a_dx = v * a_dt + 0.5 * p.max_acc * a_dt * a_dt;
    // Alternatively: double a_dx = (r.vmax*r.vmax - v*v)/(2*p.max_acc);

    double accel_time = a_dt;
    double brake_time = 0.0;
    for (auto &ri : r.ahead) {
      double r_dx = ri.first;
      double r_v = ri.second;

      double b_dx = (r.vmax * r.vmax - r_v * r_v) / (2 * p.max_brk);
      if (r_dx < a_dx + b_dx) {
        double i_dx = (2 * p.max_brk * r_dx + r_v * r_v - v * v) /
                      (2 * (p.max_acc + p.max_brk));
        double i_v = std::sqrt(2 * p.max_acc * i_dx + v * v);
        double i_dt = (i_v - v) / p.max_acc;
        if (i_dt < accel_time) {
          accel_time = i_dt;
          brake_time = (v - r_v) / p.max_brk;
        }
      }
    }

    if (accel_time <= 1e-10) {
      return {LinearTrainStep::Action::Brake, brake_time};
    } else {
      return {LinearTrainStep::Action::Accel, accel_time};
    }
  } else { // No further acceleration allowed, try coasting
    double coast_time = std::numeric_limits<double>::infinity();
    double brake_time = 0.0;
    for (auto &ri : r.ahead) {
      double r_dx = ri.first;
      double r_v = ri.second;
      double b_dx = (v * v - r_v * r_v) / (2 * p.max_brk);
      double b_dt = (v - r_v) / p.max_brk;
      double d_dx = r_dx - b_dx;
      double d_dt = d_dx / v;
      double r_dt = r_dx / v;
      if (d_dt < coast_time) {
        // Coast until start of braking curve.
        coast_time = d_dt;
        brake_time = b_dt;
      }
      if (r_dt < coast_time) {
        // Also, don't go further than next restriction,
        // because we might start accelerating there.
        coast_time = r_dt;
        brake_time = b_dt;
      }
    }

    if (coast_time <= 1e-10) {
      return {LinearTrainStep::Action::Brake, brake_time};
    } else {
      return {LinearTrainStep::Action::Coast, coast_time};
    }
  }
}

#endif
