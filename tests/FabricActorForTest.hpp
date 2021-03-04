 #pragma once
 #include "ActorSimpleEventBasedForTest.hpp"
 #include "ActorSimpleBlockingForTest.hpp"


class FabricActorForTest
{
public:
    static rf::IAbstractActor* Create(std::string typeActor, std::string uid)
    {
        std::cout << "Start Fabric " << typeActor <<" : " <<uid<<'\n';
        rf::IAbstractActor *actor{nullptr};
        if (typeActor == std::string("ActorSimpleEventBasedForTest"))
        {
            actor = new ActorSimpleEventBasedForTest(uid);
        }
        else if (typeActor == std::string("ActorSimpleBlockingForTest") )
        {
            actor = new ActorSimpleBlockingForTest(uid);
        }
        std::cout << "Fabric create " << typeid(actor).name() << '\n';
        return actor;
    }
};