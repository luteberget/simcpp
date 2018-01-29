#include "world.h"

#include "objects/signal.h"
#include "objects/sight.h"
#include "objects/switch.h"
#include "objects/boundary.h"
#include "objects/stop.h"
#include "objects/tvd.h"

typedef shared_ptr<Simulation> Sim;

Link mk_link(const LinkSpec &ls, const vector<unique_ptr<ISObj>> &objIndex)
{
    Link l;
    l.obj = objIndex[ls.index].get();
    l.length = ls.length;
    return l;
}

void set_switch_next(Switch &sw, const ISObjSpec &spec, const vector<unique_ptr<ISObj>> &objs)
{
    if (spec.n_up == 2)
    {
        sw.left = mk_link(spec.up[0], objs);
        sw.right = mk_link(spec.up[1], objs);
        sw.entry = mk_link(spec.down[0], objs);
        sw.splitDir = Direction::Up;
    }
    if (spec.n_down == 2)
    {
        sw.left = mk_link(spec.down[0], objs);
        sw.right = mk_link(spec.down[1], objs);
        sw.entry = mk_link(spec.up[0], objs);
        sw.splitDir = Direction::Down;
    }
}

void mk_infrastructure(Sim sim, World &world, const InfrastructureSpec &is)
{
    vector<unique_ptr<ISObj>> graphObjs;
    for (auto &spec : is.driveGraph)
    {
        if (spec.type == ISObjSpec::ISObjType::Signal)
        {
            graphObjs.emplace_back(new Signal(sim, spec.signal.dir));
        }
        else if (spec.type == ISObjSpec::ISObjType::Detector)
        {
            graphObjs.emplace_back(new Detector(sim));
        }
        else if (spec.type == ISObjSpec::ISObjType::Sight)
        {
            graphObjs.emplace_back(new Sight(spec.sight.distance));
        }
        else if (spec.type == ISObjSpec::ISObjType::Switch)
        {
            graphObjs.emplace_back(new Switch(sim, spec.sw.default_state));
        }
        else if (spec.type == ISObjSpec::ISObjType::Boundary)
        {
            graphObjs.emplace_back(new Boundary());
        }
        else if (spec.type == ISObjSpec::ISObjType::Stop)
        {
            graphObjs.emplace_back(new Stop());
        }
        else if (spec.type == ISObjSpec::ISObjType::TVD)
        {
            graphObjs.emplace_back(new TVD(sim));
        }
        else
        {
            throw;
        }
    }

    for (size_t i = 0; i < is.driveGraph.size(); i++)
    {
        auto &spec = is.driveGraph[i];
        if (spec.n_up >= 1)
            graphObjs[i]->up = mk_link(spec.up[0], graphObjs);
        if (spec.n_down >= 1)
            graphObjs[i]->down = mk_link(spec.down[0], graphObjs);

        // Special case for switches, which have three (or more) connections.
        if (spec.type == ISObjSpec::ISObjType::Switch)
        {
            set_switch_next(*(Switch *)graphObjs[i].get(), spec, graphObjs);
        }
        if (spec.type == ISObjSpec::ISObjType::Detector)
        {
            Detector *d = (Detector *)graphObjs[i].get();
            if (spec.detector.upTVD >= 0)
                d->upTVD = ((TVD *)graphObjs[spec.detector.upTVD].get());
            if (spec.detector.downTVD >= 0)
                d->downTVD = ((TVD *)graphObjs[spec.detector.downTVD].get());
        }
        if (spec.type == ISObjSpec::ISObjType::Sight)
        {
            Sight *s = (Sight *)graphObjs[i].get();
            s->sig = (Signal *)graphObjs[spec.sight.signal_index].get();
        }
        if (spec.type == ISObjSpec::ISObjType::Signal)
        {
            Signal *s = (Signal *)graphObjs[i].get();
            s->detector = (Detector *)graphObjs[spec.signal.detector_index].get();
        }
    }

    world.objects = std::move(graphObjs);
}

void mk_routes(Sim sim, World &world, const InfrastructureSpec &is) {
  for (auto &i_route : is.routes) {
    auto &route = i_route.second;
    vector<pair<TVD *, vector<Resource *>>> releases;
    for (auto &spec : route.releases) {
      vector<Resource *> res;
      for (auto &ridx : spec.resources)
        res.push_back((Resource *)world.objects[ridx].get());
      releases.push_back(
          make_pair((TVD *)world.objects[spec.trigger].get(), std::move(res)));
    }
    Signal *entrySignal = nullptr;
    if (route.entry_signal >= 0) {
      entrySignal = (Signal *)world.objects[route.entry_signal].get();
    }
    vector<pair<Switch *, SwitchState>> swPos;
    for (auto &sw : route.switches) {
      swPos.push_back(
          make_pair((Switch *)world.objects[sw.first].get(), sw.second));
    }
    vector<TVD *> tvds;
    for (auto &tvd : route.tvds) {
      tvds.push_back((TVD *)world.objects[tvd].get());
    }

    printf("Constructing route %s\n", i_route.first.c_str());
    world.routes.emplace(i_route.first, Route(sim, entrySignal, swPos, tvds,
                                              releases, route.length));
  }
}


World World::Create(Sim sim, InfrastructureSpec &is)
{
    World world;
    mk_infrastructure(sim, world, is);
    mk_routes(sim, world, is);
    return world;
}