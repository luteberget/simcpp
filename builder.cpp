#include "il_inputspec.h"

extern "C" {

  InfrastructureSpec* new_infrastructurespec(void) { auto x = new InfrastructureSpec(); printf("new %p\n",x);return x; }
  void delete_infrastructurespec(InfrastructureSpec* is) { printf("delete %p\n",is);delete is; }


  ISObjSpec mkobj( const char* name, int upIdx, double upLength, int downIdx, double downLength) {
    ISObjSpec spec;
    spec.name = name;
    if(upIdx >= 0) {
      spec.n_up = 1;
      spec.up[0] = {(size_t)upIdx, upLength};
    }
    if(downIdx >= 0) {
      spec.n_down = 1;
      spec.down[0] = {(size_t)downIdx, downLength};
    }
    return spec;
  }

  void add_signal(InfrastructureSpec* is, const char* name,
      int upIdx, double upLength, int downIdx, double downLength,
      bool upDir) {
    ISObjSpec spec = mkobj(name,upIdx,upLength,downIdx,downLength);
    spec.type = ISObjSpec::ISObjType::Signal;
    spec.signal.dir = upDir > 0 ? Direction::Up : Direction::Down;
    is->driveGraph.push_back(spec);
    printf("Added signal at [%lu]\n",is->driveGraph.size()-1);
    printf("IS %p",is);
  }

  // DETECTOR

  void add_detector(InfrastructureSpec* is, const char* name,
		  int upIdx, double upLength, int downIdx, double downLength,
		  int upTvd, int downTvd) {
    ISObjSpec spec = mkobj(name,upIdx,upLength,downIdx,downLength);
    spec.type = ISObjSpec::ISObjType::Detector;
    spec.detector.upTVD = upTvd;
    spec.detector.downTVD = downTvd;
    is->driveGraph.push_back(spec);
  }

  // SIGHT
  void add_sight(InfrastructureSpec* is, const char* name,
		  int upIdx, double upLength, int downIdx, double downLength,
		  size_t sigref, double dist) {
    ISObjSpec spec = mkobj(name,upIdx,upLength,downIdx,downLength);
    spec.type = ISObjSpec::ISObjType::Sight;
    spec.sight.signal_index = sigref;
    spec.sight.distance = dist;
    is->driveGraph.push_back(spec);
  }
  // BOUNDARY
  void add_boundary(InfrastructureSpec* is, const char* name,
		  int upIdx, double upLength, int downIdx, double downLength) {
    ISObjSpec spec = mkobj(name,upIdx,upLength,downIdx,downLength);
    spec.type = ISObjSpec::ISObjType::Boundary;
    is->driveGraph.push_back(spec);
  }
  // STOP
  void add_stop(InfrastructureSpec* is, const char* name,
		  int upIdx, double upLength, int downIdx, double downLength) {
    ISObjSpec spec = mkobj(name,upIdx,upLength,downIdx,downLength);
    spec.type = ISObjSpec::ISObjType::Stop;
    is->driveGraph.push_back(spec);
  }
  // TVD
  void add_tvd(InfrastructureSpec* is, const char* name) {
    ISObjSpec spec = mkobj(name,-1,0,-1,0);
    spec.type = ISObjSpec::ISObjType::TVD;
    is->driveGraph.push_back(spec);
  }
  // SWITCH
  void add_switch(InfrastructureSpec* is, const char* name,
		  int upIdx1, double upLength1, 
		  int upIdx2, double upLength2, 
		  int downIdx1, double downLength1,
		  int downIdx2, double downLength2,
		  int state
		  ) {
    ISObjSpec spec;
    spec.name = name;
    if(upIdx1 >= 0) {
      spec.n_up = 1;
      spec.up[0] = {(size_t)upIdx1, upLength1};
    }
    if(upIdx2 >= 0) {
      spec.n_up = 2;
      spec.up[1] = {(size_t)upIdx2, upLength2};
    }
    if(downIdx1 >= 0) {
      spec.n_down = 1;
      spec.down[0] = {(size_t)downIdx1, downLength1};
    }
    if(downIdx2 >= 0) {
      spec.n_down = 2;
      spec.down[1] = {(size_t)downIdx2, downLength2 };
    }
    spec.sw.default_state = state > 0 ? SwitchState::Right : SwitchState::Left;
    is->driveGraph.push_back(spec);
  }
}
