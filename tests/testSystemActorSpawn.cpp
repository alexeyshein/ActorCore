#include "catch.hpp"
#include <iostream>
// #include "SystemLocal.h"
#include "FabricActorForTest.hpp"
#include "SystemLocal.h"



TEST_CASE("Actors are Spawned  By System", "")
{
    std::set<std::string> myActorTypes{"ActorSimpleEventBasedForTest", "ActorSimpleBlockingForTest"};
    rf::SystemLocal system;
    system.RegisterFactory(myActorTypes, &FabricActorForTest::Create);

    SECTION("Register Fabric")
    {
        auto registered = system.GetRegisteredActorTypes();
        REQUIRE(myActorTypes == registered); //
    }

    SECTION("Spawn")
    {
        for(const auto &curType: myActorTypes)
        {
            auto actor  = system.Spawn(curType);
            REQUIRE(actor != nullptr); //
        }
        REQUIRE(system.countActors() == myActorTypes.size()); //

        REQUIRE(system.Spawn(std::string("zero")) == nullptr); 
        REQUIRE(system.countActors() == myActorTypes.size());
    }

    SECTION("Attach-Detach")
    {
        auto actor = std::shared_ptr<rf::IAbstractActor>(FabricActorForTest::Create(*myActorTypes.begin(),"123"));
        REQUIRE(actor != nullptr); //
       
        REQUIRE(system.countActors() == 0); //
        REQUIRE(system.Attach(actor));
        REQUIRE(system.countActors() == 1); //
        auto detached = system.Detach(actor->Id());
        REQUIRE(detached);
        REQUIRE(detached->Id() == actor->Id());
        REQUIRE(system.countActors() == 0); //
    }

}
