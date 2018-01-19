#include "simcpp.h"
#include "simobj.h"
#include <utility>
#include <cmath>
#include <string>
#include <limits>
#include <unordered_map>
#include "il_inputspec.h"
#include "traindynamics.h"

using std::pair;
using std::make_pair;
using std::string;

template <typename T> int sgn(T val) { return (T(0) < val) - (val < T(0)); }

typedef shared_ptr<Simulation> Sim;

class Train;
class ISObj;

struct TrainLoc {
  ISObj *obj;
  double offset;
};

struct TrainRun {
  LinearTrainParams params;
  Direction startDir;
  double startAuthorityLength;
  TrainLoc startLoc;
  vector<pair<TrainLoc, double>> stops;
  TrainLoc endLoc;
};

struct Link {
  ISObj *obj = nullptr;
  double length;
};

Link mk_link(const LinkSpec &ls, const vector<unique_ptr<ISObj>> &objIndex) {
  Link l;
  l.obj = objIndex[ls.index].get();
  l.length = ls.length;
  return l;
}

class ISObj {
public:
  Link down;
  Link up;

  virtual void arrive_front(Train &t){};
  virtual void arrive_back(Train &t){};
  virtual Link *next(Direction dir) {
    if (dir == Direction::Up)
      return &this->up;
    if (dir == Direction::Down)
      return &this->down;
    return nullptr;
  }
};

class Signal : protected EnvObj, public ISObj {
public:
  Direction dir;
  Signal(Sim s, Direction dir) : EnvObj(s), dir(dir) {}
  OBSERVABLE_PROPERTY(bool, green, false)
  OBSERVABLE_PROPERTY(double, authority, 0.0)
};

class Boundary : public ISObj {};

class Stop : public ISObj {};

class Train : public Process {
  vector<pair<Signal *, double>> can_see;
  TrainRun runSpec;
  Direction dir;
  TrainLoc location;
  LinearTrainParams params;
  double velocity;
  LinearTrainSpeedRestrictions targets;
  const double LINESPEED = 36.0;
  LinearTrainStep::Action action = LinearTrainStep::Action::Coast;
  double last_t;
  double authority = 0.0;
  vector<pair<ISObj *, double>> nodesUnderTrain;

public:
  void add_sight(Signal *s, double d) {
    this->can_see.push_back(make_pair(s, d));
  }
  Direction get_dir() { return this->dir; }

  enum class TargetReason { Target, ReachNode, ClearNode, NoTrack };

  pair<TargetReason, double> node_dist() {
    auto link = this->location.obj->next(this->dir);
    if (link == nullptr)
      return {TargetReason::NoTrack, 0.0};

    double distToFrontNode = link->length - this->location.offset;

    if (this->nodesUnderTrain.size() >= 1 &&
        this->nodesUnderTrain[0].second < distToFrontNode) {
      return {TargetReason::ClearNode, this->nodesUnderTrain[0].second};
    }
    return {TargetReason::ReachNode, distToFrontNode};
  }

  Train(Sim s, TrainRun spec) : Process(s) {
    this->runSpec = spec;
    this->location = spec.startLoc;
    this->dir = spec.startDir;
    this->params = spec.params;
    this->targets.vmax = LINESPEED;
    this->authority = spec.startAuthorityLength;
    this->location.obj->arrive_front(*this); // At a boundary, nothing happens
    this->nodesUnderTrain.push_back({this->location.obj, this->params.length});
    this->last_t = s->get_now();
  }

  shared_ptr<Process> until_discrete() {
    vector<shared_ptr<Event>> evs;
    auto nodestep = this->node_dist();
    auto step =
        trainStep(this->params, nodestep.second, this->velocity, this->targets);
    this->action = step.action;

    for (auto &sig : this->can_see) {
      evs.push_back(sig.first->authority_event);
    }

    if (step.dt > 1e-10) {
      evs.push_back(this->sim->timeout(step.dt));
    }

    return this->sim->start_process<AnyOf>(evs);
  }

  void update_continuous() {
    double dt = this->sim->get_now() - this->last_t;
    if(dt >= 1e-10) {
      auto dx_v = trainUpdate(this->params, this->velocity, {this->action, dt});

      auto dx = dx_v.first;
      auto new_v = dx_v.second;

      this->velocity = new_v;
      this->location.offset += dx;

      // Distance to sighted signals
      for (auto &see : this->can_see)
        see.second -= dx;

      for (auto &n : this->nodesUnderTrain)
        n.second -= dx;
    }
  }

