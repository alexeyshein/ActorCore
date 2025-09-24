#include "SystemLocal.h"

#include <iostream>
#include <unordered_set>

#include "IAbstractActor.h"
#include "ActorFactoryCollection.hpp"
#include "UidGenerator.hpp"
#include "Logger.h"
#include "PortOutput.h" //access for specific functions, like setLinkUserData

using nlohmann::json;
using rf::ActorCreatorFunction;
using rf::IUnit;
using rf::IAbstractActor;
using rf::SystemLocal;
using rf::Logger;


SystemLocal::SystemLocal(const std::string& loggerInitParam) :
    logger(new Logger())
    , userData(json::object())
{
  if(logger)
  {
     logger->Create(loggerInitParam, "Actor Scheme Trace channel", "Actor Scheme Telemetry channel");
     logger->Share("ActorSystem log client", "Actor System Trace channel", "Actor System Telemetry channel");
  }
}


SystemLocal::~SystemLocal()
{
    logger->client->Flush();
    Clear();
}


bool SystemLocal::Init(const nlohmann::json &scheme)
{
  Clear();
  // Init Actors
  //std::lock_guard<std::mutex> lock(mutexScheme);
  const auto  actorsJson = scheme.find("actors");
  bool res{ true };
  if (actorsJson == scheme.end())
   {
      logger->TRACE(0, TM("Actors Not Found"));
      return true;
   }
  for (const auto &actorJson : *actorsJson)
  {
    auto actor = Spawn(actorJson);
    if (!actor.lock())
    {
      logger->WARNING(0, TM("Actor wasn`t spawned:%s"),actorJson.dump().c_str());
      //Clear();
      res = false;
    }
    else
    {
        actor.lock()->SetParent(this);
    }
  }
  // Init Links
  auto const linksJson = scheme.find("links");
  if (linksJson == scheme.end())
   {
      logger->TRACE(0, TM("Links Not Found"));
  }
  else
  {
      for (const auto& linkJson : *linksJson)
      {
          if (!Connect(linkJson))
          {
              logger->WARNING(0, TM("Connection problem %s"), linkJson.dump().c_str());
              res = false;
          }
      }
  }
  if (scheme.contains("userData"))
  {
      userData = scheme.at("userData");
  }
  //if(logger->trace)
  if (res)
  {
      this->Activate();
      logger->INFO(0, TM("Actor System was Init successfully %d"), 0);
  }
  return res;
}

//TODO realize as Init without clear
bool SystemLocal::Append(const nlohmann::json& scheme)
{
    const auto  actorsJson = scheme.find("actors");
    bool res{ true };
    if (actorsJson != scheme.end())
    {
        for (const auto& actorJson : *actorsJson)
        {
            auto actor = Spawn(actorJson);
            if (!actor.lock())
            {
                logger->WARNING(0, TM("Actor wasn`t spawned:%s"), actorJson.dump().c_str());
                //Clear();
                res = false;
            }
            else
            {
                actor.lock()->SetParent(this);
            }
        }
    }

    // Init Links
    auto const linksJson = scheme.find("links");
    if (linksJson == scheme.end())
    {
        logger->TRACE(0, TM("Links Not Found"));
    }
    else
    {
        for (const auto& linkJson : *linksJson)
        {
            if (!Connect(linkJson))
            {
                logger->WARNING(0, TM("Connection problem %s"), linkJson.dump().c_str());
                res = false;
            }
        }
    }
    if (scheme.contains("userData"))
    {
        userData.merge_patch(scheme.at("userData"));
    }
    //if(logger->trace)
    if (res)
    {
        this->Activate();
        logger->INFO(0, TM("Actor System was Append successfully %d"), 0);
    }
    return res;
}

json SystemLocal::Scheme() const
{
    json jsonScheme{};
    auto jsonActors = json::array();
    auto jsonLinks = json::array();
    //std::lock_guard<std::mutex> lock(mutexScheme);
    std::shared_lock lock(mutexScheme);
    for (const auto& [actorId, actor] : _mapActors)
    {
        jsonActors.emplace_back(actor->Configuration());
        auto links = actor->Links();
        //jsonConnections+=connections;
        for (const auto& link : links)
        {
            jsonLinks.emplace_back(link);
        }

    }
    jsonScheme["actors"] = std::move(jsonActors);
    jsonScheme["links"] = std::move(jsonLinks);
    jsonScheme["userData"] = userData;
    return jsonScheme;
}


