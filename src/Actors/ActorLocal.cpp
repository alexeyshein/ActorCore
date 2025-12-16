
#include "ActorLocal.h"
#include "UidGenerator.hpp"
#include "PortFactory.h"
#include "Logger.h"
#include "PortOutput.h" //access for specific functions, like setLinkUserData

using rf::ActorLocal;
using rf::IPort;
using rf::Logger;

using nlohmann::json;

ActorLocal::ActorLocal(const std::string& id, IUnit* parent) :_parent(parent), _id(id), _type("ActorLocal"), logger(new Logger()), _flagActive(true)
{
	//logger->ConnectToShare(L"ActorSystem log client", L"Actor System Trace channel", L"Actor System Telemetry channel");
	logger->ConnectToShare("ActorSystem log client", "Actor System Trace channel", "Actor System Telemetry channel");
	label = id;
	description = "";
}

ActorLocal::~ActorLocal()
{
}

bool ActorLocal::Init(const json& actorConfig)
{
	logger->TRACE(0, TM("%s start init"), Id().c_str());

	if (actorConfig.contains("properties"))
	{
		this->SetProperties(actorConfig.at("properties"));
	}
	if (actorConfig.contains("userData"))
		userData = actorConfig.at("userData");
	logger->DEBUG(0, TM("%s initialized"), Id().c_str());

	return true;
}



json ActorLocal::Configuration()
{
	auto portJson = json::array();
	std::shared_lock lock(mtx_mapPort);
	for (const auto& [portIdInternal, portInternal] : _mapPorts)
	{
		portJson.emplace_back(portInternal->Configuration());
	}
	json properties = json({ {"label", label},
			{"description", description} });

	return json{
		{"id", _id},
		{"type", _type},
		{"label", label},
		{"properties",properties},
		{"ports", portJson},
		{"userData", userData},
		{"userData", {}},
	};
}

bool ActorLocal::SetProperties(const json& properties)
{
	if (properties.contains("label"))
		if (properties.at("label").is_string())
			label = properties.at("label").get<std::string>();

	if (properties.contains("description"))
		if (properties.at("description").is_string())
			description = properties.at("description").get<std::string>();

	
	return true;
}

bool ActorLocal::SetUserData(const json& newUserData)
{
	userData = newUserData;
	return true;
}


json ActorLocal::Links()
{
	json connections = json::array();
	std::shared_lock lock(mtx_mapPort);
	for (const auto& [portIdInternal, portInternal] : _mapPorts)
	{
		auto mapExternals = portInternal->IdentifiersOfNotifiable();
		for (const auto& [actorIdExternal, portIdExternal] : mapExternals)
		{

			//connections.emplace_back(json::array({_id, portIdInternal, actorIdExternal, portIdExternal}));
			json link{ {"idActorSrc", _id}, {"idPortSrc", portIdInternal}, {"idActorDst",actorIdExternal}, {"idPortDst",portIdExternal} };
			if (rf::PortOutput* portOut = dynamic_cast<rf::PortOutput*>(portInternal.get()))
			{
				json userData = portOut->GetLinkUserData(actorIdExternal, portIdExternal);
				if (!userData.empty())
				{
					link["userData"] = userData;
				}
			}		
			connections.emplace_back(link);
		}
	}
	return connections;
}

std::vector<std::weak_ptr<IPort>> ActorLocal::GetPorts()
{
	std::vector<std::weak_ptr<IPort>> ports;
	std::shared_lock lock(mtx_mapPort);
	for (const auto& [portIdInternal, portInternal] : _mapPorts)
		ports.push_back(portInternal);
	return ports;
}


std::set<std::string> ActorLocal::GetPortsIdSet()
{
	std::set<std::string> ports;
	std::shared_lock lock(mtx_mapPort);
	for (const auto& [portIdInternal, portInternal] : _mapPorts)
		ports.insert(portIdInternal);
	return ports;
}

