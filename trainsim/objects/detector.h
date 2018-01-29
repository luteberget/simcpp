#ifndef __DETECTOR_H
#define __DETECTOR_H

#include "infrastructure_object.h"
#include "tvd.h"
typedef shared_ptr<Simulation> Sim;

class Detector : public ISObj
{
  public:
    shared_ptr<Event> touched_event;
    TVD *upTVD;
    TVD *downTVD;
    Sim s;
    Detector(Sim s) : s(s) { touched_event = std::make_shared<Event>(s); }

    void touched()
    {
        this->s->schedule(this->touched_event);
        this->touched_event = std::make_shared<Event>(s);
    }

    void arrive_front(Train &t) override
    {
        if (upTVD != nullptr && t.get_dir() == Direction::Up)
            upTVD->set_occupied(true);
        if (downTVD != nullptr && t.get_dir() == Direction::Down)
            downTVD->set_occupied(true);
        this->touched();
    }
    void arrive_back(Train &t) override
    {
        if (upTVD != nullptr && t.get_dir() == Direction::Up)
            upTVD->set_occupied(false);
        if (downTVD != nullptr && t.get_dir() == Direction::Down)
            downTVD->set_occupied(false);
        this->touched();
    }
};

#endif