#include "simcpp.h"


void Process::abort()
{
  this->value = 2; //?
  Aborted();
}

void Process::resume()
{
  // Is the process already finished?
  if (this->is_triggered())
    return;

  bool run = Run();

  // Did the process finish now?
  if (!run)
  {
    // Process finished
    this->value = 1;
    this->sim->schedule(shared_from_this());
  }
}


shared_ptr<Event> Simulation::schedule(shared_ptr<Event> e, double delay)
{
  this->events.emplace(QueuedEvent(this->now + delay, id_counter++, e));
  return e;
}

bool Simulation::step()
{
  if (this->events.empty())
  {
    return false;
  }

  auto queuedEvent = this->events.top();
  this->events.pop();
  // printf("Stepping from \t%g to \t%g\n",this->now,queuedEvent.time);
  this->now = queuedEvent.time;
  auto event = queuedEvent.event;
  // TODO Does this keep the shared pointer in scope, making the
  // automatic memory management useless?
  // TODO Is this whole approach limited by the stack size?
  event->fire();
  return true;
}

void Simulation::advance_to(shared_ptr<Process> p)
{
  while (!p->is_triggered() && this->has_next())
  {
    this->step();
  }
}


void Event::fire()
{
  auto listeners = std::move(this->listeners);
  this->listeners = nullptr;
  for (auto proc : *listeners)
  {
    proc->resume();
  }
}

shared_ptr<Process> Simulation::run_process(shared_ptr<Process> p)
{
  p->resume();
  return p;
}

shared_ptr<Event> Simulation::timeout(double delay)
{
  auto ev = std::make_shared<Event>(shared_from_this());
  this->schedule(ev, delay);
  return ev;
}

