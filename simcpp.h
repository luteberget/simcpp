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

    QueuedEvent(double time, size_t id, shared_ptr<Event> event);

    bool operator<(const QueuedEvent &other) const;
  };

  static shared_ptr<Simulation> create();

  template <class T, class... Args>
  shared_ptr<Process> start_process(Args &&...args) {
    return run_process(std::make_shared<T>(shared_from_this(), args...));
  }

  shared_ptr<Process> run_process(shared_ptr<Process> process);

  shared_ptr<Event> timeout(double delay);

  shared_ptr<Event> schedule(shared_ptr<Event> event, double delay = 0.0);

  bool step();

  void advance_by(double duration);

  void advance_to(shared_ptr<Process> process);

  void run();

  double get_now();

private:
  double now = 0.0;
  size_t next_id = 0;
  priority_queue<QueuedEvent> queued_events;

  bool has_next();

  double peek_next_time();
};

class Event {
private:
  unique_ptr<vector<shared_ptr<Process>>> listeners;

protected:
  int value = -1;
  shared_ptr<Simulation> sim;

public:
  Event(shared_ptr<Simulation> sim);

  bool add_handler(shared_ptr<Process> process);

  bool is_processed();

  int get_value();

  void fire();

  bool is_triggered();

  bool is_success();

  bool is_failed();
};

class Process : public Event,
                public Protothread,
                public std::enable_shared_from_this<Process> {
public:
  Process(shared_ptr<Simulation> sim);

  void resume();

  void abort();

  virtual void Aborted();
};

class AnyOf : public Process {
public:
  AnyOf(shared_ptr<Simulation> sim, vector<shared_ptr<Event>> events);

  virtual bool Run() override;
};

class AllOf : public Process {
  size_t i = 0;
  vector<shared_ptr<Event>> events;

public:
  AllOf(shared_ptr<Simulation> sim, vector<shared_ptr<Event>> events);

  virtual bool Run() override;
};

#endif
