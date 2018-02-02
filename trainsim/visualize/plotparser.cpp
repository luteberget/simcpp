#include "plotparser.h"
#include "../parser.h"
#include "../history.h"
#include <sstream>

HistoryItem::StartEnd parse_startend(std::istream &in)
{
    string s;
    in >> s;
    if (s == "start")
        return HistoryItem::StartEnd::Start;
    if (s == "end")
        return HistoryItem::StartEnd::End;
    throw string("Start/end parse error.");
}

bool parse_redgreen(std::istream &in)
{
    string s;
    in >> s;
    if (s == "green")
        return true;
    if (s == "red")
        return false;
    throw string("Red/green parse error.");
}

bool startEndToBool(HistoryItem::StartEnd x)
{
    return x == HistoryItem::StartEnd::Start;
}

TrainAction parse_trainaction(std::istream &in)
{
    string x;
    in >> x;
    if (x == "accel")
        return TrainAction::Accel;
    if (x == "brake")
        return TrainAction::Brake;
    if (x == "coast")
        return TrainAction::Coast;
    throw string("Train action parse error.");
}

pair<string, string> parse_keyvalue(std::istream &in, string expectedkey = "")
{
    string key;
    std::getline(in, key, '=');
    if (expectedkey != "" && key != expectedkey)
    {
        throw string("Expected key ") + expectedkey;
    }
    string value;
    in >> value;
    return {key, value};
}

pair<string, double> parse_keyvalue_double(std::istream &in, string expectedkey = "")
{
    string keyline;
    std::getline(in, keyline, '=');
    std::stringstream keystream(keyline);
    auto key = parse_string(keystream);
    if (expectedkey != "" && key != expectedkey)
    {
       throw string("Expected key ") + expectedkey + " got " + key;
    }
    double value = parse_double(in);
    return {key, value};
}

TrainPlot parse_plot(std::istream &in)
{
    TrainPlot p;
    //p.dtview = add_view_margin(0.05, ImRect(-50, 0, 212.5, 32.5));
    //p.dvview = add_view_margin(0.05, ImRect(-50, 0, 212.5, 10.0));

    TrainPlot::Train t;
    t.name = "train1";
    t.params = {1.0, 0.8, 99.0, 50.0};

    double now = 0;
    string line;
    while (std::getline(in, line))
    {
        std::stringstream s(line);
        auto dt = parse_double(s);
        now += dt;

        auto type = parse_string(s);

        if (type == "route")
        {
            auto name = parse_string(s);
            auto on = startEndToBool(parse_startend(s));
            //printf("Route %s on %g %d\n",name.c_str(), time, on);
            p.routes[name].history.push_back({now, on});
        }
        else if (type == "alloc")
        {
            auto name = parse_string(s);
            auto on = startEndToBool(parse_startend(s));
            p.tvds[name].history.push_back({now, on});
        }
        else if (type == "signal")
        {
            auto name = parse_string(s);
            auto on = parse_redgreen(s);
            //printf("Signal %s %g %s\n", name.c_str(), now, on ? "green" : "red");
            p.signals[name].history.push_back({now, on});
        }
        else if (type == "train")
        {
            auto action = parse_trainaction(s);
            auto x = parse_keyvalue_double(s, "x");
            auto v = parse_keyvalue_double(s, "v");
            auto last_x = t.history.size() > 0 ? t.history.back().x : 0.0;
            t.history.push_back({now, last_x + x.second, v.second, action});
        }
    }

    /*t.history.push_back({0.0, 0.0, 0.0, TrainAction::Accel});
    t.history.push_back({10.0, 50.0, 10.0, TrainAction::Coast});
    t.history.push_back({20.0, 150.0, 10.0, TrainAction::Brake});
    t.history.push_back({32.5, 212.5, 0.0, TrainAction::Coast});*/

    printf("Read train history with %lu history items.\n", t.history.size());
    p.trains.push_back(t);
    p.dtview = add_view_margin(0.1,p.default_view());
    return p;
}