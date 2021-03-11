#ifndef __SIMCPP_H
#define __SIMCPP_H

#include "protothread.h"
#include <memory>
#include <queue>
#include <vector>

#define PROC_WAIT_FOR(event)                                                   \
  do {                                                                         \
    if ((event)->add_handler(shared_from_this())) {                            \
      PT_YIELD();                                                              \
    }                                                                          \
  } while (0)

using std::priority_queue;
using std::shared_ptr;
using std::unique_ptr;
using std::vector;

class Event;
class Process;
class Simulation : public std::enable_shared_from_this<Simulation> {

public:
  class QueuedEvent {
  public:
    double time;
    size_t id;
    shared_ptr<Event> event;
    QueuedEvent(double t, size_t id, shared_ptr<Event> e)
        : time(t), id(id), event(e) {}

    bool operator<(const QueuedEvent &b) const {
      if (time != b.time)
        return time > b.time;
      return id > b.id;
    }
  };

  static shared_ptr<Simulation> create() {
    return std::make_shared<Simulation>();
  }
  shared_ptr<Process> run_process(shared_ptr<Process> p);

  template <class T, class... Args>
  shared_ptr<Process> start_process(Args &&...args) {
    return run_process(std::make_shared<T>(shared_from_this(), args...));
  }

  shared_ptr<Event> timeout(double delay);

private:
  double now = 0.0;
  priority_queue<QueuedEvent> events;
  size_t id_counter = 0;
  bool has_next() { return !events.empty(); }
  double peek_next_time() { return events.top().time; }

public:
  shared_ptr<Event> schedule(shared_ptr<Event> e, double delay = 0.0);
  bool step();

  void advance_by(double t) {
    double target = now + t;
    while (has_next() && peek_next_time() <= target) {
      step();
    }
    now = target;
  }

  void advance_to(shared_ptr<Process> p);

  void run() {
    while (step())
      ;
  }
  double get_now() { return now; }
};

class Event {
private:
  unique_ptr<vector<shared_ptr<Process>>> listeners;

protected:
  shared_ptr<Simulation> sim;
  int value = -1;

public:
  Event(shared_ptr<Simulation> sim)
      : listeners(new vector<shared_ptr<Process>>()), sim(sim) {}

  bool add_handler(shared_ptr<Process> p) {
    if (is_processed()) {
      return false;
    }

    listeners->push_back(p);
    return true;
  }

  bool is_processed() { return listeners == nullptr; }
  int get_value() { return value; }
  void fire();

  bool is_triggered() { return value != -1; }
  bool is_success() { return value == 1; }
  bool is_failed() { return value == 2; }
};

class Process : public Event,
                public Protothread,
                public std::enable_shared_from_this<Process> {
public:
  Process(shared_ptr<Simulation> sim) : Event(sim), Protothread() {}
  void resume();
  void abort();
  virtual void Aborted(){};
};

void Process::abort() {
  value = 2; //?
  Aborted();
}

void Process::resume() {
  // Is the process already finished?
  if (is_triggered())
    return;

  bool run = Run();

  // Did the process finish now?
  if (!run) {
    // Process finished
    value = 1;
    sim->schedule(shared_from_this());
  }
}

shared_ptr<Event> Simulation::schedule(shared_ptr<Event> e, double delay /* = 0 */) {
  events.emplace(now + delay, id_counter++, e);
  return e;
}

bool Simulation::step() {
  if (events.empty()) {
    return false;
  }

  auto queuedEvent = events.top();
  events.pop();
  now = queuedEvent.time;
  auto event = queuedEvent.event;
  // TODO Does this keep the shared pointer in scope, making the
  // automatic memory management useless?
  // TODO Is this whole approach limited by the stack size?
  event->fire();
  return true;
}

void Event::fire() {
  auto listeners = std::move(this->listeners);

  this->listeners = nullptr;
  value = 0;

  for (auto proc : *listeners) {
    proc->resume();
  }
}

shared_ptr<Process> Simulation::run_process(shared_ptr<Process> p) {
  auto ev = std::make_shared<Event>(shared_from_this());
  ev->add_handler(p);
  schedule(ev);
  return p;
}

shared_ptr<Event> Simulation::timeout(double delay) {
  auto ev = std::make_shared<Event>(shared_from_this());
  schedule(ev, delay);
  return ev;
}

class AnyOf : public Process {
public:
  AnyOf(shared_ptr<Simulation> sim, vector<shared_ptr<Event>> v)
      : Process(sim) {
    for (auto &e : v)
      e->add_handler(shared_from_this());
  }
  virtual bool Run() override {
    PT_BEGIN();
    PT_YIELD();
    // Any time we get called back, we are finished.
    PT_END();
  }
};
;

// ALLOF event
class AllOf : public Process {
  vector<shared_ptr<Event>> v;
  size_t i = 0;

public:
  AllOf(shared_ptr<Simulation> sim, vector<shared_ptr<Event>> v)
      : Process(sim), v(v) {}
  virtual bool Run() override {
    PT_BEGIN();
    while (i < v.size()) {
      if (!v[i]->is_triggered()) {
        PROC_WAIT_FOR(v[i]);
        i++;
      }
    }
    PT_END();
  }
};

void Simulation::advance_to(shared_ptr<Process> p) {
  while (!p->is_triggered() && has_next()) {
    step();
  }
}

#endif