std::variant<std::monostate, bool, int, double, std::string> ActorLocal::GetProperty(const std::string& propertyPath)
{
	auto jsonConf = this->Configuration();
	try
	{
		auto properties = jsonConf["properties"];
		auto valueOpt = GetJsonValueFromJson(properties, propertyPath); // Добавлена точка с запятой

		// Проверяем, что значение найдено
		if (!valueOpt.has_value()) {
			logger->WARNING(0, TM("Property '%s' not found for actor %s"),
				propertyPath.c_str(), Id().c_str());
			return std::monostate{};
		}

		nlohmann::json* jsonValue = valueOpt.value();

		if (jsonValue == nullptr) {
			return std::monostate{};
		}

		// Определяем тип и возвращаем соответствующее значение
		if (jsonValue->is_boolean()) {
			return jsonValue->get<bool>();
		}
		else if (jsonValue->is_number_integer()) {
			return jsonValue->get<int>();
		}
		else if (jsonValue->is_number_float()) {
			return jsonValue->get<double>();
		}
		else if (jsonValue->is_string()) {
			return jsonValue->get<std::string>();
		}
		else {
			// null, object, array - неподдерживаемые типы
			logger->WARNING(0, TM("Unsupported property type for '%s'"),
				propertyPath.c_str());
			return std::monostate{};
		}
	}
	catch (const std::exception& e)
	{
		logger->WARNING(0, TM("GetProperty error for %s: %s"),
			Id().c_str(), e.what());
		return std::monostate{};
	}
}


json ActorLocal::GetStatus()
{
	json res;
	res["id"] = _id;
	// res["Recived frames"] = recivedFramesByPeriod;
	// recivedFramesByPeriod = 0;
	// res["Processed frames"] = processedFramesByPeriod;
	// processedFramesByPeriod = 0;

	return res;
}

std::shared_ptr<IPort> ActorLocal::addPort(const std::string& typePort, const std::string& portId)
{
	std::shared_ptr<IPort> portPtr = rf::PortFactory::Create(typePort, portId, this);
	if (portPtr)
	{
		//portPtr->SetEveventOnReceive(std::bind(&ActorLocal::OnInputReceive, this, std::placeholders::_1, std::placeholders::_2));
		// std::function<void(std::string,std::shared_ptr<IMessage>)> functionOnRecive;
		portPtr->SetEventOnReceive([&](std::string idPort, std::shared_ptr<IMessage> ptrData) {
			if (this->_flagActive)
				this->OnInputReceive(idPort, ptrData);
			});
		std::scoped_lock lock(mtx_mapPort);
		_mapPorts.emplace(std::make_pair(portPtr->Id(), portPtr));
	}
	return portPtr;
}

std::shared_ptr<IPort> ActorLocal::addPort(const std::string& typePort)
{
	std::string portId = rf::UidGenerator::Generate(typePort);
	return addPort(typePort, portId);
}

std::shared_ptr<IPort> ActorLocal::addPort(const json& portJson)
{
	std::shared_ptr<IPort> port = nullptr;
	try
	{
		std::string typePort = portJson.at("type").get<std::string>();
		std::string portId = portJson.at("id").get<std::string>();

		port = addPort(typePort, portId);
		if (port)
			port->Init(portJson);
	}
	catch (...)
	{
	}
	return port;
}


void ActorLocal::deletePort(const std::string& portId)
{
	
	{  // If there is no such port, exit
		std::scoped_lock lock(mtx_mapPort);
		if (_mapPorts.count(portId)<1)
			return;
	}
	// 1. For all actors in the scheme, remove the connection to the current port (For input ports)
	auto* parent = this->Parent();
	if (parent)
	{
		auto children = parent->Children(); //все акторы
		for (auto& child : children)
		{
			std::shared_ptr<IUnit> unit = child.lock();
			if (unit)
			{
				IAbstractActor* actor = dynamic_cast<IAbstractActor*>(unit.get());
				if (actor)
				{
					actor->DisconnectAll(Id(), portId); //для всех акторов схемы вызываем удаление связи с текущим портом
				}
			}
		}
	}

	std::scoped_lock lock(mtx_mapPort);

	auto portIt = _mapPorts.find(portId);
	if (portIt == _mapPorts.end())
		return;

	auto port = portIt->second;
	// 2.  For the OUT port, remove the observers
	port->CleanObservers(); 
	// 3. Remove the port itself
	_mapPorts.erase(portIt);
}