  void update_discrete() {
    while(this->node_dist().second < 1e-10 && this->node_dist().first != TargetReason::NoTrack) {
      auto nodestep = this->node_dist();
      if(nodestep.first == TargetReason::ReachNode) {
        auto link = this->location.obj->next(this->dir);
        if (link != nullptr) {
          this->location.obj = link->obj;
          this->location.offset -= link->length;
          this->location.obj->arrive_front(*this);
        }
      }
      if(nodestep.first == TargetReason::ClearNode) {
        auto cleared = this->nodesUnderTrain[0];
	cleared.first->arrive_back(*this);
	this->nodesUnderTrain.erase(this->nodesUnderTrain.begin());
      }
    }
  }

  void update_targets() {
    // Start with known authority
    double signal_authority = this->authority;

    // Update with signal information
    bool updated;
    do {
      updated = false;
      for (auto &s : this->can_see) {
        if (s.second - signal_authority <= 1e-10) {
          signal_authority =
              std::max(signal_authority, s.second + s.first->get_authority());
          updated = true;
        }
      }
    } while (updated);

    // Write back to train state. (remember authority even if we pass the
    // signal).
    this->authority = signal_authority;
    printf("Update train targets: authority=%g\n", this->authority);
    this->targets.ahead.clear();
    this->targets.ahead.push_back({this->authority, 0.0});
  }

  bool Run() override {
    PT_BEGIN();
    while (true) {
      this->update_targets();
      PROC_WAIT_FOR(this->until_discrete());
      this->update_continuous();
      this->update_discrete();
    }
    PT_END();
  }
};

class Sight : public ISObj {
  Signal *sig;
  double dist;

public:
  Sight(Signal *sig, double dist) : sig(sig), dist(dist) {}
  void arrive_front(Train &t) override {
    if (t.get_dir() == this->sig->dir)
      t.add_sight(this->sig, this->dist);
  }
};

class Resource : protected EnvObj {
  OBSERVABLE_PROPERTY(bool, allocated, false)
public:
  Resource(Sim s) : EnvObj(s) {}
};

class TVD : public Resource, public ISObj {
  OBSERVABLE_PROPERTY(bool, occupied, false)
public:
  TVD(Sim s) : Resource(s) {}
};

class Detector : public ISObj {
public:
  shared_ptr<Event> touched_event;
  TVD *upTVD;
  TVD *downTVD;
  Sim s;
  Detector(Sim s) :s(s) {
    touched_event = std::make_shared<Event>(s);
  }

  void touched() {
    this->s->schedule(this->touched_event);
    this->touched_event = std::make_shared<Event>(s);
  }

  void arrive_front(Train &t) override {
    if (upTVD != nullptr && t.get_dir() == Direction::Up)
      upTVD->set_occupied(true);
    if (downTVD != nullptr && t.get_dir() == Direction::Down)
      downTVD->set_occupied(true);
    this->touched();
  }
  void arrive_back(Train &t) override {
    if (upTVD != nullptr && t.get_dir() == Direction::Up)
      upTVD->set_occupied(false);
    if (downTVD != nullptr && t.get_dir() == Direction::Down)
      downTVD->set_occupied(false);
    this->touched();
  }
};

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
  void set_next(const ISObjSpec &spec, const vector<unique_ptr<ISObj>> &objs) {
    if (spec.n_up == 2) {
      this->left = mk_link(spec.up[0], objs);
      this->right = mk_link(spec.up[1], objs);
      this->entry = mk_link(spec.down[0], objs);
      this->splitDir = Direction::Up;
    }
    if (spec.n_down == 2) {
      this->left = mk_link(spec.down[0], objs);
      this->right = mk_link(spec.down[1], objs);
      this->entry = mk_link(spec.up[0], objs);
      this->splitDir = Direction::Down;
    }
  }

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

typedef pair<TVD *, vector<Resource *>> ReleaseDef;
class Route : protected EnvObj {
  vector<pair<Switch *, SwitchState>> switchPositions;
  vector<TVD *> tvds;
  Signal *entrySignal;
  vector<ReleaseDef> releases;
  double length;

public:
  Route(Sim s, Signal *entrySignal, vector<pair<Switch *, SwitchState>> swPos,
        vector<TVD *> tvds, vector<ReleaseDef> releases, double length)
      : EnvObj(s), switchPositions(swPos), tvds(tvds), entrySignal(entrySignal),
        releases(releases), length(length) {}

  shared_ptr<Process> activate() {
    printf("route::activate\n");
    return this->env->start_process<RouteActivation>(this);
  }

private:
  bool resources_available() {
    bool available = true;
    for (auto &t : this->tvds)
      available &= !t->get_allocated();
    for (auto &s : this->switchPositions)
      available &= !s.first->get_allocated();
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
      for (auto &t : this->route->releases) {
        this->sim->start_process<ReleaseTrigger>(t.first, t.second);
      }
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
      sig->set_authority(0.0);
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
    this->entrySignal->set_authority(this->length);
    return this->env->start_process<CatchSignal>(entrySignal, tvds[0]);
  }
};

