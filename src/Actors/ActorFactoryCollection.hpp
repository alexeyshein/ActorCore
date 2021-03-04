#pragma once

/**
 * @file ActorFactoryCollection.hpp
 * @author your name (you@domain.com)
 * @brief Implement extensible factories for Actors  =>collect std::function<IAbstractActor *(const std::string &, int id)>
 * @version 0.1
 * @date 2021-02-26
 * 
 * @copyright Copyright (c) 2021 
 * 
 */
#include "IAbstractActor.h"
#include <map>
namespace rf
{
    using ActorCreatorFunction = std::function<IAbstractActor *(const std::string &, const std::string &)>;
    class ActorFactoryCollection
    {
    public:
        //typedef _Base* (*CreatorFunction) (const _Key&);

        typedef std::map<std::string, ActorCreatorFunction> _mapFactory;

        // called statically by all classes that can be created
        static const std::string Register(const std::string idKey, ActorCreatorFunction classCreator)
        {
            get_mapFactory()->insert(std::pair<std::string, ActorCreatorFunction>(idKey, classCreator));
            return idKey;
        }

        // Tries to create instance based on the key
        static IAbstractActor *Create(const std::string& idKey, const std::string & id)
        {
            auto it = get_mapFactory()->find(idKey);
            if (it != get_mapFactory()->end())
            {
                if (it->second)
                {
                    auto func = it->second;
                    return func(idKey,id); //functor execute
                }
            }
            return 0;
        }

        static std::set<std::string> getRegisteredTypes()
        {
            std::set<std::string> keysSet;
            std::transform(get_mapFactory()->begin(), get_mapFactory()->end(),
                std::inserter(keysSet, keysSet.end()),
                [](auto pair){ return pair.first; });
            return keysSet;
        }

    protected:
        static _mapFactory *get_mapFactory()
        {
            static _mapFactory m_sMapFactory;
            return &m_sMapFactory;
        }
    };
}