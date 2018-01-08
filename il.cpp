#include "simcpp.h"
#include "simobj.h"
#include <utility>
#include <cmath>
#include <string>
#include <unordered_map>

using std::pair;
using std::make_pair;
using std::string;

template <typename T> int sgn(T val) { return (T(0) < val) - (val < T(0)); }

typedef shared_ptr<Simulation> Sim;

struct TrainRunSpec {};
enum class Direction { Up, Down };
class Train;
class InfrastructureObject;
struct Link {
  InfrastructureObject* obj = nullptr;
  double length;
};
class InfrastructureObject {
public:
  Link down;
  Link up;

  virtual void arrive_front(Train &t){};
  virtual void arrive_back(Train &t){};
  virtual Link* next(Direction dir) {
    if (dir == Direction::Up)
      return &this->up;
    if (dir == Direction::Down)
      return &this->down;
    return nullptr;
  }
};

class Signal : protected EnvObj, public InfrastructureObject {
public:
  Direction dir;
  Signal(Sim s, Direction dir) : EnvObj(s),dir(dir) {}
  OBSERVABLE_PROPERTY(bool, green, false)
};

class Boundary : public InfrastructureObject {
};

class Stop : public InfrastructureObject {
};

class Train : public Process {
  vector<Signal *> can_see;
  Direction dir = Direction::Up;

public:
  void add_sight(Signal* s) { can_see.push_back(s); }
  Direction get_dir() { return this->dir; }
  Train(Sim s, TrainRunSpec spec) : Process(s) {}
  bool Run() override {
    PT_BEGIN();
    PT_END();
  }
};


class Sight : public InfrastructureObject{
  public:
    Signal* sig;
    Sight(Signal* sig) :sig(sig) {}
    void arrive_front(Train& t) override { 
      if(t.get_dir() == sig->dir) t.add_sight(sig);
    }
    
};

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

class Detector : public InfrastructureObject {
public:
  TVD *upTVD;
  TVD *downTVD;

  void arrive_front(Train &t) override {
    if (upTVD != nullptr && t.get_dir() == Direction::Up)
      upTVD->set_occupied(true);
    if (downTVD != nullptr && t.get_dir() == Direction::Down)
      downTVD->set_occupied(true);
  }
  void arrive_back(Train &t) override {
    if (upTVD != nullptr && t.get_dir() == Direction::Up)
      upTVD->set_occupied(false);
    if (downTVD != nullptr && t.get_dir() == Direction::Down)
      downTVD->set_occupied(false);
  }
};

enum class SwitchState { Left, Right, Unknown };

class Switch : public Resource, public InfrastructureObject {
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

class Route : protected EnvObj {
  vector<pair<Switch *, SwitchState>> switchPositions;
  vector<TVD *> tvds;
  Signal *entrySignal;

public:
  Route(Sim s, Signal *entrySignal, vector<pair<Switch *, SwitchState>> swPos,
        vector<TVD *> tvds)
      : EnvObj(s), switchPositions(swPos), tvds(tvds),
        entrySignal(entrySignal) {}

  shared_ptr<Process> activate() {
    return this->env->start_process<RouteActivation>(this);
  }

private:
  bool resources_available() {
    bool available = true;
    for (auto &t : this->tvds)
      available &= t->get_allocated();
    for (auto &s : this->switchPositions)
      available &= s.first->get_allocated();
    return available;
  }

  bool switches_correct() {
    bool correct = true;
    for (auto &s : this->switchPositions)
      correct &= s.first->get_state() == s.second;
    return correct;
  }

  class RouteActivation : public Process {
    Route *route;
    void reserve_resources() {
      for (auto &t : this->route->tvds)
        t->set_allocated(true);
      for (auto &s : this->route->switchPositions)
        s.first->set_allocated(true);
    }

    shared_ptr<Process> turn_switches() {
      vector<shared_ptr<Event>> all;
      for (auto &s : this->route->switchPositions)
        all.push_back(s.first->turn(s.second));
      return this->sim->start_process<AllOf>(all);
    }

    shared_ptr<Process> wait_busy_resources() {
      vector<shared_ptr<Event>> all;
      for (auto &t : this->route->tvds) {
        if (t->get_allocated())
          all.push_back(t->allocated_event);
      }
      for (auto &s : this->route->switchPositions) {
        if (s.first->get_allocated())
          all.push_back(s.first->allocated_event);
      }
      return this->sim->start_process<AllOf>(all);
    }

  public:
    RouteActivation(Sim s, Route *r) : Process(s), route(r) {}

