#ifndef __TRAIN_H
#define __TRAIN_H

#include "../inputs.h"
#include "../history.h"
#include "../simcpp.h"
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
    string name;
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
    TrainAction action;
    double time;

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

    Train(Sim s, OutputWriter *out, TrainRun trainRun)
        : Process(s), out(out), name(trainRun.name), params(trainRun.params)
    {
        authority = trainRun.startAuthorityLength;
        velocity = 0.0;
        location = trainRun.startLoc;
        dir = trainRun.startDir;
        time = s->get_now();
        
        if (location.offset >= params.length)
        {
            printf("ERROR: train is not at boundary");
        }

        nodesUnderTrain.push_back({location.obj, params.length - location.offset});
    }

    void add_sight(Signal *s, double d) {}
    Direction get_dir() { return dir; }

    virtual bool Run() override;
    void update();
};

#endif