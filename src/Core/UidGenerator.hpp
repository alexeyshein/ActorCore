#pragma once

#include <random>

namespace rf
{
    class UidGenerator
    {
    public:
        //uid = type-dddddddd
        static std::string Generate(std::string type)
        {
            std::random_device rd;
            std::mt19937_64 gen(rd());
            std::uniform_int_distribution<> dis(0, 9);
            std::string uid = type + "-";
            for (int i = 0; i < 8; i++)
            {
                uid += std::to_string(dis(gen));
            }
            return uid;
        }
    };

}