#include "Athena/Core/Log.h"

#include <pybind11/pybind11.h>

#include <iostream>

namespace py = pybind11;


void Log_Info(const std::string& message)
{
    std::cout << message;
}



PYBIND11_MODULE(AthenaScriptCore, handle)
{
    handle.doc() = "pybind11 example plugin";

    handle.def("Info", &Log_Info);
}
