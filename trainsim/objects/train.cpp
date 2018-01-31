#include <vector>
#include "train.h"
#include "../traindynamics.h"
#include "infrastructure_object.h"
#include "signal.h"

#include "../history.h"

#define TOL (1e-4)

double update_authority(double auth, vector<Sighted> &sighted)
{
    for (auto &s : sighted)
    {
        printf("->* Signal %p: %g %s\n", s.sig, s.sig->get_authority(), s.sig->get_green() ? "green" : "red");
        auth = s.dist + s.sig->get_authority();
        if (!s.sig->get_green())
        {
            auth -= 10.0; // Stop before signal
            break;        // Don't use greens behind the red.
        }
    }
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
    NoTrack,
    Exiting
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

    if (link == NoLink)
        return {TargetReason::NoTrack, 0.0};

    double distToFrontNode = link.length - loc.offset;

    auto &underTrain = t.get_nodes_under_train();
    if (underTrain.size() >= 1 &&
        underTrain[0].offset < distToFrontNode)
    {
        return {TargetReason::ClearNode, underTrain[0].offset};
    }

    if (link == BoundaryLink)
        return {TargetReason::Exiting, std::numeric_limits<double>::infinity()};

    return {TargetReason::ReachNode, distToFrontNode};
}

LinearTrainStep drive_dt(Train &t, const vector<PosVel> &speed_profile)
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
        if (step.action == TrainAction::Brake)
            printf("BRAKE");
        if (step.action == TrainAction::Coast)
            printf("COAST");
        printf("    No driving possible.\n");
    }

    return step;
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
    while (!has_exited)
    {
        authority = update_authority(authority, signalInSight);
        step = drive_dt(*this, speed_profile(authority));
        PROC_WAIT_FOR(sim->start_process<AnyOf>(
            next_events(*this, sim, step.dt,
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
        //printf("TRAIN stepped %g\n", dt);
        // Update train's position and velocity
        auto update = trainUpdate(params, velocity, {step.action, dt});
        velocity = update.velocity;
        printf("    offset %g -> %g\n", location.offset, location.offset + update.dist);
        location.offset += update.dist;

        string mode;
        if (step.action == TrainAction::Accel)
            mode = "ACCEL";
        if (step.action == TrainAction::Brake)
            mode = "BRAKE";
        if (step.action == TrainAction::Coast)
            mode = "COAST";
        printf("TRAIN\t%s\t%g\t%g\t%g\t%p\t%g\n", 
        mode.c_str(), this->sim->get_now(),
               dt, this->location.offset, this->location.obj, this->velocity);

        // Reduce distance to sighted signals
        for (auto &see : signalInSight)
        {
            see.dist -= update.dist;
        }

        // Reduce distance to nodes under train
        for (auto &node : nodesUnderTrain)
        {
            node.offset -= update.dist;
        }

        authority -= update.dist;

        out->write(HistoryItem::mkTrainStatus(step.action, update.dist, update.velocity, &name));
    }
    time = sim->get_now();

    auto next_node = node_dist(*this);
    while (next_node.dist < TOL &&
           next_node.reason != TargetReason::NoTrack)
    {
        if (next_node.reason == TargetReason::ReachNode)
        {
            auto link = location.obj->next(dir);
            printf("reached node %s\n", link.obj == nullptr ? "NoObject" : link.obj->name.c_str());
            location.obj = link.obj;
            location.offset -= link.length;
            printf("updating offset to %g\n", location.offset);
            location.obj->arrive_front(*this);
            nodesUnderTrain.push_back({location.obj, params.length});
        }
        else if (next_node.reason == TargetReason::ClearNode)
        {
            auto cleared = nodesUnderTrain[0];
            printf("Cleared node %p %s\n", cleared.obj, cleared.obj->name.c_str());
            cleared.obj->arrive_back(*this);
            nodesUnderTrain.erase(this->nodesUnderTrain.begin());
        }
        //printf("NODE DIST\n");
        next_node = node_dist(*this);
        //printf("NODE DIST DONE\n");
    }
    //printf("UPDATE DONE\n");
}