class World {
public:
  // Infrastructure
  vector<unique_ptr<ISObj>> objects;

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
  vector<unique_ptr<ISObj>> graphObjs;
  for (auto &spec : is.driveGraph) {
    if (spec.type == ISObjSpec::ISObjType::Signal) {
      graphObjs.emplace_back(new Signal(sim, spec.signal.dir));
    } else if (spec.type == ISObjSpec::ISObjType::Detector) {
      graphObjs.emplace_back(new Detector());
    } else if (spec.type == ISObjSpec::ISObjType::Sight) {
      graphObjs.emplace_back(
          new Sight((Signal *)graphObjs[spec.sight.signal_index].get(),
                    spec.sight.distance));
    } else if (spec.type == ISObjSpec::ISObjType::Switch) {
      graphObjs.emplace_back(new Switch(sim, spec.sw.default_state));
    } else if (spec.type == ISObjSpec::ISObjType::Boundary) {
      graphObjs.emplace_back(new Boundary());
    } else if (spec.type == ISObjSpec::ISObjType::Stop) {
      graphObjs.emplace_back(new Stop());
    } else if (spec.type == ISObjSpec::ISObjType::TVD) {
      graphObjs.emplace_back(new TVD(sim));
    } else {
      throw;
    }
  }

  for (size_t i = 0; i < is.driveGraph.size(); i++) {
    auto &spec = is.driveGraph[i];
    if (spec.n_up >= 1)
      graphObjs[i]->up = mk_link(spec.up[0], graphObjs);
    if (spec.n_down >= 1)
      graphObjs[i]->down = mk_link(spec.down[0], graphObjs);

    // Special case for switches, which have three (or more) connections.
    if (spec.type == ISObjSpec::ISObjType::Switch) {
      ((Switch *)graphObjs[i].get())->set_next(spec, graphObjs);
    }
    if (spec.type == ISObjSpec::ISObjType::Detector) {
      Detector* d = (Detector*)graphObjs[i].get();
      if(spec.detector.upTVD >= 0) d->upTVD = ((TVD*)graphObjs[spec.detector.upTVD].get());
      if(spec.detector.downTVD >= 0) d->downTVD = ((TVD*)graphObjs[spec.detector.downTVD].get());
    }
  }

  world.objects = std::move(graphObjs);
}

void mk_routes(Sim sim, World &world, const InfrastructureSpec &is) {
  for (auto &route : is.routes) {
    vector<pair<TVD *, vector<Resource *>>> releases;
    for (auto &spec : route.releases) {
      vector<Resource *> res;
      for (auto &ridx : spec.resources)
        res.push_back((Resource *)world.objects[ridx].get());
      releases.push_back(
          make_pair((TVD *)world.objects[spec.trigger].get(), std::move(res)));
    }
    Signal *entrySignal = nullptr;
    if (route.entry_signal >= 0) {
      entrySignal = (Signal *)world.objects[route.entry_signal].get();
    }
    vector<pair<Switch *, SwitchState>> swPos;
    for (auto &sw : route.switches) {
      swPos.push_back(
          make_pair((Switch *)world.objects[sw.first].get(), sw.second));
    }
    vector<TVD *> tvds;
    for (auto &tvd : route.tvds) {
      tvds.push_back((TVD *)world.objects[tvd].get());
    }
    world.routes.emplace_back(
        Route(sim, entrySignal, swPos, tvds, releases, route.length));
  }
}

World create_world(Sim sim, const InfrastructureSpec &is) {
  World world;
  mk_infrastructure(sim, world, is);
  mk_routes(sim, world, is);
  return world;
}

void test_plan(const InfrastructureSpec &is, const Plan &p) {
  auto sim = Simulation::create();
  vector<shared_ptr<Process>> trains;
  World world = create_world(sim, is);
  for (auto &i : p.activations) {
    if (i.first == Plan::PlanItemType::Route) {
      sim->advance_to(world.routes[i.second].activate());
    } else if (i.first == Plan::PlanItemType::Train) {
      // TODO convert from TrainRunSpec to TrainRun
      // auto proc = sim->start_process<Train>(p.trains[i.second]);
      // trains.push_back(proc);
    }
  }
  printf("Starting run\n");
  sim->run();
  // TODO return whether all *Train*s terminate successfully.
}

//int main() {
//  auto sim = Simulation::create();
//  sim->run();
//  return 0;
//}