json SystemLocal::Links() const
{
    auto jsonLinks = json::array();
    //std::lock_guard<std::mutex> lock(mutexScheme);
    std::shared_lock lock(mutexScheme);
    for (const auto& [actorId, actor] : _mapActors)
    {

        auto links = actor->Links();
        for (const auto& link : links)
        {
            jsonLinks.emplace_back(link);
        }
    }
    return jsonLinks;
}

void SystemLocal::Clear()
{
  Deactivate();
  {
      std::shared_lock lock(mutexScheme);
      for (auto it = _mapActors.begin(); it != _mapActors.end();it++)
      {
          auto actor = it->second;
          this->RemoveAllConectionsWithActor(actor);
      }
  }
 
  std::scoped_lock lock(mutexScheme);
  for (auto it = _mapActors.begin(); it != _mapActors.end();)
  {
    auto actor = it->second;
    actor->SetParent(nullptr);
    it = _mapActors.erase(it);
  }
  userData = json::object();
}


//by json
std::weak_ptr<IAbstractActor> SystemLocal::Spawn(json jsonActor)
{
  std::shared_ptr<IAbstractActor> actorPtr = nullptr;

  try
  {
    std::string typeName = jsonActor["type"].get<std::string>();
    
    std::string id = UidGenerator::Generate(typeName);
    if (jsonActor.contains("id"))
        if (jsonActor.at("id").is_string())
             id = jsonActor["id"].get<std::string>();

    actorPtr =  std::shared_ptr<IAbstractActor>(ActorFactoryCollection::Create(typeName, id));
    if(!actorPtr)
      return actorPtr;
    if (Attach(actorPtr))
    {
        actorPtr->Init(jsonActor);
    }
    else
    {
        actorPtr = nullptr;
    }
    
  }
  catch (...)
  {
      logger->TRACE(0, TM("Spawn error %s"), jsonActor);
  }
  return actorPtr;
}


//by name
std::weak_ptr<IAbstractActor> SystemLocal::Spawn(std::string typeName)
{
  std::string id = UidGenerator::Generate(typeName);
  std::shared_ptr<IAbstractActor> actorPtr(ActorFactoryCollection::Create(typeName, id));
  if (actorPtr)
  {
      if (!Attach(actorPtr))
      {
          actorPtr = nullptr;
      }
  }
    
  return actorPtr;
}


std::weak_ptr<IAbstractActor> SystemLocal::Clone(const std::string& id, bool withLinks) //clone one by id
{
    auto result = std::weak_ptr<IAbstractActor>();
    auto actor = this->GetActorById(id).lock();
    if (!actor)
        return result;
    auto actorNew = this->Spawn(actor->Type()).lock();
    if (!actorNew)
        return result;
    actorNew->Init(actor->Configuration());
    result = actorNew;
    if (withLinks)
    {
        auto links = Links();
        for (auto& link : links)
        {
            if (link.contains("idActorSrc"))
                if (link.at("idActorSrc").is_string())
                    if (link.at("idActorSrc").get<std::string>() == id)
                        link["idActorSrc"] = actorNew->Id();
            if (link.contains("idActorDst"))
                if (link.at("idActorDst").is_string())
                    if (link.at("idActorDst").get<std::string>() == id)
                        link["idActorDst"] = actorNew->Id();
            this->Connect(link);
        }
    }
    return result;
}

json SystemLocal::Clone(const std::vector<std::string>& ids, bool withLinks) //clone one by id
{
    std::map<std::string, std::string> idMap;
    for (const auto &id : ids)
    {
        auto actor = Clone(id, false).lock();
        if (actor)
            idMap[id] = actor->Id();
    }
    if (withLinks)
    {
        auto links = Links();
        for (auto& link : links)
        {
            bool found{ false };
            if (link.contains("idActorSrc"))
                if (link.at("idActorSrc").is_string())
                {
                    std::string  id = link.at("idActorSrc").get<std::string>();
                    if (idMap.count(id) > 0)
                    {
                        link["idActorSrc"] = idMap[id];
                        found = true;
                    }
                        
                }

            if (link.contains("idActorDst"))
                if (link.at("idActorDst").is_string())
                {
                    std::string  id = link.at("idActorDst").get<std::string>();
                    if (idMap.count(id) > 0)
                    {
                        link["idActorDst"] = idMap[id];
                        found = true;
                    } 
                }
            if(found)
                this->Connect(link);
        }
    }
    return idMap;
}

