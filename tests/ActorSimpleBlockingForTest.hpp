#pragma once
#include "ActorBlocking.h"
#include <iostream>

class ActorSimpleBlockingForTest : public rf::ActorBlocking
{
public:
    ActorSimpleBlockingForTest(std::string id) : 
    rf::ActorBlocking(id)
    , steps(0)
    { 
        _type = "ActorSimpleBlockingForTest";
        std::cout << "Constructor " << typeid(this).name() << std::endl; 
       portOut = addPort(std::string("Output"));
       
    }
    bool Process() override
    {
        auto ptrData = std::make_shared< rf::IData>();
        portOut->Notify(ptrData);
        steps++;
        //std::cout <<Id()<< " step : " <<steps<< std::endl;
        return true;
    }
    size_t steps;
    std::shared_ptr<rf::IPort> portOut ;
};