// Copyright © 2021 Bjørnar Steinnes Luteberget
// Licensed under the MIT license. See the LICENSE file for details.

#ifndef SIMCPP_H_
#define SIMCPP_H_

#include <functional>
#include <memory>
#include <queue>
#include <vector>

#include "protothread.h"

/**
 *
 */
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

/// Simulation environment.
class Simulation : public std::enable_shared_from_this<Simulation> {
public:
  /**
   * Create a simulation environment.
   *
   * @return Simulation instance.
   */
  static SimulationPtr create();

  /**
   * Construct a process and run it immediately.
   *
   * @tparam T Process class. Must be a subclass of Process.
   * @tparam Args Additional argument types of the constructor of T.
   * @param args Additional arguments for the construction of T.
   * @return Process instance.
   */
  template <typename T, typename... Args>
  std::shared_ptr<T> start_process(Args &&...args) {
    auto process = std::make_shared<T>(shared_from_this(), args...);
    run_process(process);
    return process;
  }

  /**
   * Construct a process and run it after a delay.
   *
   * @tparam T Process class. Must be a subclass of Process.
   * @tparam Args Additional argument types of the constructor of T.
   * @param delay Delay after which to run the process.
   * @param args Additional arguments for the construction of T.
   * @return Process instance.
   */
  template <typename T, typename... Args>
  std::shared_ptr<T> start_process_delayed(simtime delay, Args &&...args) {
    auto process = std::make_shared<T>(shared_from_this(), args...);
    run_process(process, delay);
    return process;
  }

  /**
   * Run a process after a delay.
   *
   * @param process Process to be run.
   * @param delay Delay after which to run the process.
   */
  void run_process(ProcessPtr process, simtime delay = 0.0);

  /**
   * Construct an event.
   *
   * @tparam T Event class. Must be Event or a subclass thereof.
   * @tparam Args Additional argument types of the constructor.
   * @param args Additional arguments for the construction of T.
   * @return Event instance.
   */
  template <typename T = Event, typename... Args>
  std::shared_ptr<T> event(Args &&...args) {
    return std::make_shared<T>(shared_from_this(), args...);
  }

  /**
   * Construct an event and schedule it to be processed  after a delay.
   *
   * @param delay Delay after which the event is processed.
   * @return Event instance.
   */
  EventPtr timeout(simtime delay);

  /**
   * Create an "any of" event.
   *
   * An "any of" event is the union of multiple events. It is triggered when
   * any of the underlying events is processed.
   *
   * @param events Underlying events.
   * @return Event instance.
   */
  EventPtr any_of(std::initializer_list<EventPtr> events);

  /**
   * Create an "all of" event.
   *
   * An "all of" event is the conjunction of multiple events. It is triggered
   * when all of the underlying events are processed.
   *
   * @return Event instance.
   */
  EventPtr all_of(std::initializer_list<EventPtr> events);

  /**
   * Schedule an event to be processed after a delay.
   *
   * @param event Event instance.
   * @param delay Delay after which the event is processed.
   */
  void schedule(EventPtr event, simtime delay = 0.0);

  /**
   * Process the next scheduled event.
   *
   * @return Whether there was a scheduled event to be processed.
   */
  bool step();

  /**
   * Advance the simulation by a duration.
   *
   * @param duration Duration to advance the simulation by.
   */
  void advance_by(simtime duration);

  /**
   * Advance the simulation until an event is triggered.
   *
   * @param event Event to wait for.
   * @return Whether the event was triggered. If no scheduled events are left or
   * the event is aborted, this is not the case.
   */
  bool advance_to(EventPtr event);

  /// Run the simulation until no scheduled events are left.
  void run();

  /// @return Current simulation time.
  simtime get_now();

  /// @return Whether a scheduled event is left.
  bool has_next();

  /// @return Time at which the next event is scheduled.
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

/**
 * Event in a simulation.
 *
 * This class can be subclassed to create custom events with special attributes
 * or methods.
 */
class Event : public std::enable_shared_from_this<Event> {
public:
  /// State of an event.
  enum class State {
    /// Event has not been triggered or aborted.
    Pending,
    /// Event has been triggered.
    Triggered,
    /// Event has been triggered and processed.
    Processed,
    /// Event has been aborted.
    Aborted
  };

  /**
   * Construct an event.
   *
   * @param sim Simulation instance.
   */
  explicit Event(SimulationPtr sim);

  /**
   * Add the resume method of a process as an handler of the event.
   *
   * If the event is already triggered or aborted, nothing is done. If the event
   * is triggered, the process should not wait to be resumed. The return value
   * can be used to check this.
   *
   * @param process The process to resume when the event is processed.
   * @return Whether the event was not already triggered.
   */
  bool add_handler(ProcessPtr process);

  /**
   * Add the callback as an handler of the event.
   *
   * If the event is already triggered or aborted, nothing is done.
   *
   * @param handler Callback to call when the event is processed. The callback
   * receives the event instance as an argument.
   * @return Whether the event was not already triggered.
   */
  bool add_handler(Handler handler);

  /**
   * Trigger the event with a delay.
   *
   * The event is scheduled to be processed after the delay.
   *
   * @param delay Delay after with the event is processed.
   * @return Whether the event was not already triggered or aborted.
   */
  bool trigger(simtime delay = 0.0);

  /**
   * Abort the event.
   *
   * The Aborted callback is called.
   *
   * @return Whether the event was not already triggered or aborted.
   */
  bool abort();

  /**
   * Process the event.
   *
   * All handlers of the event are called when the event is not already
   * processed or aborted.
   */
  void process();

  /// @return Whether the event is pending.
  bool is_pending();

  /**
   * @return Whether the event is triggered. Also true if the event is
   * processed.
   */
  bool is_triggered();

  /// @return Whether the event is aborted.
  bool is_aborted();

  /// @return Whether the event is processed.
  bool is_processed();

  /// @return Whether the event is pending.
  State get_state();

  /// Called when the event is aborted.
  virtual void Aborted();

protected:
  /**
   * Weak pointer to the simulation instance.
   *
   * Used in subclasses of Event to access the simulation instance. To convert
   * it to a shared pointer, use sim.lock().
   */
  SimulationWeakPtr sim;

private:
  State state = State::Pending;
  std::vector<Handler> handlers = {};
};

/// Process in a simulation.
class Process : public Event, public Protothread {
public:
  /**
   * Construct a process.
   *
   * @param sim Simulation instance.
   */
  explicit Process(SimulationPtr sim);

  /**
   * Resumes the process.
   *
   * If the event is triggered or aborted, nothing is done. If the process
   * finishes, the event is triggered.
   */
  void resume();

  /// @return Shared pointer to the process instance.
  ProcessPtr shared_from_this();
};

/// Condition process used for Simulation::any_of and Simulation::all_of.
class Condition : public Process {
public:
  /**
   * Constructs a condition.
   *
   * @param sim Simulation instance.
   * @param n Number of times the process must be resumed before it finishes.
   */
  Condition(SimulationPtr sim, int n);

  /**
   * Wait for the given number of resumes and then finish.
   *
   * @return Whether the process is still running.
   */
  bool Run() override;

private:
  int n;
};

} // namespace simcpp

#endif // SIMCPP_H_