bool SystemLocal::Attach(std::shared_ptr<IAbstractActor> actorPtr)
{
 // std::lock_guard<std::mutex> lock(mutexScheme);
   std::scoped_lock lock(mutexScheme);
  if (_mapActors.count(actorPtr->Id()) != 0)
    return false;
  _mapActors.emplace(std::make_pair(actorPtr->Id(), actorPtr));
  actorPtr->SetParent(this);
  return true;
}


std::shared_ptr<IAbstractActor> SystemLocal::Detach(std::string id)
{
  std::shared_ptr<IAbstractActor> actor{nullptr};
  {
      std::scoped_lock lock(mutexScheme);
      auto it = _mapActors.find(id);
      if (it != _mapActors.end())
      {
          actor = it->second;
          it = _mapActors.erase(it);
      }
  }
  //separate to avoid deadlock in RemoveAllConectionsWithActor
  if (actor)
  {
      this->RemoveAllConectionsWithActor(actor);
      actor->SetParent(nullptr);   
  }
  return actor;
}


//spawn copy of existing
std::weak_ptr<IAbstractActor> SystemLocal::GetActorById(std::string id)
{
  std::shared_lock lock(mutexScheme);
  auto it = _mapActors.find(id);
  if (it != _mapActors.end())
  {
    return it->second;
  }
  return std::weak_ptr<IAbstractActor>();
}

std::weak_ptr<IAbstractActor> SystemLocal::GetActorByLabel(std::string label)
{
    std::shared_lock lock(mutexScheme);
    for (const auto& pair : _mapActors) {
        const auto& actor = pair.second; // ѕолучаем указатель на IAbstractActor
        if (actor && actor->Label() == label) {
            return actor; 
        }
    }
    return std::weak_ptr<IAbstractActor>();
}

void SystemLocal::RegisterFactory(std::set<std::string> keySet, ActorCreatorFunction functor)
{
  //FactoryAllActors::Register("key", funcor);
  std::for_each(keySet.begin(), keySet.end(),
                [&](auto const &key) { ActorFactoryCollection::Register(key, functor); });
}


std::set<std::string> SystemLocal::GetRegisteredActorTypes()
{
  return ActorFactoryCollection::getRegisteredTypes();
}


bool SystemLocal::Connect(std::string idActor1, std::string idPortActor1, std::string idActor2, std::string idPortActor2)
{
  auto actor1 = GetActorById(idActor1);
  if (!actor1.lock())
    return false;

  auto actor2 = GetActorById(idActor2).lock();
  if (!actor2)
    return false;

  return actor2->ConnectTo(actor1, idPortActor1, idPortActor2);
}


bool SystemLocal::Connect(json connection)
{
  bool res{ false };
  try
  {
    std::string idActor1 = connection["idActorSrc"].get<std::string>();
    std::string idPortActor1 = connection["idPortSrc"].get<std::string>();
    std::string idActor2 = connection["idActorDst"].get<std::string>();
    std::string idPortActor2 = connection["idPortDst"].get<std::string>();
    res =  Connect(idActor1, idPortActor1, idActor2, idPortActor2);
    if (res && connection.contains("userData"))
    {
        auto&  userData = connection.at("userData");
        SetLinkUserData(idActor1, idPortActor1, idActor2, idPortActor2, userData);
    }
  }
  catch (...)
  {
  }
  return res;
}



//TODO 
void SystemLocal::Disconnect(std::string idActor1, std::string idPortActor1, std::string idActor2, std::string idPortActor2)
{
  auto actor1 = GetActorById(idActor1).lock();
  auto actor2 = GetActorById(idActor2).lock();
  if (actor1 && actor2)
  {
    auto port2 = actor2->GetPortById(idPortActor2);
    if (port2.lock())
      actor1->Disconnect(idActor2, port2, idPortActor1);

    //TODO проверить почему port1 беретс€ с actor2
    auto port1 = actor2->GetPortById(idPortActor2);
    if (port1.lock())
      actor2->Disconnect(idActor1, port1, idPortActor2);
  }
}