bool ActorLocal::ConnectTo(const std::string& actorIdExternal, std::weak_ptr<IPort>& portExternalWeakPtr, const std::string& portIdInternal)
{
	auto portInternalWeakPtr = GetPortById(portIdInternal);
	auto portInternal = portInternalWeakPtr.lock();
	if (!portInternal)
		return false;
	// можно не проверять тип порта, т.к. для входа Attach ничего не делает
	// а лучше вызвать для входа и выхода сразу
	auto portExternal = portExternalWeakPtr.lock();
	if (!portExternal)
		return false;
	portExternal->Attach(_id, portInternalWeakPtr);
	portInternal->Attach(actorIdExternal, portExternalWeakPtr);
	return true;
}

bool ActorLocal::ConnectTo(std::weak_ptr<IAbstractActor>& actorExternalWeakPtr, const std::string& portIdExternal, const std::string& portIdInternal)
{
	auto actorExternal = actorExternalWeakPtr.lock();
	if (!actorExternal)
		return false;
	auto portExternal = actorExternal->GetPortById(portIdExternal);
	if (!portExternal.lock())
		return false;

	return ConnectTo(actorExternal->Id(), portExternal, portIdInternal);
}

void ActorLocal::Disconnect(const std::string& actorIdExternal, std::weak_ptr<IPort>& portExternalWeakPtr, const std::string& portIdInternal)
{
	auto portInternalWeakPtr = GetPortById(portIdInternal);
	auto portInternal = portInternalWeakPtr.lock();
	if (!portInternal)
		return;
	// можно не проверять тип порта, т.к. для входа Attach ничего не делает
	portInternal->Detach(actorIdExternal, portExternalWeakPtr);
	auto portExternal = portExternalWeakPtr.lock();
	if (!portExternal)
		return;
	portExternal->Detach(_id, portInternalWeakPtr);
}

void ActorLocal::Disconnect(const std::string& actorIdExternal, const std::string& portIdExternal, const std::string& portIdInternal)
{
	auto portInternal = GetPortById(portIdInternal).lock();
	if (portInternal)
		portInternal->Detach(actorIdExternal, portIdExternal);
}

void ActorLocal::DisconnectAll(const std::string& actorIdExternal, const std::string& portIdExternal)
{
	std::shared_lock lock(mtx_mapPort);
	for (const auto& [portIdInternal, portInternal] : _mapPorts)
	{
		portInternal->Detach(actorIdExternal, portIdExternal);
	}
}

std::weak_ptr<IPort> ActorLocal::GetPortById(const std::string& portId)
{
	std::shared_lock lock(mtx_mapPort);
	auto it = _mapPorts.find(portId);
	if (it == _mapPorts.end())
		return std::weak_ptr<IPort>();

	return it->second;
}

std::optional<nlohmann::json*> ActorLocal::GetJsonValueFromJson(const nlohmann::json& jsonData, const std::string& propertyPath) {
	// Разбиваем путь на части
	std::vector<std::string> keys;
	std::string key;
	std::stringstream ss(propertyPath);

	// Разделяем путь по точкам и квадратным скобкам
	while (std::getline(ss, key, '.')) {
		size_t bracketPos = key.find('[');
		if (bracketPos != std::string::npos) {
			std::string objectKey = key.substr(0, bracketPos);
			keys.push_back(objectKey);
			std::string indexStr = key.substr(bracketPos + 1, key.size() - bracketPos - 2);
			keys.push_back(indexStr); // Добавляем индекс как отдельный элемент
		}
		else {
			keys.push_back(key);
		}
	}

	const nlohmann::json* current = &jsonData;
	for (const auto& k : keys) {
		//std::cout << current->dump()<<"\n";
		try {
			if (std::isdigit(k[0])) { // Если это индекс массива
				int index = std::stoi(k);
				if (current->is_array() && index < current->size()) {
					current = &(*current)[index];
				}
				else {
					return std::nullopt; // Индекс вне диапазона
				}
			}
			else { // Это ключ объекта
				if (current->contains(k)) {
					current = &(*current)[k];
				}
				else {
					return std::nullopt; // Если ключ не найден
				}
			}
		}
		catch (const std::exception&) {
			return std::nullopt; // Обработка ошибок
		}
	}

	return std::optional<nlohmann::json*>(const_cast<nlohmann::json*>(current));
}
