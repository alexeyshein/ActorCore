#pragma once

#include <json.hpp>

#include "Message.hpp"   // заголовок с шаблоном rf::Message<T>

namespace nlohmann
{

	// Message<T> -> {"id", "timestamp", "type", "sender": {...}, "data": ...}
	template<typename T>
	struct adl_serializer<rf::Message<T>>
	{
		template<typename BasicJsonType>
		static void to_json(BasicJsonType& j, const rf::Message<T>& m)
		{
			j = BasicJsonType{
				{ "id",        m.Id() },
				{ "timestamp", m.Timestamp() },
				{ "type",      m.Type() },
				{ "sender", BasicJsonType{
					{ "id",    m.IdSender() },
					{ "port",  m.IdPortSender() },
					{ "label", m.LabelSender() }
				} },
				{ "data",      m.DataConstRef() }   // данные сериализуются адаптерами выше
			};
		}
	};

} // namespace nlohmann