#include "PortFactory.h"


#include "PortInput.h"
#include "PortOutput.h"
#include "PortBiDirectional.h"

using namespace rf;

std::unique_ptr<IPort>  PortFactory::Create(std::string type, std::string id)
{
	std::unique_ptr<IPort> port(nullptr);
	if (type == "Input")
	{
		port.reset(new PortInput(id));
	}
	else if (type == "Output")
	{
		port.reset(new PortOutput(id));
	}
	else if (type == "BiDirectional")
	{
		port.reset(new PortOutput(id));
	}
	return std::move(port);
}

std::vector<std::string> PortFactory::GetTypes()
{
	return {"Input", "Output", "BiDirectional"};
}

//=============================================================

/** \brief	. */
extern "C" {

	void* setup_plugin(void) {
		return new PortFactory();
	}
}

