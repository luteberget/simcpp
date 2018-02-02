// ImGui - standalone example application for SDL2 + OpenGL
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.
// (SDL is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan graphics context creation, etc.)
// (GL3W is a helper library to access OpenGL functions since there is no standard header to access modern OpenGL functions easily. Alternatives are GLEW, Glad, etc.)

#include "imgui.h"
#include "imgui_impl_sdl_gl3.h"
#include <stdio.h>
#include <GL/gl3w.h> // This example is using gl3w to access OpenGL functions (because it is small). You may use glew/glad/glLoadGen/etc. whatever already works for you.
#include <SDL.h>

#include <vector>
#include <iostream>
#include <fstream>
using std::vector;

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"

#include "../history.h"
#include "../inputs.h"
#include "../traindynamics.h"
#include "trainplot.h"
#include "plotparser.h"

#include <signal.h>
Uint32 reload_event_no;
SDL_Event reload_event;
void signal_callback_handler(int signum)
{
    //printf("Caught signal %d\n", signum);
    SDL_PushEvent(&reload_event);
    // Cleanup and close up stuff here
    // Terminate program
}

void trainPlotLine(ImGuiWindow *window, const ImRect &view, const ImRect &inner_bb,
                   bool hovered,
                   const ImVec2 &p0, const ImVec2 &p1, double thickness = 4.0)
{
    using namespace ImGui;

    static const ImU32 col_base = GetColorU32(ImGuiCol_PlotLines);
    static const ImU32 col_hovered = GetColorU32(ImGuiCol_PlotLinesHovered);

    auto xy0 = stretch_normalized(normalize_view_limits(p0, view), inner_bb);
    auto xy1 = stretch_normalized(normalize_view_limits(p1, view), inner_bb);
    window->DrawList->AddLine(xy0, xy1, hovered ? col_hovered : col_base, thickness);
}

//ImVec2 px2view(ImVec2 px, ImRect bb, ImRect view) {
//(bb-bb.Min)
//}

ImRect zoom_plot(TrainPlot &plot, double factor, ImVec2 center, ImRect view)
{
    auto view_min = view.Min;
    auto view_size = view.GetSize();
    view.Expand(view.GetSize() * (1 - factor));
    view.Translate(((center - view.Min) - (center - view_min) / view_size * view.GetSize()));
    return view;
}

