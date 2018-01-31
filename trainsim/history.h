#ifndef __HISTORY_H
#define __HISTORY_H

#include "inputs.h"
using std::string;

class HistoryItem
{
public:
  enum class ItemType
  {
    RouteActivation,
    Allocation,
    SignalAspect,
    MovablePosition,
    TrainStatus,
  };
  enum class StartEnd
  {
    Start,
    End
  };
  struct RouteActivation
  {
    StartEnd status;
    const string *route;
  };
  struct Allocation
  {
    StartEnd status;
    const string *resource;
  };
  struct SignalAspect
  {
    bool green;
    const string *signal;
  };
  struct MovablePosition
  {
    SwitchState position;
    const string *resource;
  };
  struct TrainStatus
  {
    TrainAction action;
    double x;
    double v;
    const string* name;
  };

  ItemType tag;
  union {
    RouteActivation routeActivation;
    Allocation allocation;
    SignalAspect signalAspect;
    MovablePosition movablePosition;
    TrainStatus trainStatus;
  };

  static HistoryItem mkRouteActivation(StartEnd status, const string *name)
  {
    HistoryItem i;
    i.tag = ItemType::RouteActivation;
    i.routeActivation = {status, name};
    return i;
  }
  static HistoryItem mkAllocation(StartEnd status, const string *name)
  {
    HistoryItem i;
    i.tag = ItemType::Allocation;
    i.allocation = {status, name};
    return i;
  }
    static HistoryItem mkSignalAspect(bool green, const string *name)
  {
    HistoryItem i;
    i.tag = ItemType::SignalAspect;
    i.signalAspect = {green, name};
    return i;
  }
  static HistoryItem mkMovablePosition(SwitchState state, const string *name)
  {
    HistoryItem i;
    i.tag = ItemType::MovablePosition;
    i.movablePosition = {state, name};
    return i;
  }
  static HistoryItem mkTrainStatus(TrainAction action, double x, double v, const string* name) {
    HistoryItem i;
    i.tag = ItemType::TrainStatus;
    i.trainStatus = {action, x, v, name};
    return i;
  }

  
};

struct History
{
  vector<pair<double, HistoryItem>> items;
};

class OutputWriter
{
public:
  virtual void write(HistoryItem h) = 0;
};

#endif
