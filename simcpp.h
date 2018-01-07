#ifndef __SIMCPP_H
#define __SIMCPP_H

#include "protothread.h"
#include <memory>
#include <queue>
#include <vector>

#define PROC_WAIT_FOR(event)                                                   \
  do {                                                                         \
    event->add_handler(shared_from_this());                                    \
    PT_YIELD();                                                                \
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
      if (this->time != b.time)
        return this->time < b.time;
      return this->id < b.id;
    }
  };

  static shared_ptr<Simulation> create() {
    return std::make_shared<Simulation>();
  }
  Simulation() { printf("sim starting\n"); }
  ~Simulation() { printf("sim ended\n"); }
  shared_ptr<Process> run_process(shared_ptr<Process> p);

  template< class T, class... Args > shared_ptr<Process> start_process( Args&&... args ) {
    return run_process(std::make_shared<T>(shared_from_this(), args...));
  }

  shared_ptr<Event> timeout(double delay);

private:
  double now = 0.0;
  priority_queue<QueuedEvent> events;
  size_t id_counter = 0;
  bool has_next() { return !this->events.empty(); }
  double peek_next_time() { return this->events.top().time; }

public:
  shared_ptr<Event> schedule(shared_ptr<Event> e, double delay = 0.0);
  bool step();

  void advance_by(double t) {
    double target = this->now + t;
    while (this->has_next() && this->peek_next_time() <= target) {
      this->step();
    }
  }

  void run() {
    while (this->step())
      ;
  }
  double get_now() { return this->now; }
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

  void add_handler(shared_ptr<Process> p) { listeners->push_back(p); }
  bool is_triggered() { return this->value != -1; }
  bool is_processed() { return this->listeners == nullptr; }
  int get_value() { return this->value; }
  void fire();
};

class Process : public Event,
                public Protothread,
                public std::enable_shared_from_this<Process> {
public:
  Process(shared_ptr<Simulation> sim) : Event(sim), Protothread() {}
  void resume();
};

void Process::resume() {
  if (!Run()) {
    // Process finished
    this->value = 0;
    this->sim->schedule(shared_from_this());
  }
}

shared_ptr<Event> Simulation::schedule(shared_ptr<Event> e, double delay) {
  this->events.emplace(QueuedEvent(this->now + delay, id_counter++, e));
  return e;
}

bool Simulation::step() {
  if (this->events.empty()) {
    return false;
  }

  auto queuedEvent = this->events.top();
  this->events.pop();
  // printf("Stepping from \t%g to \t%g\n",this->now,queuedEvent.time);
  this->now = queuedEvent.time;
  auto event = queuedEvent.event;
  event->fire();
  return true;
}

void Event::fire() {
  auto listeners = std::move(this->listeners);
  for (auto proc : *listeners) {
    proc->resume();
  }
}

shared_ptr<Process> Simulation::run_process(shared_ptr<Process> p) {
  p->resume();
  return p;
}

shared_ptr<Event> Simulation::timeout(double delay) {
  auto ev = std::make_shared<Event>(shared_from_this());
  this->schedule(ev, delay);
  return ev;
}

#endif
