#include "catch.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include "FabricActorForTest.hpp"
#include "PortOutput.h"
#include "SystemLocal.h"

TEST_CASE("Actors Interchange", "test")
{
    std::set<std::string> myActorTypes{"ActorSimpleEventBasedForTest", "ActorSimpleBlockingForTest"};
    rf::SystemLocal system;
    system.RegisterFactory(myActorTypes, &FabricActorForTest::Create);
    auto registered = system.GetRegisteredActorTypes();
    REQUIRE(myActorTypes == registered); //

    auto actorEvent = std::dynamic_pointer_cast<ActorSimpleEventBasedForTest>(system.Spawn(std::string("ActorSimpleEventBasedForTest")).lock());
    REQUIRE(actorEvent != nullptr); //
    auto actorBlock = std::dynamic_pointer_cast<ActorSimpleBlockingForTest>(system.Spawn(std::string("ActorSimpleBlockingForTest")).lock());
    REQUIRE(actorBlock != nullptr); //

    SECTION("actorBlock (async out)-> actorEvent (active) messages")
    {
        auto connected = system.Connect(actorBlock->Id(), actorBlock->portOut->Id(), actorEvent->Id(), actorEvent->portInput->Id());
        REQUIRE(connected == true);
        actorEvent->Activate();
        actorBlock->Activate();
        std::this_thread::sleep_for(std::chrono::seconds(1));
        actorBlock->Deactivate();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        actorEvent->Deactivate();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        auto actorBlockStepsAfterStop1 = actorBlock->steps;
        auto actorEventStepsAfterStop1 = actorEvent->steps;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        REQUIRE(actorBlock->steps == actorBlockStepsAfterStop1);  // Проверка что все остановилось
        REQUIRE(actorEvent->steps == actorEventStepsAfterStop1);  // Проверка что все остановилось
        REQUIRE(actorBlock->steps > 0);
        REQUIRE(0.9* actorBlock->steps < actorEvent->steps); // Проверка что принято столько-же сколько и отправлено
    }

    SECTION("actorBlock (sync out)-> actorEvent (active) messages")
    {
        std::dynamic_pointer_cast<rf::PortOutput>(actorBlock->portOut)->SetAsyncMode(false);
        auto connected = system.Connect(actorBlock->Id(), actorBlock->portOut->Id(), actorEvent->Id(), actorEvent->portInput->Id());
        REQUIRE(connected == true);
        actorEvent->Activate();
        actorBlock->Activate();
        std::this_thread::sleep_for(std::chrono::seconds(1));
        actorBlock->Deactivate();
        actorEvent->Deactivate();
        REQUIRE(actorBlock->steps > 0);
        REQUIRE(actorEvent->steps == actorEvent->steps); // Проверка что принято столько-же сколько и отправлено
    }
    
    SECTION("actorBlock (async out) -> actorEvent (inactive) messages")
    {
        auto connected = system.Connect(actorBlock->Id(), actorBlock->portOut->Id(), actorEvent->Id(), actorEvent->portInput->Id());
        REQUIRE(connected == true);
        actorEvent->Deactivate();
        actorBlock->Activate();
        std::this_thread::sleep_for(std::chrono::seconds(1));
        actorBlock->Deactivate();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        REQUIRE(actorBlock->steps > 0); // Проверка что принято столько-же сколько и отправлено
        REQUIRE(actorEvent->steps == 0);
    }


        SECTION("Delete actorEvent => actorBlock (async out)-> actorEvent (active)")
    {
        auto connected = system.Connect(actorBlock->Id(), actorBlock->portOut->Id(), actorEvent->Id(), actorEvent->portInput->Id());
        REQUIRE(connected == true);
        actorEvent->Activate();
        actorBlock->Activate();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        system.Detach(actorEvent->Id());
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        actorBlock->Deactivate();
        actorEvent->Deactivate();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        REQUIRE(actorBlock->steps > 0);
        REQUIRE(actorBlock->steps >= actorEvent->steps); // Проверка что принято столько-же сколько и отправлено
    }

    SECTION("Delete actorEvent =>  actorBlock (sync out)-> actorEvent (active) messages")
    {
        std::dynamic_pointer_cast<rf::PortOutput>(actorBlock->portOut)->SetAsyncMode(false);
        auto connected = system.Connect(actorBlock->Id(), actorBlock->portOut->Id(), actorEvent->Id(), actorEvent->portInput->Id());
        REQUIRE(connected == true);
        actorEvent->Activate();
        actorBlock->Activate();
        std::this_thread::sleep_for(std::chrono::seconds(1));
        system.Detach(actorEvent->Id());
        std::this_thread::sleep_for(std::chrono::seconds(1));
        actorBlock->Deactivate();
        actorEvent->Deactivate();
        REQUIRE(actorBlock->steps > 0);
        REQUIRE(actorBlock->steps > 1.4*actorEvent->steps); // После удаления актора он должен перестать принимать
    }
}