void PlotTrainHistory(TrainPlot &plot)
{
    using namespace ImGui;

    ImGuiWindow *window = GetCurrentWindow();
    if (window->SkipItems)
        return;

    ImGuiContext &g = *GImGui;
    const ImGuiStyle &style = g.Style;
    //const ImVec2 label_size = CalcTextSize(label, NULL, true);

    ImVec2 canvas_size;
    canvas_size.x = CalcItemWidth();
    canvas_size.y = GetWindowSize().y - GetCursorPos().y - 2 * style.FramePadding.y;

    // TODO create room for axes
    // TODO can/should we cache the display list for axis lines?
    const ImVec2 graph_size = canvas_size;

    const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(graph_size.x, graph_size.y));
    const ImRect inner_bb(frame_bb.Min + style.FramePadding, frame_bb.Max - style.FramePadding);
    const ImRect total_bb(frame_bb.Min, frame_bb.Max);
    ItemSize(total_bb, style.FramePadding.y);
    if (!ItemAdd(total_bb, 0))
        return;
    const bool hovered = ItemHoverable(inner_bb, 0);

    if (hovered && g.IO.MouseWheel != 0.0f)
    {
        auto center = stretch_normalized(normalize_view_limits(g.IO.MousePos, inner_bb), plot.dtview);
        //std::cout << "zooming on " << center.x << "," << center.y << std::endl;
        plot.dtview = zoom_plot(plot, 1 + g.IO.MouseWheel * 0.1, center, plot.dtview);
    }

    RenderFrame(frame_bb.Min, frame_bb.Max, GetColorU32(ImGuiCol_FrameBg), true, style.FrameRounding);

    static const int px_stride = 3;
    if (IsMouseDragging(0))
    {
        //static char x[100];
        //sprintf(x, "dragged %g %g\n", g.IO.MouseDelta.x, g.IO.MouseDelta.y);
        //SetTooltip(x);
        plot.dtview.Translate(ImVec2(-1.0, 1.0) * g.IO.MouseDelta * plot.dtview.GetSize() / inner_bb.GetSize());
    }
    if (hovered)
    {
        //SetTooltip("hovered");
        //window->DrawList->AddLine(inner_bb.Min, inner_bb.Max, hovered ? col_hovered : col_base, 10.0);
    }
    for (auto &train : plot.trains)
    {
        // At least these types are relevant:
        // 1. x=distance / y=time (or swapped axes?)
        // 2. x=distance / y=velocity

        double dbar = (plot.dtview.Max.y - plot.dtview.Min.y) / 50;

        for (size_t i = 0; i < train.history.size() - 1; i++)
        {
            auto &prev = train.history[i];
            auto &next = train.history[i + 1];
            static const size_t num_segments = 20;
            vector<ImVec2> frontOfTrain;
            auto last_bar_t = -std::numeric_limits<double>::infinity();
            for (size_t i = 0; i < num_segments; i++)
            {
                double dt0 = (next.t - prev.t) * ((double)i / (double)num_segments);
                double dt1 = (next.t - prev.t) * ((double)(i + 1) / (double)num_segments);
                auto t0 = trainUpdate(train.params, prev.v, {next.action, dt0});
                auto t1 = trainUpdate(train.params, prev.v, {next.action, dt1});

                if (plot.type == TrainPlot::PlotType::DistanceTime)
                {
                    auto front0 = ImVec2(prev.x + t0.dist, prev.t + dt0);
                    auto front1 = ImVec2(prev.x + t1.dist, prev.t + dt1);
                    auto l = ImVec2(train.params.length, 0.0);
                    trainPlotLine(window, plot.dtview, inner_bb, hovered, front0, front1);
                    trainPlotLine(window, plot.dtview, inner_bb, hovered, front0 - l, front1 - l);

                    //if(last_bar_t + dbar < front0.y) {
                    trainPlotLine(window, plot.dtview, inner_bb, false, front0, front0 - l, 1.0);
                    last_bar_t = front0.y;
                    //}

                    /*if(i == 0) { frontOfTrain.push_back(front0); }
                    frontOfTrain.push_back(front1);*/
                }
                else if (plot.type == TrainPlot::PlotType::DistanceVelocity)
                {
                    auto v0 = ImVec2(prev.x + t0.dist, t0.velocity);
                    auto v1 = ImVec2(prev.x + t1.dist, t1.velocity);
                    trainPlotLine(window, plot.dvview, inner_bb, hovered, v0, v1);
                }
            }
            /*static const auto fill = GetColorU32(ImGuiCol_PlotLinesHovered);
            size_t n = frontOfTrain.size();
            for(size_t i = 0; i < n; i++) frontOfTrain.push_back(frontOfTrain[i] - ImVec2(train.params.length,0.0));
            for(size_t i = 0; i < frontOfTrain.size(); i++) frontOfTrain[i] = stretch_normalized(normalize_view_limits(frontOfTrain[i], plot.dtview), inner_bb);
            window->DrawList->AddConvexPolyFilled(frontOfTrain.data(), frontOfTrain.size(), fill);*/
        }
    }

    //if (values_count > 0)
    //{
    //    int res_w = ImMin((int)graph_size.x, values_count) - 1;
    //    int item_count = values_count - 1;

    //    // Tooltip on hover
    //    int v_hovered = -1;
    //    if (hovered)
    //    {
    //        const float t = ImClamp((g.IO.MousePos.x - inner_bb.Min.x) / (inner_bb.Max.x - inner_bb.Min.x), 0.0f, 0.9999f);

    //        const int v_idx = (int)(t * item_count);
    //        IM_ASSERT(v_idx >= 0 && v_idx < values_count);

    //        const float v0 = values_getter(data, (v_idx + values_offset) % values_count);
    //        const float v1 = values_getter(data, (v_idx + 1 + values_offset) % values_count);

    //        SetTooltip("%d: %8.4g\n%d: %8.4g", v_idx, v0, v_idx + 1, v1);

    //        v_hovered = v_idx;
    //    }

    //    const float t_step = 1.0f / (float)res_w;

    //    float v0 = values_getter(data, (0 + values_offset) % values_count);
    //    float t0 = 0.0f;
    //    ImVec2 tp0 = ImVec2(t0, 1.0f - ImSaturate((v0 - scale_min) / (scale_max - scale_min))); // Point in the normalized space of our target rectangle
    //    //float histogram_zero_line_t = (scale_min * scale_max < 0.0f) ? (-scale_min / (scale_max - scale_min)) : (scale_min < 0.0f ? 0.0f : 1.0f);   // Where does the zero line stands

    //    const ImU32 col_base = ImGuiCol_PlotLines;
    //    const ImU32 col_hovered = ImGuiCol_PlotLinesHovered;

    //    for (int n = 0; n < res_w; n++)
    //    {
    //        const float t1 = t0 + t_step;
    //        const int v1_idx = (int)(t0 * item_count + 0.5f);
    //        IM_ASSERT(v1_idx >= 0 && v1_idx < values_count);
    //        const float v1 = values_getter(data, (v1_idx + values_offset + 1) % values_count);
    //        const ImVec2 tp1 = ImVec2(t1, 1.0f - ImSaturate((v1 - scale_min) / (scale_max - scale_min)));

    //        // NB: Draw calls are merged together by the DrawList system. Still, we should render our batch are lower level to save a bit of CPU.
    //        ImVec2 pos0 = ImLerp(inner_bb.Min, inner_bb.Max, tp0);
    //        ImVec2 pos1 = ImLerp(inner_bb.Min, inner_bb.Max, tp1);
    //        window->DrawList->AddLine(pos0, pos1, v_hovered == v1_idx ? col_hovered : col_base);

    //        /*            else if (plot_type == ImGuiPlotType_Histogram)
    //        {
    //            if (pos1.x >= pos0.x + 2.0f)
    //                pos1.x -= 1.0f;
    //            window->DrawList->AddRectFilled(pos0, pos1, v_hovered == v1_idx ? col_hovered : col_base);
    //        }
    //  */
    //        t0 = t1;
    //        tp0 = tp1;
    //    }
    //}

    // Text overlay
    /*static char overlay_text[100] = "";
    sprintf(overlay_text, "At zoom %g.", 1.0);
    if (overlay_text)
        RenderTextClipped(ImVec2(frame_bb.Min.x, frame_bb.Min.y + style.FramePadding.y), frame_bb.Max, overlay_text, NULL, NULL, ImVec2(0.5f, 0.0f));
*/
    //if (label_size.x > 0.0f)
    //    RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, inner_bb.Min.y), label);
}

