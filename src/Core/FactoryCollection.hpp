#pragma once

/**
 * @file FactoryCollection.hpp
 * @author your name (you@domain.com)
 * @brief Implement extensible factories in C++
 * @version 0.1
 * @date 2021-02-25
 * 
 * @copyright Copyright (c) 2021 
 * 
 * Idea from https://stackoverflow.com/questions/17378961/elegant-way-to-implement-extensible-factories-in-c
 * if you have many factories or methods  to produce Base* object
 */

 #include <map>
template <typename _Key, typename _Base, typename _Pred = std::less<_Key> >

class FactoryCollection {
public:
    //typedef _Base* (*CreatorFunction) (const _Key&);
    using CreatorFunction = std::function<_Base*(const _Key&)>;
    typedef std::map<_Key, CreatorFunction, _Pred> _mapFactory;

    // called statically by all classes that can be created
    static _Key Register(_Key idKey, CreatorFunction classCreator) {
        get_mapFactory()->insert(std::pair<_Key, CreatorFunction>(idKey, classCreator));
        return idKey;
    }

    // Tries to create instance based on the key
    static _Base* Create(_Key idKey) {
        auto it = get_mapFactory()->find(idKey);
        if (it != get_mapFactory()->end()) {
            if (it->second) {
                auto func = it->second;
                return func(idKey); //functor execute
            }
        }
        return 0;
    }

protected:
    static _mapFactory * get_mapFactory() {
        static _mapFactory m_sMapFactory;
        return &m_sMapFactory;
    }
};