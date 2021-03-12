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

namespace simcpp {

class Event;
using EventPtr = std::shared_ptr<Event>;

class Process;
using ProcessPtr = std::shared_ptr<Process>;

class Simulation;
using SimulationPtr = std::shared_ptr<Simulation>;

class Simulation : public std::enable_shared_from_this<Simulation> {
public:
  static SimulationPtr create();

  template <class T, class... Args> ProcessPtr start_process(Args &&...args) {
    return run_process(std::make_shared<T>(shared_from_this(), args...));
  }

  ProcessPtr run_process(ProcessPtr process);

  EventPtr event();

  EventPtr timeout(double delay);

  EventPtr any_of(std::initializer_list<EventPtr> events);

  EventPtr all_of(std::initializer_list<EventPtr> events);

  EventPtr schedule(EventPtr event, double delay = 0.0);

  bool step();

  void advance_by(double duration);

  void advance_to(EventPtr event);

  void run();

  double get_now();

  bool has_next();

  double peek_next_time();

private:
  class QueuedEvent {
  public:
    double time;
    size_t id;
    EventPtr event;

    QueuedEvent(double time, size_t id, EventPtr event);

    bool operator<(const QueuedEvent &other) const;
  };

  double now = 0.0;
  size_t next_id = 0;
  std::priority_queue<QueuedEvent> queued_events;
};

class Event : public std::enable_shared_from_this<Event> {
public:
  enum class State { Pending, Triggered, Aborted };

  Event(SimulationPtr sim);

  bool add_handler(ProcessPtr process);

  void trigger();

  void abort();

  void fire();

  bool is_pending();

  bool is_triggered();

  bool is_aborted();

  bool is_processed();

  State get_state();

  virtual void Aborted();

protected:
  SimulationPtr sim;

private:
  State state = State::Pending;
  std::unique_ptr<std::vector<ProcessPtr>> listeners;
};

class Process : public Event, public Protothread {
public:
  Process(SimulationPtr sim);

  void resume();

  ProcessPtr shared_from_this();
};

class Condition : public Process {
public:
  Condition(SimulationPtr sim, int n);

  virtual bool Run() override;

private:
  int n;
};

} // namespace simcpp

#endif