    virtual bool Run() {
      PT_BEGIN();
      while (!this->route->resources_available()) {
        PROC_WAIT_FOR(wait_busy_resources());
      }
      reserve_resources();
      while (!this->route->switches_correct()) {
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

struct PosRef {
  string track;
  double pos;
};

struct TrackSpec {
  // name
  double length;
};

struct SwitchSpec {
  SwitchState default_state = SwitchState::Left;
};

struct SignalSpec {
  Direction dir;
};

struct DetectorSpec {
  string upTVD, downTVD;
};

struct SightSpec {
  size_t signal_index;
};

struct LinkSpec {
  size_t index;
  double length;
};

struct ISObjSpec {
  enum class ISObjType { Signal, Detector, Sight, Switch, Boundary, Stop };
  const char* name;
  ISObjType type;
  short n_up; short n_down;
  LinkSpec up[2];
  LinkSpec down[2];
  union {
    SignalSpec signal;
    DetectorSpec detector;
    SightSpec sight;
    SwitchSpec sw;
  };
};

struct InfrastructureSpec {
  vector<ISObjSpec> driveGraph;
};

struct ReleaseSpec {}; // TODO

struct RouteSpec {
  const char* name;
  int entry_signal;
  vector<int> tvds;
  vector<pair<int,SwitchState>> switches;
  vector<ReleaseSpec> releases;
};

struct InterlockingSpec {
  vector<RouteSpec> routes;
};

class World {
public:
  // Infrastructure
  vector<TVD> tvds;
  vector<Detector> detectors;
  vector<Sight> sights;
  vector<Switch> switches;
  vector<Signal> signals;

  // Interlocking
  vector<Route> routes;

  // Trains
  vector<Train> trains;
};

struct Plan {
  enum class PlanItemType { Route, Train };
  vector<pair<PlanItemType, size_t>> activations;
  vector<TrainRunSpec> trains;
};

void mk_infrastructure(Sim sim, World &world, const InfrastructureSpec &is) {
  vector<InfrastructureObject*> graphObjs;
  std::unordered_map<string,TVD*> tvdmap;
  for(auto& spec : is.driveGraph) {
    if(spec.type == ISObjSpec::ISObjType::Signal) {
      graphObjs.push_back(new Signal(sim, spec.signal.dir));
    } else if (spec.type == ISObjSpec::ISObjType::Detector) {
      auto d = new Detector();
      if(tvdmap.find(spec.detector.upTVD) == tvdmap.end()) {
        tvdmap[spec.detector.upTVD] = new TVD(sim);
      }
      if(tvdmap.find(spec.detector.downTVD) == tvdmap.end()) {
        tvdmap[spec.detector.downTVD] = new TVD(sim);
      }
      d->upTVD = tvdmap[spec.detector.upTVD];
      d->downTVD = tvdmap[spec.detector.downTVD];
      graphObjs.push_back(d);
    } else if (spec.type == ISObjSpec::ISObjType::Sight) {
      graphObjs.push_back(new Sight((Signal*)graphObjs[spec.sight.signal_index]));
    } else if (spec.type == ISObjSpec::ISObjType::Switch) {
      graphObjs.push_back(new Switch(sim, spec.sw.default_state));
    } else if (spec.type == ISObjSpec::ISObjType::Boundary) {
      graphObjs.push_back(new Boundary());
    } else if (spec.type == ISObjSpec::ISObjType::Stop) {
      graphObjs.push_back(new Stop());
    } else {
      throw; 
    }
  }

  for(size_t i = 0; i < is.driveGraph.size(); i++) {
    auto& spec = is.driveGraph[i];
    if(spec.n_up >= 1) {
      auto& ls = spec.up[0];
      Link l;
      l.obj = graphObjs[ls.index];
      l.length = ls.length;
      graphObjs[i]->up = l;
    }
    // TODO try to have only have up-dir in spec 
    // and reverse relation here?
    if(spec.n_down >= 1) {
      auto& ls = spec.down[0];
      Link l;
      l.obj = graphObjs[ls.index];
      l.length = ls.length;
      graphObjs[i]->down = l;
    }

    if(spec.type == ISObjSpec::ISObjType::Switch) {
      printf("TODO switch\n");
    }
  }
}

void mk_routes(Sim sim, World& world, const InterlockingSpec& il) {
}

World create_world(Sim sim, const InfrastructureSpec &is,
                   const InterlockingSpec &il) {
  World world;
  mk_infrastructure(sim, world, is);
  mk_routes(sim,world,il);
  return world;
}

void test_plan(const InfrastructureSpec &is, const InterlockingSpec &il,
               const Plan &p) {
  auto sim = Simulation::create();
  vector<shared_ptr<Process>> trains;
  World world; // = create_world(sim,is,il);
  for (auto &i : p.activations) {
    if (i.first == Plan::PlanItemType::Route) {
      sim->advance_to(world.routes[i.second].activate());
    } else if (i.first == Plan::PlanItemType::Train) {
      auto proc = sim->start_process<Train>(p.trains[i.second]);
      trains.push_back(proc);
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

//
// struct InfrastructureSpec {
//   vector<TrackSpec> tracks;
//   vector<pair<vector<size_t>, vector<size_t>>> nodes;
//   vector<SignalSpec> signals;
//   vector<SightSpec> sight;
//   vector<DetectorSpec> detectors;
// };

// enum class InfrastructureObjectType { Boundary, Switch, Detector, SightPoint,
// Other /* specified stopping points */ };
// struct Position {
//   InfrastructureObjectType type;
//   InfrastructureObject* obj;
// };
//
// class Infrastructure {
//   std::unordered_map<Position,vector<Position>> graph;
//   public:
//     Position next(Direction dir, Position pos);
// };

