
#include "parser.h"
#include "simulate.h"
#include "history.h"

#include <iostream>
using std::cout;
using std::endl;

string startendtostring(HistoryItem::StartEnd x)
{
    if (x == HistoryItem::StartEnd::Start)
        return "start";
    if (x == HistoryItem::StartEnd::End)
        return "end";
    return "error";
}

string positiontostring(SwitchState s)
{
    if (s == SwitchState::Left)
        return "left";
    if (s == SwitchState::Right)
        return "right";
    if (s == SwitchState::Unknown)
        return "unknown";
    return "error";
}

string aspecttostring(bool green)
{
    return green ? "green" : "red";
}

string actiontostring(TrainAction s)
{
    if (s == TrainAction::Accel)
        return "accel";
    if (s == TrainAction::Brake)
        return "brake";
    if (s == TrainAction::Coast)
        return "coast";
    return "error";
}

bool write_historyitem(std::ostream &out, pair<double, HistoryItem> item)
{
    // This function defines the history output grammar
    auto &dt = item.first;
    auto &d = item.second;
    out << dt << " ";
    if (d.tag == HistoryItem::ItemType::Allocation)
    {
        out << "alloc " << *d.allocation.resource << " "
            << startendtostring(d.allocation.status) << endl;
    }
    else if (d.tag == HistoryItem::ItemType::MovablePosition)
    {
        out << "position " << *d.movablePosition.resource << " "
            << positiontostring(d.movablePosition.position) << endl;
    }
    else if (d.tag == HistoryItem::ItemType::RouteActivation)
    {
        out << "route " << *d.routeActivation.route << " "
            << startendtostring(d.routeActivation.status) << endl;
    }
    else if (d.tag == HistoryItem::ItemType::SignalAspect)
    {
        out << "signal " << *d.signalAspect.signal << " "
            << aspecttostring(d.signalAspect.green) << endl;
    }
    else if (d.tag == HistoryItem::ItemType::TrainStatus)
    {
        out << "train " << actiontostring(d.trainStatus.action) << " "
            << "x=" << d.trainStatus.x << " v=" << d.trainStatus.v << endl;
    }
    else
    {
        out << "error" << endl;
    }

    return true;
}

int main()
{
    try
    {
        auto p = parse_simulatorinput(std::cin);
        cout << "parsed " << p.infrastructure.driveGraph.size() << " objects" << endl;
        cout << "parsed " << p.infrastructure.routes.size() << " routes" << endl;
        cout << "parsed " << p.plan.items.size() << " plan items" << endl;

        auto out = std::bind(write_historyitem, std::ref(std::cout), std::placeholders::_1);
        simulate(p.infrastructure, p.plan, out);

        return 0;
    }
    catch (string s)
    {
        cout << s << endl;
        return 1;
    }
}
