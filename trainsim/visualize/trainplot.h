#ifndef __VISUALIZE__TRAINPLOT__H
#define __VISUALIZE__TRAINPLOT__H

#include "../inputs.h"
#include "imgui.h"
#include "imgui_internal.h"

ImRect add_view_margin(double c, ImRect r);
ImVec2 normalize_view_limits(const ImVec2 &v, const ImRect &view);
ImVec2 stretch_normalized(const ImVec2 &v, const ImRect &r);

struct TrainPlot
{
    enum class PlotType
    {
        DistanceTime,
        DistanceVelocity
    };
    struct TrainHistorySnapshot
    {
        double t, x, v;
        TrainAction action;
    };
    struct Train
    {
        string name;
        LinearTrainParams params;
        vector<TrainHistorySnapshot> history;
    };

    PlotType type = PlotType::DistanceTime;

    ImRect dtview;
    ImRect dvview;
    vector<Train> trains;

    template <class T>
    struct History
    {
        //string name;
        vector<pair<double, T>> history;
    };

    unordered_map<string, History<bool>> signals; // TODO multiple signal aspects?
    unordered_map<string, History<bool>> tvds;
    unordered_map<string, History<bool>> routes;
    unordered_map<string, History<SwitchState>> switches;

    ImRect default_view()
    {
        auto a_point = ImVec2(trains[0].history[0].x, trains[0].history[0].t);
        ImRect view(a_point, a_point);
        for (auto &t : trains)
        {
            for (auto &h : t.history)
            {
                view.Add(ImVec2(h.x, h.t));
            }
        }

        //printf("VIEW (%g,%g) (%g,%g)\n", view.Min.x, view.Min.y, view.Max.x, view.Max.y);
        return view;
    }
};

#endif