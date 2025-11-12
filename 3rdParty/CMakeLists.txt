 cmake_minimum_required(VERSION 3.8)

project(third_party)

add_subdirectory(json)

add_subdirectory(libP7Client)
set_target_properties(p7 PROPERTIES COMPILE_FLAGS "-Wno-address-of-packed-member") #hide warnings for p7




