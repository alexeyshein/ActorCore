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

    	std::variant<std::monostate, bool, int, double, std::string> GetProperty(const std::string &) override ;
    	bool SetProperty(const std::string&, int) override ;

	   json GetStatus() override;

        void Activate() final;
        void Deactivate() final;
		
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
	private:
	  
	};
}
