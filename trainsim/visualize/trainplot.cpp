#include "trainplot.h"

ImRect add_view_margin(double c, ImRect r)
{
    auto w = r.Max.x - r.Min.x;
    auto h = r.Max.y - r.Min.y;
    return ImRect(r.Min.x - c * w, r.Min.y - c * h,
                  r.Max.x + c * w, r.Max.y + c * h);
}


ImVec2 normalize_view_limits(const ImVec2 &v, const ImRect &view)
{
    return ImVec2(
        (v.x - view.Min.x) / (view.Max.x - view.Min.x),
        1.0 - (v.y - view.Min.y) / (view.Max.y - view.Min.y));
}

ImVec2 stretch_normalized(const ImVec2 &v, const ImRect &r)
{
    return ImLerp(r.Min, r.Max, v);
}