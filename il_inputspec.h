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
  TrainLocSpec startLoc;
  vector<pair<TrainLocSpec,double>> stops;
  TrainLocSpec endLoc;
};

struct SwitchSpec {
  SwitchState default_state = SwitchState::Left;
};

struct SignalSpec {
  Direction dir;
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
  enum class ISObjType { Signal, Detector, Sight, Switch, Boundary, Stop };
  const char* name;
  ISObjType type;
  short n_up; short n_down;
  LinkSpec up[2];
  LinkSpec down[2];
  union {
    SignalSpec signal;
    DetectorSpec detector;
    SightSpec sight;
    SwitchSpec sw;
  };
};

struct InfrastructureSpec {
  vector<ISObjSpec> driveGraph;
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
  vector<pair<size_t,SwitchState>> switches;
  vector<ReleaseSpec> releases;
  double length;
};

struct InterlockingSpec {
  vector<RouteSpec> routes;
};
