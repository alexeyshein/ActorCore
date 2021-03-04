#pragma once

#include <random>

namespace rf
{
    class UidGenerator
    {
    public:
        //uid = typeId-dddddddd
        static std::string Generate(std::string typeId)
        {
            std::random_device rd;
            std::mt19937_64 gen(rd());
            std::uniform_int_distribution<> dis(0, 9);
            std::string uid = typeId + "-";
            for (int i = 0; i < 8; i++)
            {
                uid += std::to_string(dis(gen));
            }
            return uid;
        }
    };

}