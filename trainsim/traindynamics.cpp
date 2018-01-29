#include "traindynamics.h"
#include <stdio.h>
using std::pair;
using std::vector;

pair<double, double> trainUpdate(const LinearTrainParams &p, double v,
                                 const LinearTrainStep &s)
{
  if (s.action == TrainAction::Accel)
  {
    double dx = v * s.dt + 0.5 * p.max_acc * s.dt * s.dt;
    return {dx, v + s.dt * p.max_acc};
  }
  else if (s.action == TrainAction::Brake)
  {
    double dx = v * s.dt - 0.5 * p.max_brk * s.dt * s.dt;
    return {dx, v - s.dt * p.max_brk};
  }
  else if (s.action == TrainAction::Coast)
  {
    return {v * s.dt, v};
  }
  return {0, 0}; // impossible
}

LinearTrainStep trainStep(const LinearTrainParams &p, double max_x, double v,
                          const LinearTrainSpeedRestrictions &r)
{

  printf("entering trainStep(-, %g, %g, {", max_x, v);
  for (auto &x : r.ahead)
    printf("{%g, %g} ", x.first, x.second);
  printf("}\n");

  // Can we accelerate?
  if (v + 1e-4 < r.vmax)
  {
    // Acceleration to maximum speed
    double a_dt = (r.vmax - v) / p.max_acc;
    double a_dx = v * a_dt + 0.5 * p.max_acc * a_dt * a_dt;
    // Alternatively: double a_dx = (r.vmax*r.vmax - v*v)/(2*p.max_acc);
    //

    double target_max = r.vmax;
    if (a_dx > max_x)
    {
      printf("   acceleration bounded by max_x %g ", a_dx);
      a_dx = max_x;
      double new_v = std::sqrt(2 * p.max_acc * a_dx + v * v);
      target_max = new_v;
      a_dt = (new_v - v) / p.max_acc;
      printf(" %g %g %g\n", max_x, target_max, a_dt);
    }

    double accel_time = a_dt;
    double brake_time = 0.0;
    for (auto &ri : r.ahead)
    {
      double r_dx = ri.first;
      double r_v = ri.second;

      double b_dx = (target_max * target_max - r_v * r_v) / (2 * p.max_brk);
      if (r_dx < a_dx + b_dx)
      {
        double i_dx = (2 * p.max_brk * r_dx + r_v * r_v - v * v) /
                      (2 * (p.max_acc + p.max_brk));
        double i_v = std::sqrt(2 * p.max_acc * i_dx + v * v);
        double i_dt = (i_v - v) / p.max_acc;
        double b_dx = (v * v - r_v * r_v) / (2 * p.max_brk);
        double b_dt = (v - r_v) / p.max_brk;
        if (b_dx > max_x)
        {
          printf("  braking to (%g,%g) bounded by max_x\n", r_dx, r_v);
          b_dx = max_x;
          double new_v = std::sqrt(v * v - 2 * p.max_brk * b_dx);
          b_dt = (v - new_v) / p.max_brk;
          printf("  new_v %g,  b_dt %g\n", new_v, b_dt);
        }
        if (i_dt < accel_time)
        {
          accel_time = i_dt;
          brake_time = b_dt;
        }
      }
    }

    if (accel_time <= 1e-4)
    {
      return {TrainAction::Brake, brake_time};
    }
    else
    {
      return {TrainAction::Accel, accel_time};
    }
  }
  else
  { // No further acceleration allowed, try coasting
    double coast_time = std::numeric_limits<double>::infinity();
    if (max_x < std::numeric_limits<double>::infinity())
    {
      coast_time = max_x / v;
    }
    double brake_time = 0.0;
    for (auto &ri : r.ahead)
    {
      double r_dx = ri.first;
      double r_v = ri.second;
      double b_dx = (v * v - r_v * r_v) / (2 * p.max_brk);
      double b_dt = (v - r_v) / p.max_brk;
      double d_dx = r_dx - b_dx;
      double d_dt = d_dx / v;
      double r_dt = r_dx / v;

      if (b_dx > max_x)
      {
        b_dx = max_x;
        double new_v = std::sqrt(v * v - 2 * p.max_brk * b_dx);
        b_dt = (v - new_v) / p.max_brk;
      }

      if (d_dt < coast_time)
      {
        // Coast until start of braking curve.
        coast_time = d_dt;
        brake_time = b_dt;
      }

      if (r_dt < coast_time)
      {
        // Also, don't go further than next restriction,
        // because we might start accelerating there.
        coast_time = r_dt;
        brake_time = b_dt;
      }
    }

    if (coast_time <= 1e-4)
    {
      return {TrainAction::Brake, brake_time};
    }
    else
    {
      return {TrainAction::Coast, coast_time};
    }
  }
}
