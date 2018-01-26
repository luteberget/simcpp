#ifndef __IL_INPUTSPEC_H
#define __IL_INPUTSPEC_H
#include "traindynamics.h"
#include <string>
#include <vector>
#include <unordered_map>

using std::string;
using std::vector;
using std::pair;
using std::make_pair;
using std::unordered_map;

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
  size_t detector_index;
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
  string name;
  ISObjType type;
  unsigned short n_up;
  unsigned short n_down;
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
  size_t entry_signal;
  vector<size_t> tvds;
  vector<pair<size_t, SwitchState>> switches;
  vector<ReleaseSpec> releases;
  double length;
};

struct InfrastructureSpec {
  vector<ISObjSpec> driveGraph;
  unordered_map<string, RouteSpec> routes;
};

struct Plan {
  enum class ItemType { Route, Train };
  struct PlanItem {
    ItemType type;
    double dt;
    string name;
    TrainRunSpec trainData;
  };
  vector<PlanItem> items;
};

struct SimulatorInput {
  InfrastructureSpec infrastructure;
  Plan plan;
};

struct Schedule {
};

#endif
