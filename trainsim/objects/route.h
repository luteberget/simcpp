#ifndef __ROUTE_H
#define __ROUTE_H

using std::pair;

#include "tvd.h"
#include "switch.h"
#include "signal.h"

typedef pair<TVD *, vector<Resource *>> ReleaseDef;

class Route : protected EnvObj
{
    vector<pair<Switch *, SwitchState>> switchPositions;
    vector<TVD *> tvds;
    Signal *entrySignal;
    vector<ReleaseDef> releases;
    double length;

  public:
    Route(Sim s, Signal *entrySignal, vector<pair<Switch *, SwitchState>> swPos,
          vector<TVD *> tvds, vector<ReleaseDef> releases, double length)
        : EnvObj(s), switchPositions(swPos), tvds(tvds), entrySignal(entrySignal),
          releases(releases), length(length)
    {

        printf("creating route, sim=%p\n", s.get());
    }

    shared_ptr<Process> activate()
    {
        printf("route::activate %p\n", this->env.get());
        return this->env->start_process<RouteActivation>(this);
    }

  private:
    bool resources_available()
    {
        bool available = true;
        for (auto &t : this->tvds)
            available &= !t->get_allocated();
        for (auto &s : this->switchPositions)
            available &= !s.first->get_allocated();
        return available;
    }

    bool switches_correct()
    {
        bool correct = true;
        for (auto &s : this->switchPositions)
            correct &= s.first->get_state() == s.second;
        return correct;
    }

    class RouteActivation : public Process
    {
        Route *route;
        void reserve_resources()
        {
            for (auto &t : this->route->tvds)
                t->set_allocated(true);
            for (auto &s : this->route->switchPositions)
                s.first->set_allocated(true);
        }

        shared_ptr<Process> turn_switches()
        {
            vector<shared_ptr<Event>> all;
            for (auto &s : this->route->switchPositions)
                all.push_back(s.first->turn(s.second));
            return this->sim->start_process<AllOf>(all);
        }

        shared_ptr<Process> wait_busy_resources()
        {
            vector<shared_ptr<Event>> all;
            for (auto &t : this->route->tvds)
            {
                if (t->get_allocated())
                    all.push_back(t->allocated_event);
            }
            for (auto &s : this->route->switchPositions)
            {
                if (s.first->get_allocated())
                    all.push_back(s.first->allocated_event);
            }
            return this->sim->start_process<AllOf>(all);
        }

      public:
        RouteActivation(Sim s, Route *r) : Process(s), route(r) {}

        virtual bool Run()
        {
            PT_BEGIN();
            printf("begin activate\n");
            while (!this->route->resources_available())
            {
                PROC_WAIT_FOR(wait_busy_resources());
            }
            printf("reserve\n");
            reserve_resources();
            while (!this->route->switches_correct())
            {
                PROC_WAIT_FOR(turn_switches());
            }
            // Release triggers
            for (auto &t : this->route->releases)
            {
                printf("trigger\n");
                this->sim->start_process<ReleaseTrigger>(t.first, t.second);
            }
            // Green entry signal
            printf("open signal\n");
            this->route->open_signal();
            printf("activation finished\n");
            PT_END();
        }
    };

    class CatchSignal : public Process
    {
        Signal *sig;
        Detector *d;

      public:
        CatchSignal(Sim s, Signal *sig) : Process(s), sig(sig) {}
        virtual bool Run()
        {
            PT_BEGIN();
            PROC_WAIT_FOR(sig->detector->touched_event);
            sig->set_green(false);
            sig->set_authority(0.0);
            PT_END();
        }
    };
    class ReleaseTrigger : public Process
    {
        TVD *tvd;
        vector<Resource *> resources;

      public:
        ReleaseTrigger(Sim s, TVD *tvd, vector<Resource *> resources)
            : Process(s), tvd(tvd), resources(resources) {}
        virtual bool Run()
        {
            PT_BEGIN();
            PROC_WAIT_FOR(tvd->occupied_event);
            PROC_WAIT_FOR(tvd->occupied_event);
            for (auto &r : resources)
                r->set_allocated(false);
            PT_END();
        }
    };

    shared_ptr<Event> open_signal()
    {
        this->entrySignal->set_green(true);
        this->entrySignal->set_authority(this->length);
        return this->env->start_process<CatchSignal>(entrySignal);
    }
};

#endif