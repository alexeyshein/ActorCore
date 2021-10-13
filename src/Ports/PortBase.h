#pragma once

#include "IPort.h"
#include "IMessage.h"
#include "MessagePublisherFunctor.hpp"
#include <memory>
#include <string>

namespace rf
{
  class Logger;
  class PortBase : public IPort
  {
  public:
    PortBase(std::string id, IUnit* parent = nullptr);
    virtual ~PortBase();
    std::string Id() override { return _id; }
    std::string Type() override {return  _type;}
    bool Init(const json&) override;
    json Configuration() override;
    json UserData() override { return userData; }
    bool SetUserData(const json& ud) override { userData = ud; return true;}
    IUnit* Parent() override {return _parent;}
    virtual std::vector<std::weak_ptr<IUnit>> Children() override {return std::vector<std::weak_ptr<IUnit>>();}
   
     // filters for the types of processed messages
    const std::set<uint16_t>& TypesMessages() const override   {return  typesMessages;}
    void SetTypesMessages(const std::set<uint16_t>&) override;


    json Links() override;
    std::variant<std::monostate, bool, int, double, std::string> GetProperty(const std::string&) override {return std::monostate{};};
    bool SetProperty(const std::string&, bool) override { return true; }
    bool SetProperty(const std::string&, int) override { return true; }
    bool SetProperty(const std::string&, double) override { return true; }
    bool SetProperty(const std::string&, std::string) override { return true; }
    bool SetProperties(const json&) override { return true; }

    void Attach( const  std::string& remotePortOwnerId, std::weak_ptr<IPort>& ptrRemotePort) override{}
	  void Detach( const  std::string& remotePortOwnerId, std::weak_ptr<IPort>& ptrRemotePort) override{}
    void Detach( const  std::string& remotePortOwnerId, const std::string& remotePortId)  override{}

	  void Notify(const std::shared_ptr<IMessage> &data) override{}
	  size_t NumObservers() override{return 0;}
	  void CleanObservers() override{}
    std::set<std::pair<std::string, std::string>>  IdentifiersOfNotifiable() override {return std::set<std::pair<std::string, std::string>>();}

    //for inputs specific
    void Receive(std::shared_ptr<IMessage> data) override{}
    void SetEveventOnReceive(std::function<void(std::string,std::shared_ptr<IMessage>)>)  override{}

  protected:
    IUnit* _parent;
    std::string _id;
    std::string _type;
    std::unique_ptr<Logger> logger;
    std::set<uint16_t> typesMessages;//
    json userData;
  };
}
