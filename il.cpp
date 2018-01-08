#include "simcpp.h"
#include "simobj.h"
#include <utility>
#include <cmath>

using std::pair;
using std::make_pair;

template <typename T> int sgn(T val) {
  return (T(0) < val) - (val < T(0));
}

typedef shared_ptr<Simulation> Sim;

class Resource : protected EnvObj {
  OBSERVABLE_PROPERTY(bool, allocated, false)
public:
  Resource(Sim s) : EnvObj(s) {}
};

class TVD : public Resource {
  OBSERVABLE_PROPERTY(bool, occupied, false)
public:
  TVD(Sim s) : Resource(s) {}
};

enum class SwitchState { Left, Right, Unknown };

class Switch : public Resource {
private:
  double position = 0.0;
  shared_ptr<Process> turning;
  OBSERVABLE_PROPERTY(SwitchState, state, SwitchState::Left)
  void set_position(double p) {
     printf("Switch set to %g.\n",p);
     position = p;
     if(p == 0.0) set_state(SwitchState::Left);
     else if(p == 1.0) set_state(SwitchState::Right);
     else set_state(SwitchState::Unknown);
  }

public:
  Switch(Sim s, SwitchState state) : Resource(s), state(state) {}

  shared_ptr<Process> turn(SwitchState s) {
    if(this->turning != nullptr && !this->turning->is_triggered()) turning->abort();
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
    Switch *sw; double start,end,start_time;
    double TURNING_TIME = 5.0;
  public:
    TurnSwitch(Sim s, Switch *sw, double start, double end)
        : Process(s), sw(sw),start(start),end(end),start_time(sim->get_now()) {}
    bool Run() {
      PT_BEGIN();
      printf("Turning switch @%g.\n", this->sim->get_now());
      PROC_WAIT_FOR(sim->timeout(TURNING_TIME*std::abs(this->start-this->end)));
      printf("Switch turned @%g.\n", this->sim->get_now());
      sw->set_position(end);
      sw->turning = nullptr;
      PT_END();
    }

    void Aborted() override {
      double length = (this->sim->get_now() - this->start_time) / TURNING_TIME;
      double end_pos = this->start + sgn(this->end-this->start)*length;
      printf("Switch turn aborted at %g %g\n", this->sim->get_now(), end_pos);
      sw->set_position(end_pos);
      sw->turning = nullptr; // TODO create Success and Finally virtual functions
    }
  };
};

class Signal : protected EnvObj {
public:
  Signal(Sim s) : EnvObj(s) {}
  OBSERVABLE_PROPERTY(bool, green, false)
};

class Route : protected EnvObj {
  vector<pair<Switch *, SwitchState>> switchPositions;
  vector<TVD *> tvds;
  Signal *entrySignal;

public:
  Route(Sim s, Signal *entrySignal, vector<pair<Switch *, SwitchState>> swPos,
        vector<TVD *> tvds)
      : EnvObj(s), switchPositions(swPos), tvds(tvds),
        entrySignal(entrySignal) {}

  shared_ptr<Process> activate() { return this->env->start_process<RouteActivation>(this); }
private:
  bool resources_available() { 
    bool available = true;
    for(auto& t : this->tvds) available &= t->get_allocated();
    for(auto& s : this->switchPositions) available &= s.first->get_allocated();
    return available;
  }

  bool switches_correct() {
    bool correct = true;
    for(auto& s : this->switchPositions) 
      correct &= s.first->get_state() == s.second;
    return correct;
  }

  class RouteActivation : public Process {
    Route* route;
    void reserve_resources() {
      for(auto& t : this->route->tvds) t->set_allocated(true);
      for(auto& s : this->route->switchPositions) s.first->set_allocated(true);
    }

    shared_ptr<Process> turn_switches() {
      vector<shared_ptr<Event>> all;
      for(auto& s : this->route->switchPositions) all.push_back(s.first->turn(s.second));
      return this->sim->start_process<AllOf>(all);
    }

