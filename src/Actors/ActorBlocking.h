#pragma once

#include "ActorLocal.h"
#include <thread>

namespace rf
{
	class ActorBlocking : public ActorLocal
	{
	public:
		ActorBlocking(const std::string& id);

		 ~ActorBlocking() override;
         //~ActorBlocking() = default;
		bool Init(const json&) override;

		std::variant<bool, int, double> GetProperty(const std::string&) override;


	   json GetStatus() override;

	protected:
	   
       void OnActivate() override;

       void OnDeactivate() override;
			/*!
	\brief Циклически вызываемая функция.
	\return успех
	*/
		virtual bool Process() = 0;

	private:
		
		void processingLoop();

	protected:
		//Поток непрерывной обработки
		std::thread _processingLoopThread;

		//Флаг для остановки потока
		bool _flagStop;

		size_t minLoopTimeMks;
	private:
	  
	};
}
