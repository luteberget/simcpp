#include "builder.h"

extern "C" {

  InfrastructureSpec* new_infrastructurespec(void) { auto x = new InfrastructureSpec(); printf("new %p\n",x);return x; }
  void free_infrastructurespec(InfrastructureSpec* is) { printf("delete %p\n",is);delete is; }

  ReleaseSpec* new_release(size_t trigger) { auto r = new ReleaseSpec(); 
	  r->trigger = trigger;
	  return r;
    }

Plan* new_plan(void) {
	return new Plan;
}
TrainRunSpec* new_trainrunspec(double acc, double brk, double vmax, double len,
		int dir, double authLen, size_t startObj) {
	TrainRunSpec* s = new TrainRunSpec;
	s->params.max_acc = acc;
	s->params.max_brk = brk;
	s->params.max_vel = vmax;
	s->params.length = len;
	s->startDir = dir > 0 ? Direction::Up : Direction::Down;
	s->startAuthorityLength = authLen;
	s->startLoc = TrainLocSpec { startObj, 0.0 };
	return s;
}
void add_trainrunspec_stop(TrainRunSpec* s, double stop) {
	s->stops.push_back(stop);
}
void add_plan_train(Plan* p, TrainRunSpec* s) {
	p->trains.push_back(*s);
	delete s;
	p->activations.push_back({Plan::PlanItemType::Train, p->trains.size() -1});
}

void add_plan_route(Plan* p, size_t r) {
	p->activations.push_back({Plan::PlanItemType::Route, r});
}
double run_plan(InfrastructureSpec* is, Plan* p) {
	test_plan(*is,*p);
	return 90.5;
}
void free_plan(Plan* p) { delete p; }

  void add_release_resource(ReleaseSpec* r, size_t resource) { r->resources.push_back(resource); }

  RouteSpec* new_route(const char* name, 
		  int entry_signal, 
		  double length) { 
	  auto r = new RouteSpec(); 
	  if(name != nullptr) r->name = string(name);
	  r->entry_signal = entry_signal;
	  r->length = length;
	  return r;
  }
  
  void add_release(RouteSpec* route, ReleaseSpec* release) {
    route->releases.push_back(*release);
    delete release;
  }

  void add_route(InfrastructureSpec* is, RouteSpec* r) {
    // printf("C++: ADD ROUTE\n");
    is->routes.push_back(*r);
    printf("routes %lu\n", is->routes.size());
    delete r;
  }

  void add_route_switch(RouteSpec* r, size_t sw, int state) {
     auto s = state > 0 ? SwitchState::Right : SwitchState::Left;
     r->switches.push_back({sw,s});
  }

  void add_route_tvd(RouteSpec* r, size_t tvd) { r->tvds.push_back(tvd); }


  ISObjSpec mkobj( const char* name, int upIdx, double upLength, int downIdx, double downLength) {
    ISObjSpec spec;
    spec.name = name;
    spec.n_up = 0;
    spec.n_down = 0;
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
      int upDir) {
    ISObjSpec spec = mkobj(name,upIdx,upLength,downIdx,downLength);
    spec.type = ISObjSpec::ISObjType::Signal;
    spec.signal.dir = upDir > 0 ? Direction::Up : Direction::Down;
    is->driveGraph.push_back(spec);
    printf("Added signal at [%lu]\n",is->driveGraph.size()-1);
    printf("IS %p\n",is);
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
