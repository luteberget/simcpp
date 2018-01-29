#ifndef __SWITCH_H
#define __SWITCH_H

#include <cmath>
#include "infrastructure_object.h"
#include "resource.h"
#include "../simobj.h"

typedef shared_ptr<Simulation> Sim;

template <typename T> int sgn(T val) { return (T(0) < val) - (val < T(0)); }

class Switch : public Resource, public ISObj {
private:
  double position = 0.0;
  shared_ptr<Process> turning;
  OBSERVABLE_PROPERTY(SwitchState, state, SwitchState::Left)
  void set_position(double p) {
    printf("Switch set to %g.\n", p);
    position = p;
    if (p == 0.0)
      set_state(SwitchState::Left);
    else if (p == 1.0)
      set_state(SwitchState::Right);
    else
      set_state(SwitchState::Unknown);
  }

public:
  Switch(Sim s, SwitchState state) : Resource(s), state(state) {}

  Link left, right, entry;
  Direction splitDir;


  Link *next(Direction dir) override {
    if (dir == this->splitDir) {
      if (this->state == SwitchState::Left)
        return &this->left;
      if (this->state == SwitchState::Right)
        return &this->right;
      return nullptr; // TODO detect derailing
    } else {
      // TODO detect derailing
      return &this->entry;
    }
  }
  shared_ptr<Process> turn(SwitchState s) {
    if (this->turning != nullptr && !this->turning->is_triggered())
      turning->abort();
    if (s == this->state) {
      return nullptr;
    }
    this->set_state(SwitchState::Unknown);
    if (s == SwitchState::Left) {
      turning = env->start_process<TurnSwitch>(this, this->position, 0.0);
    }
    if (s == SwitchState::Right) {
      turning = env->start_process<TurnSwitch>(this, this->position, 1.0);
    }
    return turning;
  }

  class TurnSwitch : public Process {
    Switch *sw;
    double start, end, start_time;
    double TURNING_TIME = 5.0;

  public:
    TurnSwitch(Sim s, Switch *sw, double start, double end)
        : Process(s), sw(sw), start(start), end(end),
          start_time(sim->get_now()) {}
    bool Run() {
      PT_BEGIN();
      printf("Turning switch @%g.\n", this->sim->get_now());
      PROC_WAIT_FOR(
          sim->timeout(TURNING_TIME * std::abs(this->start - this->end)));
      printf("Switch turned @%g.\n", this->sim->get_now());
      sw->set_position(end);
      sw->turning = nullptr;
      PT_END();
    }

    void Aborted() override {
      double length = (this->sim->get_now() - this->start_time) / TURNING_TIME;
      double end_pos = this->start + sgn(this->end - this->start) * length;
      printf("Switch turn aborted at %g %g\n", this->sim->get_now(), end_pos);
      sw->set_position(end_pos);
      sw->turning =
          nullptr; // TODO create Success and Finally virtual functions
    }
  };
};


#endif