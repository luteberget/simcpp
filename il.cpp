#include "simcpp.h"
#include "simobj.h"
#include <utility>

using std::pair;
using std::make_pair;

typedef shared_ptr<Simulation> Sim;

class Resource : protected EnvObj {
  OBSERVABLE_PROPERTY(bool, allocated, false)
  public: Resource(Sim s) : EnvObj(s) {}
};

class TVD : protected Resource {
  OBSERVABLE_PROPERTY(bool, occupied, false)
  public: TVD(Sim s) : Resource(s) {}
};

enum class SwitchState { Left, Right, Unknown };

class Switch : protected Resource {
  private:
    double position = 0.0;
  OBSERVABLE_PROPERTY(SwitchState, state, SwitchState::Left)
  public: Switch(Sim s,SwitchState state) : Resource(s),state(state) {}

  void turn(SwitchState s) {
    if(s == this->state) { return; } 
    else if(s == SwitchState::Unknown) { this->set_state(s); }
    else if(s == SwitchState::Left) { env->start_process<TurnSwitch>(this, this->position, 0.0); }
    else if(s == SwitchState::Right) { env->start_process<TurnSwitch>(this, this->position, 1.0); }

  }

  class TurnSwitch : public Process  {
   Switch* sw;
  public:
   TurnSwitch(Sim s, Switch* sw, double start, double end) : Process(s),sw(sw) {}
   ~TurnSwitch() {printf("turnswitch deconstructor.\n");}
   virtual bool Run() {
     PT_BEGIN();
     printf("Turning switch @%g.\n",sim->get_now());
     PROC_WAIT_FOR(sim->timeout(5.0));
     printf("Switch turned @%g.\n",sim->get_now());
     //sw->set_state(
     PT_END();
   }
  };
};

class Signal : protected EnvObj {
  public: Signal(Sim s): EnvObj(s) {}
  OBSERVABLE_PROPERTY(bool, green, false)
};

class Route : protected EnvObj {
  vector<pair<Switch*, SwitchState>> switchPositions;
  vector<TVD*> tvds;
  Signal* entrySignal;
  
  public: Route(Sim s, Signal* entrySignal,
                 vector<pair<Switch*, SwitchState>> swPos,
                 vector<TVD*> tvds) : EnvObj(s), switchPositions(swPos),
                 tvds(tvds),entrySignal(entrySignal) {}
  
  private: 

  class CatchSignal : public Process {
    Signal* sig; TVD* tvd;
    public:
    CatchSignal(Sim s, Signal* sig, TVD* tvd) : Process(s), sig(sig), tvd(tvd) {}
    virtual bool Run() {
      PT_BEGIN();
      PROC_WAIT_FOR(tvd->occupied_event);
      sig->set_green(false);
      PT_END();
    }
  };

  class ReleaseTrigger : public Process {
    TVD* tvd; vector<Resource*> resources;
    public:
    ReleaseTrigger(Sim s, TVD* tvd, vector<Resource*> resources) : Process(s), tvd(tvd), resources(resources) {}
    virtual bool Run() {
      PT_BEGIN();
      PROC_WAIT_FOR(tvd->occupied_event);
      PROC_WAIT_FOR(tvd->occupied_event);
      for(auto& r:resources) r->set_allocated(false);
      PT_END();
    }
  };

  shared_ptr<Event> open_signal() {
    this->entrySignal->set_green(true);
    return this->env->start_process<CatchSignal>(entrySignal, tvds[0]);
  }
  
};

struct Interlocking {
  vector<TVD> tvds;
  vector<Switch> switches;
};

int main() {
  auto sim = Simulation::create();
  Interlocking interlocking;
  interlocking.tvds.emplace_back(TVD(sim));
  interlocking.switches.emplace_back(Switch(sim, SwitchState::Left));
  sim->run();
  interlocking.switches[0].turn(SwitchState::Right);
  sim->run();
  return 0;
}
