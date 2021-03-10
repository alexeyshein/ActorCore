#include "PortBase.h"

using  rf::PortBase;

PortBase::PortBase(std::string id):
 _id(id)
, _type("PortBase")
{

}

bool PortBase::Init(json) 
{
    return false;
}




