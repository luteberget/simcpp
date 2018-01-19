#ifndef __IL_INPUTSPEC_H
#define __IL_INPUTSPEC_H
#include "traindynamics.h"
#include <string>
#include <vector>

using std::string;
using std::vector;
using std::pair;
using std::make_pair;

enum class Direction { Up, Down };
enum class SwitchState { Left, Right, Unknown };

struct TrainLocSpec {
  size_t obj;
  double offset;
};

struct TrainRunSpec {
  LinearTrainParams params;
  Direction startDir;
  double startAuthorityLength;
  TrainLocSpec startLoc;
  vector<double> stops;
  // TrainLocSpec endLoc;
};

struct SwitchSpec {
  SwitchState default_state;
};

struct SignalSpec {
  Direction dir;
  size_t detector;
};

struct DetectorSpec {
  int upTVD;
  int downTVD;
};

struct SightSpec {
  size_t signal_index;
  double distance;
};

struct LinkSpec {
  size_t index;
  double length;
};

struct ISObjSpec {
  enum class ISObjType { Signal, Detector, Sight, Switch, Boundary, Stop, TVD };
  const char *name;
  ISObjType type;
  short n_up;
  short n_down;
  LinkSpec up[2];
  LinkSpec down[2];
  union {
    SignalSpec signal;
    DetectorSpec detector;
    SightSpec sight;
    SwitchSpec sw;
  };
};

struct ReleaseSpec {
  // trigger: name of TVD which triggers release
  // TODO handle more intricate triggering patterns
  size_t trigger;
  vector<size_t> resources;
};

struct RouteSpec {
  string name;
  int entry_signal;
  vector<size_t> tvds;
  vector<pair<size_t, SwitchState>> switches;
  vector<ReleaseSpec> releases;
  double length;
};

struct InfrastructureSpec {
  vector<ISObjSpec> driveGraph;
  vector<RouteSpec> routes;
};

struct Plan {
  enum class PlanItemType { Route, Train };
  vector<pair<PlanItemType, size_t>> activations;
  vector<TrainRunSpec> trains;
};

struct Schedule {
};

#endif
