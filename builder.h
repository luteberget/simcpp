#ifndef __BUILDER_H
#define __BUILDER_H

#include "il_inputspec.h"
#include "il.h"

extern "C" {

Plan *new_plan(void);
TrainRunSpec *new_trainrunspec(double acc, double brk, double vmax, double len,
                               int dir, double authLen, size_t startObj);
void add_trainrunspec_stop(TrainRunSpec *s, double stop);
void add_plan_train(Plan *p, TrainRunSpec *s);
void add_plan_route(Plan *p, size_t);
double run_plan(InfrastructureSpec *is, Plan *p);
void free_plan(Plan *p);

InfrastructureSpec *new_infrastructurespec(void);
void free_infrastructurespec(InfrastructureSpec *is);

ReleaseSpec *new_release(size_t trigger);
void add_release_resource(ReleaseSpec *r, size_t resource);
RouteSpec *new_route(const char *name, int entry_signal, double length);
void add_release(RouteSpec *route, ReleaseSpec *release);
void add_route(InfrastructureSpec *is, size_t idx, RouteSpec *r);
void add_route_switch(RouteSpec *r, size_t sw, int state);
void add_route_tvd(RouteSpec *r, size_t tvd);

void add_signal(InfrastructureSpec *is, const char *name, int upIdx,
                double upLength, int downIdx, double downLength, int upDir,
                size_t detector);
void add_detector(InfrastructureSpec *is, const char *name, int upIdx,
                  double upLength, int downIdx, double downLength, int upTvd,
                  int downTvd);
void add_sight(InfrastructureSpec *is, const char *name, int upIdx,
               double upLength, int downIdx, double downLength, size_t sigref,
               double dist);
void add_boundary(InfrastructureSpec *is, const char *name, int upIdx,
                  double upLength, int downIdx, double downLength);
void add_stop(InfrastructureSpec *is, const char *name, int upIdx,
              double upLength, int downIdx, double downLength);
void add_tvd(InfrastructureSpec *is, const char *name);
void add_switch(InfrastructureSpec *is, const char *name, int upIdx1,
                double upLength1, int upIdx2, double upLength2, int downIdx1,
                double downLength1, int downIdx2, double downLength2,
                int state);
}
#endif
