#ifndef SIMCPP_H_
#define SIMCPP_H_

#include <functional>
#include <memory>
#include <queue>
#include <vector>

#include "protothread.h"

#define PROC_WAIT_FOR(event)                                                   \
  do {                                                                         \
    if ((event)->add_handler(shared_from_this())) {                            \
      PT_YIELD();                                                              \
    }                                                                          \
  } while (0)

namespace simcpp {

using simtime = double;

class Event;
using EventPtr = std::shared_ptr<Event>;
using EventWeakPtr = std::weak_ptr<Event>;

class Process;
using ProcessPtr = std::shared_ptr<Process>;
using ProcessWeakPtr = std::weak_ptr<Process>;

class Simulation;
using SimulationPtr = std::shared_ptr<Simulation>;
using SimulationWeakPtr = std::weak_ptr<Simulation>;

using Handler = std::function<void(EventPtr)>;

class Simulation : public std::enable_shared_from_this<Simulation> {
public:
  static SimulationPtr create();

  template <class T, class... Args> ProcessPtr start_process(Args &&...args) {
    return run_process(std::make_shared<T>(shared_from_this(), args...));
  }

  ProcessPtr run_process(ProcessPtr process);

  EventPtr event();

  EventPtr timeout(simtime delay);

  EventPtr any_of(std::initializer_list<EventPtr> events);

  EventPtr all_of(std::initializer_list<EventPtr> events);

  void schedule(EventPtr event, simtime delay = 0.0);

  bool step();

  void advance_by(simtime duration);

  bool advance_to(EventPtr event);

  void run();

  simtime get_now();

  bool has_next();

  simtime peek_next_time();

private:
  class QueuedEvent {
  public:
    simtime time;
    size_t id;
    EventPtr event;

    QueuedEvent(simtime time, size_t id, EventPtr event);

    bool operator<(const QueuedEvent &other) const;
  };

  simtime now = 0.0;
  size_t next_id = 0;
  std::priority_queue<QueuedEvent> queued_events;
};

class Event : public std::enable_shared_from_this<Event> {
public:
  enum class State { Pending, Triggered, Processed, Aborted };

  explicit Event(SimulationPtr sim);

  bool add_handler(ProcessPtr process);
  bool add_handler(Handler handler);

  bool trigger(simtime delay = 0.0);

  bool abort();

  void process();

  bool is_pending();

  bool is_triggered();

  bool is_aborted();

  bool is_processed();

  State get_state();

  virtual void Aborted();

protected:
  SimulationWeakPtr sim;

private:
  State state = State::Pending;
  std::vector<Handler> handlers = {};
};

class Process : public Event, public Protothread {
public:
  explicit Process(SimulationPtr sim);

  void resume();

  ProcessPtr shared_from_this();
};

class Condition : public Process {
public:
  Condition(SimulationPtr sim, int n);

  bool Run() override;

private:
  int n;
};

} // namespace simcpp

#endif // SIMCPP_H_
