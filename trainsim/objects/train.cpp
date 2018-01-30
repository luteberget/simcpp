#include <vector>
#include "train.h"
#include "../traindynamics.h"
#include "infrastructure_object.h"
#include "signal.h"

#define TOL (1e-4)

double update_authority(double auth, vector<Sighted> &sighted)
{
    // TODO update based on signals
    return auth;
}

std::vector<PosVel> speed_profile(double auth)
{
    return {{auth, 0.0}};
}

enum class TargetReason
{
    Target,
    ReachNode,
    ClearNode,
    NoTrack
};

struct NodeDist
{
    TargetReason reason;
    double dist;
};

NodeDist node_dist(Train &t)
{
    auto &loc = t.get_location();
    auto link = loc.obj->next(t.get_dir());
    if (link == nullptr)
        return {TargetReason::NoTrack, 0.0};

    double distToFrontNode = link->length - loc.offset;

    auto &underTrain = t.get_nodes_under_train();
    if (underTrain.size() >= 1 &&
        underTrain[0].offset < distToFrontNode)
    {
        return {TargetReason::ClearNode, underTrain[0].offset};
    }
    return {TargetReason::ReachNode, distToFrontNode};
}

double drive_dt(Train &t, const vector<PosVel> &speed_profile)
{
    // Event: train travels within current authority until discrete change
    auto nextNode = node_dist(t);
    auto step = trainStep(t.params, nextNode.dist,
                          t.get_velocity(),
                          {t.get_max_velocity(), speed_profile});
    if (step.dt > TOL)
    {
        printf("    Calculated timeout for driving: %g\n", step.dt);
    }
    else
    {
        printf("    No driving possible.\n");
    }

    return step.dt;
}

vector<shared_ptr<Event>> next_events(
    Train &t, shared_ptr<Simulation> sim,
    double max_dt, const vector<Sighted> &signals)
{
    vector<shared_ptr<Event>> events;
    if (max_dt > TOL)
    {
        events.push_back(sim->timeout(max_dt));
    }
    else
    {
        for (auto &s : signals)
        {
            events.push_back(s.sig->authority_event);
        }
    }

    return events;
}

bool Train::Run()
{
    PT_BEGIN();
    while (true)
    {
        authority = update_authority(authority, signalInSight);
        PROC_WAIT_FOR(sim->start_process<AnyOf>(
            next_events(*this, sim,
                        drive_dt(*this, speed_profile(authority)),
                        this->signalInSight)));

        update();
    }
    PT_END();
}

void Train::update()
{
    double dt = sim->get_now() - time;
    if (dt > TOL)
    {
    }
    time = sim->get_now();

    auto next_node = node_dist(*this);
    while (next_node.dist < TOL &&
           next_node.reason != TargetReason::NoTrack)
    {
        if (next_node.reason == TargetReason::ReachNode)
        {
            auto link = location.obj->next(dir);
            if (link != nullptr)
            {
                location.obj = link->obj;
                location.offset -= link->length;
                location.obj->arrive_front(*this);
            }
        }
        else if (next_node.reason == TargetReason::ClearNode)
        {
            auto cleared = nodesUnderTrain[0];
            cleared.obj->arrive_back(*this);
            nodesUnderTrain.erase(this->nodesUnderTrain.begin());
        }
    }
}