void SystemLocal::Disconnect(json connection)
{
  try
  {
    std::string idActor1 = connection["idActorSrc"].get<std::string>();
    std::string idPortActor1 = connection["idPortSrc"].get<std::string>();
    std::string idActor2 = connection["idActorDst"].get<std::string>();
    std::string idPortActor2 = connection["idPortDst"].get<std::string>();
    Disconnect(idActor1, idPortActor1, idActor2, idPortActor2);
  }
  catch (...)
  {
  }
}


json SystemLocal::CreateMemento() const
{
    return this->Scheme();
}

// Build a unique key for the link: "srcActor|srcPort|dstActor|dstPort"
static inline std::string makeKey(const json& l)
{
    return std::string(l["idActorSrc"]) + "|" +
        std::string(l["idPortSrc"]) + "|" +
        std::string(l["idActorDst"]) + "|" +
        std::string(l["idPortDst"]);
}

void SystemLocal::SetMemento(const json& schemeSnapshot)
{
    // Actualize Actors
    // 
    const auto  actorsJson = schemeSnapshot.find("actors");
    std::set<std::string> updated;
    if (actorsJson != schemeSnapshot.end() && actorsJson->is_array())
    {
        for (const auto& actorJson : *actorsJson)
        {
            if (actorJson.contains("id"))
                if (actorJson.at("id").is_string())
                {
                    std::string id = actorJson["id"].get<std::string>();
                    auto actor = this->GetActorById(id).lock();
                    if (actor)
                    {
                        // update existing Actor
                        if (actorJson.contains("properties"))
                        {
                            actor->SetProperties(actorJson.at("properties"));
                        }
                        if (actorJson.contains("userData"))
                            actor->SetUserData(actorJson.at("userData"));
                        else
                            actor->SetUserData(json::object());
                    }
                    else
                    {
                        // add new Actor
                        this->Spawn(actorJson);
                    }
                    updated.insert(id);
                }    
        }
    }
    //Remove actors not present in the current scheme snapshot
    {
        //separate to avoid deadlock in RemoveAllConectionsWithActor
        { //first - remove all connections for removing actors
            std::shared_lock lock(mutexScheme);
            for (auto it = _mapActors.begin(); it != _mapActors.end(); ++it)
            {
                auto id = it->first;
                if (updated.count(id) < 1)
                {
                    //Detach 
                    this->RemoveAllConectionsWithActor(it->second);
                }
            }
        }
        //second - remove actors
        std::scoped_lock lock(mutexScheme);
        for (auto it = _mapActors.begin(); it != _mapActors.end();)
        {
            auto id = it->first;
            if (updated.count(id) < 1)
            {
                it = _mapActors.erase(it);
            }
            else {
                ++it;  // manually increment if not erasing
            }
        }
    }


    // Actualize Links
    //
    json linksCurJson = Links();
    std::unordered_set<std::string> newKeys;
    const auto  linksNewJson = schemeSnapshot.find("links");
    if (linksNewJson != schemeSnapshot.end() && linksNewJson->is_array())
    {
        
        std::unordered_map<std::string, json> curMap;
        for (const auto& l : linksCurJson)
            curMap[makeKey(l)] = l;
        // 2. Iterate over new links
        for (const auto& newLink : *linksNewJson)
        {
            const std::string k = makeKey(newLink);
            newKeys.insert(k);
            auto it = curMap.find(k);
            if (it == curMap.end())
            {
                Connect(newLink);               // new link
            }
            else
            {
                json newUserData{ json::object() };
                json curUserData{ json::object() };
                if (it->second.contains("userData"))
                    curUserData = it->second.at("userData");
                if (newLink.contains("userData"))
                    newUserData = newLink.at("userData");
                if (newUserData != curUserData)
                    SetLinkUserData(newLink);   // userData changed
            }
        }
    }
    // 3. Iterate over current links and remove any that are missing in the new set
    for (const auto& oldLink : linksCurJson)
    {
        if (newKeys.find(makeKey(oldLink)) == newKeys.end())
            Disconnect(oldLink);
    }

    // Actualize userData
    //
    if (schemeSnapshot.contains("userData"))
    {
        userData = schemeSnapshot.at("userData");
    }
    else
    {
        userData = json::object();
    }

    logger->INFO(0, TM("Actor System Restored successfully %d"), 0);
    return ;
}


