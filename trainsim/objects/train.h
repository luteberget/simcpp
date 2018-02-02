#ifndef __TRAIN_H
#define __TRAIN_H

#include "../inputs.h"
#include "../history.h"
#include "../simcpp.h"
#include "../traindynamics.h"
using std::vector;
typedef shared_ptr<Simulation> Sim;

class Signal;
class ISObj;

struct TrainLoc
{
    ISObj *obj;
    double offset;
};

struct TrainRun
{
    LinearTrainParams params;
    Direction startDir;
    double startAuthorityLength;
    TrainLoc startLoc;
    // vector<pair<TrainLoc, double>> stops;
    // TrainLoc endLoc;
};

struct Sighted
{
    Signal *sig;
    double dist;
};

struct ObjDist
{
    ISObj *obj;
    double offset;
};

class Train : public Process
{
  protected:
    OutputWriter *out;

    // Train state
    TrainLoc location;
    double velocity;
    double max_velocity;
    Direction dir;
    double time;
    LinearTrainStep step;
    double has_exited = false;

    // Current authority and signals in sight
    double authority;
    vector<Sighted> signalInSight;

    // Nodes under the train
    vector<ObjDist> nodesUnderTrain;

  public:
    // Train info
    const string name;
    const LinearTrainParams params;
    double get_velocity() const { return velocity; }
    double get_max_velocity() const { return max_velocity; }
    const TrainLoc &get_location() const { return location; }
    const vector<ObjDist> &get_nodes_under_train() const { return nodesUnderTrain; }

    Train(Sim s, OutputWriter *out, string name, TrainRun trainRun)
        : Process(s), out(out), name(name), params(trainRun.params)
    {
        authority = trainRun.startAuthorityLength;
        velocity = 0.0;
        max_velocity = 2.0;
        location = trainRun.startLoc;
        dir = trainRun.startDir;
        time = s->get_now();

        if (location.offset >= params.length)
        {
            fprintf(stderr, "ERROR: train is not at boundary");
        }

        nodesUnderTrain.push_back({location.obj, params.length - location.offset});
    }

    void can_see(Signal *s, double d)
    {
        signalInSight.push_back({s, d});
        // Bubble sort it into place to ensure
        // that signals are always sorted by distance.
        size_t i = signalInSight.size() - 1;
        while (i > 0 && signalInSight[i - 1].dist > signalInSight[i].dist)
        {
            std::swap(signalInSight[i - 1], signalInSight[i]);
            i -= 1;
        }
    }

    void cannot_see(Signal *s) {
        for(int i = signalInSight.size()-1; i >= 0; i--) {
            if(signalInSight[i].sig == s) {
                signalInSight.erase(signalInSight.begin()+i);
            }
        }
    }

    void cleared_boundary() { has_exited = true; }

    Direction get_dir() { return dir; }

    virtual bool Run() override;
    void update();
};

#endif
