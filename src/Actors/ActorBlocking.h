#pragma once

#include "ActorLocal.h"
#include <thread>
#include <mutex>

namespace rf
{
	class ActorBlocking : public ActorLocal
	{
	public:
		ActorBlocking(const std::string& id, IUnit* parent = nullptr);

		 ~ActorBlocking() override;
         //~ActorBlocking() = default;

		json Configuration() override;
		bool SetProperties(const json&) override;

    	std::variant<std::monostate, bool, int, double, std::string> GetProperty(const std::string &) override ;
    	bool SetProperty(const std::string&, int) override ;

	   json GetStatus() override;

        void Activate() override final;
        void Deactivate() override final;
		void WaitForTasks();
	protected:

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

		std::mutex mutexActivateDeactivate;
	private:
	    // For Telemetry purpose
        uint16_t      teleChannelIsProcessing;
	  
	};
}