TrainPlot plot;
const char* filename;
void reload_file() {
    printf("Reloading file!\n");
    std::ifstream inFile = std::ifstream(filename);
    try
    {
        TrainPlot p = parse_plot(inFile);
        plot = p;
    }
    catch (string s)
    {
        std::cerr << "Parse error: " << s << std::endl;
    }
}

int main(int argc, char ** argv)
{
    filename = argv[1];
    reload_file();

    signal(SIGUSR1, signal_callback_handler);
    reload_event_no = SDL_RegisterEvents(1);
    SDL_zero(reload_event);
    reload_event.type = reload_event_no;

    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    // Setup window
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_DisplayMode current;
    SDL_GetCurrentDisplayMode(0, &current);
    SDL_Window *window = SDL_CreateWindow("ImGui SDL2+OpenGL3 example", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    SDL_GLContext glcontext = SDL_GL_CreateContext(window);
    gl3wInit();

    // Setup ImGui binding
    ImGui_ImplSdlGL3_Init(window);

    // Setup style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'extra_fonts/README.txt' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    ImGuiIO &io = ImGui::GetIO();
    //io.Fonts->AddFontDefault();
    io.Fonts->AddFontFromFileTTF("imgui/misc/fonts/Roboto-Medium.ttf", 18.0f);
    //io.Fonts->AddFontFromFileTTF("imgui/extra_fonts/Cousine-Regular.ttf", 18.0f);
    //io.Fonts->AddFontFromFileTTF("imgui/extra_fonts/DroidSans.ttf", 18.0f);
    //io.Fonts->AddFontFromFileTTF("imgui/extra_fonts/ProggyTiny.ttf", 10.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);

    bool show_demo_window = false;
    //mabool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Main loop
    bool done = false;
    SDL_Event event;
    double zoom = 1.0;



    while (!done)
    {
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        SDL_WaitEvent(&event);
        ImGui_ImplSdlGL3_ProcessEvent(&event);
        if (event.type == SDL_QUIT)
            done = true;
        if (event.type == reload_event_no) reload_file();
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSdlGL3_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == reload_event_no) reload_file();
        }
        ImGui_ImplSdlGL3_NewFrame(window);
        float width = ImGui::GetIO().DisplaySize.x;
        float height = ImGui::GetIO().DisplaySize.y;

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5, 5));

        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);
        ImGuiWindowFlags window_flags =
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings;
        bool yes = true;
        ImGui::Begin("mainwin", &yes, window_flags);

        static float arr[] = {0.6f, 0.1f, 1.0f, 0.5f, 0.92f, 0.1f, 0.2f};
        ImGui::PlotLines("BLÆ##Frame Times", arr, IM_ARRAYSIZE(arr));

        int ptype = plot.type == TrainPlot::PlotType::DistanceTime ? 0 : 1;
        ImGui::Combo("Plot type", &ptype, "Distance/time\0Distance/velocity\0\0");
        plot.type = ptype == 0 ? TrainPlot::PlotType::DistanceTime : TrainPlot::PlotType::DistanceVelocity;
        ImGui::PushItemWidth(-1);

        PlotTrainHistory(plot);

        //ImGui::PlotLines("##Frame Times", arr, IM_ARRAYSIZE(arr));

        ImGui::End();
        ImGui::PopStyleVar(); // Tighten spacing

        // 3. Show the ImGui demo window. Most of the sample code is in ImGui::ShowDemoWindow(). Read its code to learn more about Dear ImGui!
        //ImGui::ShowDemoWindow(&show_demo_window);

        // Rendering
        glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui::Render();
        SDL_GL_SwapWindow(window);
    }

    // Cleanup
    ImGui_ImplSdlGL3_Shutdown();
    SDL_GL_DeleteContext(glcontext);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
