#ifndef __HISTORY_H
#define __HISTORY_H

#include "inputs.h"
using std::string;

struct History
{
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
  class RouteActivation
  {
    StartEnd status;
    string *route;
  };
  class Allocation
  {
    StartEnd status;
    string *resource;
  };
  class SignalAspect
  {
    string *signal;
    bool green;
  };
  class MovablePosition
  {
    string *resource;
    SwitchState position;
  };
  class TrainStatus
  {
    TrainAction action;
    double x;
    double v;
  };

  class HistoryItem
  {
    ItemType tag;
    union {
      RouteActivation routeActivation;
      Allocation allocation;
      SignalAspect signalAspect;
      MovablePosition movablePosition;
      TrainStatus trainStatus;
    };
  };

  vector<HistoryItem> items;
};

#endif