void SystemLocal::RemoveAllConectionsWithActor(std::weak_ptr<IAbstractActor> ptrWeakActorTarget)
{
  auto actorTarget = ptrWeakActorTarget.lock();
  if(!actorTarget)
     return;
  auto actorPorts = actorTarget->GetPorts();
  std::shared_lock lock(mutexScheme);
  for (auto actorPort : actorPorts)
  {
    actorPort.lock()->CleanObservers();
    for (auto [actorId, actor] : _mapActors)
    {
      actor->DisconnectAll(actorTarget->Id(),actorPort.lock()->Id());
    }
  }
}


bool SystemLocal::SetLinkUserData(std::string idActorSrc, std::string idPortSrc, std::string idActorDst, std::string idPortDst, json userData)
{
    bool res{ false };
    auto actor = GetActorById(idActorSrc).lock();
    if (actor)
    {
        auto port = actor->GetPortById(idPortSrc).lock();
        if (port)
        {
            if (PortOutput* portOut = dynamic_cast<PortOutput*>(port.get())) {
                portOut->SetLinkUserData(idActorDst, idPortDst, userData);
            }
        }
    }
    return res;
}

bool SystemLocal::SetLinkUserData(json linkJson)
{
    try
    {
        std::string idActorSrc = linkJson["idActorSrc"].get<std::string>();
        std::string idPortSrc = linkJson["idPortSrc"].get<std::string>();
        std::string idActorDst = linkJson["idActorDst"].get<std::string>();
        std::string idPortDst = linkJson["idPortDst"].get<std::string>();
        json userData = linkJson["userData"];
        return this->SetLinkUserData(idActorSrc, idPortSrc, idActorDst, idPortDst, userData);
    }
    catch (...) {}
    return false;
}

json SystemLocal::GetLinkUserData(std::string idActorSrc, std::string idPortSrc, std::string idActorDst, std::string idPortDst )
{
    json userData;
    auto actor = GetActorById(idActorSrc).lock();
    if (actor)
    {
        auto port = actor->GetPortById(idPortSrc).lock();
        if (port)
        {
            if (PortOutput* portOut = dynamic_cast<PortOutput*>(port.get())) {
                userData = portOut->GetLinkUserData(idActorDst, idPortDst);
            }
        }
    }
    return userData;
}

json SystemLocal::GetLinkUserData(json linkJson)
{
    try
    {
        std::string idActorSrc = linkJson["idActorSrc"].get<std::string>();
        std::string idPortSrc = linkJson["idPortSrc"].get<std::string>();
        std::string idActorDst = linkJson["idActorDst"].get<std::string>();
        std::string idPortDst = linkJson["idPortDst"].get<std::string>();
        json userData = linkJson["userData"];
        return this->GetLinkUserData(idActorSrc, idPortSrc, idActorDst, idPortDst);
    }
    catch (...) {}
    return json();
}


void SystemLocal::Activate()
{
  std::shared_lock lock(mutexScheme);
  std::for_each(_mapActors.cbegin(), _mapActors.cend(),[](auto & recInMap){ recInMap.second->Activate(); });
}


void SystemLocal::Deactivate()
{
    std::shared_lock lock(mutexScheme);
    std::for_each(_mapActors.cbegin(), _mapActors.cend(),[](auto & recInMap){ recInMap.second->Deactivate(); });
}

std::map<std::string, bool> SystemLocal::ActorsActivationState()
{
    std::map<std::string, bool> activation;
    std::shared_lock lock(mutexScheme);
    for (auto [actorId, actor] : _mapActors)
    {
        activation[actorId] = actor->IsActive();
    }
    return activation;
}


std::vector<std::weak_ptr<IUnit>> SystemLocal::Children() 
{
  std::vector<std::weak_ptr<IUnit>> children;
  std::shared_lock lock(mutexScheme);
  std::for_each(_mapActors.cbegin(), _mapActors.cend(),[&children](auto & recInMap){children.emplace_back(recInMap.second); });
  return children;
}

