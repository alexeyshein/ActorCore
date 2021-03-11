#pragma once
#include <iostream>
#include "ActorEventBased.h"

class ActorSimpleEventBasedForTest : public rf::ActorEventBased
{
public:
    ActorSimpleEventBasedForTest(std::string id) : 
    rf::ActorEventBased(id)
    , steps(0)
    { 
        _type = "ActorSimpleEventBasedForTest";
        std::cout << "Constructor " << typeid(this).name() << std::endl; 
        portInput = addPort(std::string("Input"));
        portOut = addPort(std::string("Output"));
    }


protected:
    void Process(const std::string& idPort, std::shared_ptr<rf::IData>& data) override
    {
        steps++;
        //std::cout <<idPort<<"->"<<id()<<" onRecive : "<<steps<< std::endl;
        portOut->Notify(data);
    }

    
public:
    std::shared_ptr<rf::IPort> portInput;
    std::shared_ptr<rf::IPort> portOut;
    size_t steps;
};