    shared_ptr<Process> wait_busy_resources() {
      vector<shared_ptr<Event>> all;
      for(auto& t : this->route->tvds) {
        if(t->get_allocated()) all.push_back(t->allocated_event);
      }
      for(auto& s : this->route->switchPositions)  {
        if(s.first->get_allocated()) all.push_back(s.first->allocated_event);
      }
      return this->sim->start_process<AllOf>(all);
    }

    public:
      RouteActivation(Sim s, Route* r) : Process(s), route(r) {}
  
    virtual bool Run() {
      PT_BEGIN();
      while(!this->route->resources_available()) {
        PROC_WAIT_FOR(wait_busy_resources());
      }
      reserve_resources();
      while(!this->route->switches_correct()) {
        PROC_WAIT_FOR(turn_switches());
      }
      // Release triggers
      // Green entry signal
      this->route->open_signal();
      PT_END();
    }
  };

  class CatchSignal : public Process {
    Signal *sig;
    TVD *tvd;

  public:
    CatchSignal(Sim s, Signal *sig, TVD *tvd)
        : Process(s), sig(sig), tvd(tvd) {}
    virtual bool Run() {
      PT_BEGIN();
      PROC_WAIT_FOR(tvd->occupied_event);
      sig->set_green(false);
      PT_END();
    }
  };

  class ReleaseTrigger : public Process {
    TVD *tvd;
    vector<Resource *> resources;

  public:
    ReleaseTrigger(Sim s, TVD *tvd, vector<Resource *> resources)
        : Process(s), tvd(tvd), resources(resources) {}
    virtual bool Run() {
      PT_BEGIN();
      PROC_WAIT_FOR(tvd->occupied_event);
      PROC_WAIT_FOR(tvd->occupied_event);
      for (auto &r : resources)
        r->set_allocated(false);
      PT_END();
    }
  };

  shared_ptr<Event> open_signal() {
    this->entrySignal->set_green(true);
    return this->env->start_process<CatchSignal>(entrySignal, tvds[0]);
  }
};

struct TrainRunSpec {
};

class Train : public Process {
  public:
    Train(Sim s, TrainRunSpec spec) : Process(s) {}
    bool Run() override {
      PT_BEGIN();
      PT_END();
    }
};

struct Infrastructure {

};

struct InterlockingSpec {
};

struct World {
  vector<TVD> tvds;
  vector<Switch> switches;
  vector<Signal> signals;
  vector<Route> routes;
  vector<Train> trains;
};

struct ReleaseSpec {
  TVD* trigger;
  vector<Resource*> resources;
};


struct Plan {
  enum class PlanItemType { Route, Train };
  vector<pair<PlanItemType,size_t>> activations;
  vector<TrainRunSpec> trains;
};


void test_plan(const Infrastructure& is, const InterlockingSpec& il, const Plan& p) {
  auto sim = Simulation::create();
  World world;// = create_world(sim,is,il);
  for(auto& i : p.activations) {
    if(i.first == Plan::PlanItemType::Route) {
      sim->advance_to(world.routes[i.second].activate());
    } else if(i.first == Plan::PlanItemType::Train) {
      sim->start_process<Train>(p.trains[i.second]);
    }
  }
  sim->run();
  // TODO return whether all *Train*s terminate successfully.
}

int main() {
  auto sim = Simulation::create();
  World world;
  world.tvds.emplace_back(TVD(sim));
  world.switches.emplace_back(Switch(sim, SwitchState::Left));
  world.switches.emplace_back(Switch(sim, SwitchState::Left));
  world.switches[0].turn(SwitchState::Right);
  sim->advance_by(1);
  world.switches[1].turn(SwitchState::Right);
  sim->advance_by(1);
  world.switches[0].turn(SwitchState::Left);
  sim->run();
  return 0;
}
