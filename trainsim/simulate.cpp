#include "simulate.h"
#include "simcpp.h"
#include "world.h"
#include "objects/train.h"

TrainRun create_train_input(const TrainRunSpec &spec,
                            const vector<unique_ptr<ISObj>> &objIndex)
{
    TrainRun t;
    t.params = spec.params;
    t.startDir = spec.startDir;
    t.startAuthorityLength = spec.startAuthorityLength;
    t.startLoc = {objIndex[spec.startLoc.obj].get(), 0.0};
    return t;
}

typedef std::function<bool(pair<double, HistoryItem>)> OutputFunc;

class FuncWriter : public OutputWriter
{
    OutputFunc f;
    shared_ptr<Simulation> sim;
    double t = 0.0;

  public:
    FuncWriter(OutputFunc f, shared_ptr<Simulation> sim)
        : f(f), sim(sim) {}

    void write(HistoryItem item) override
    {
        auto dt = sim->get_now() - t;
        f({dt, item});
        t = sim->get_now();
    }
};

void simulate(InfrastructureSpec &infrastructure, Plan &plan,
              OutputFunc output)
{
    auto sim = Simulation::create();
    auto writer = std::move(unique_ptr<FuncWriter>(new FuncWriter(output, sim)));
    auto world = World::Create(infrastructure, sim, std::move(writer));

    vector<shared_ptr<Train>> trains;
    for (auto &planitem : plan.items)
    {
        printf("    Next plan event at dt=%g, advancing time...\n", planitem.dt);
        sim->advance_by(planitem.dt);
        if (planitem.type == Plan::ItemType::Route)
        {
            printf("    Activating route %s\n", planitem.name.c_str());
            auto route = world.routes.find(planitem.name);
            if (route == world.routes.end())
            {
                printf("    ERROR finding route\n");
            }
            else
            {
                route->second.activate();
            }
        }
        else if (planitem.type == Plan::ItemType::Train)
        {
            printf("    STARTING TRAIN %s\n", planitem.name.c_str());
            auto &trainspec = planitem.trainData;
            auto trainInput = create_train_input(trainspec, world.objects);
            auto proc = sim->start_process<Train>(world.output.get(), planitem.name, trainInput);
            trains.push_back(proc);
        }
    }
    printf("    Plan input done, finishing simulation\n");
    sim->run();
    printf("    Simulation finsihed\n");
    vector<string> unfinishedTrains;
    for(auto& t: trains) {
        if(!t->is_success()) unfinishedTrains.push_back(t->name);
    }
    if(unfinishedTrains.size() > 0) {
        printf("Warning: trains did not finish:\n");
        for(auto& s  :unfinishedTrains) printf("  - %s\n",s.c_str());
    }
}
