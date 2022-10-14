#include <pybind11/pybind11.h>

#include <iostream>

namespace py = pybind11;


struct Pet
{
    Pet(const std::string& name) : name(name) { }
    void setName(const std::string& name_) { name = name_; }
    const std::string& getName() const { return name; }

    std::string name;
};

struct Dog : Pet
{
    Dog(const std::string& name) : Pet(name) { }
    void bark() const { std::cout << "woof!"; }
};


int add(int i, int j = 2)
{
    std::cout << "Hello from C++!\n";
    return i + j;
}


PYBIND11_MODULE(AthenaScriptCore, handle)
{
    handle.doc() = "pybind11 example plugin";

    handle.def("add", &add, py::arg("i"), py::arg("j") = 2);

    py::class_<Pet>(handle, "Pet")
        .def(py::init<const std::string&>())
        .def("setName", &Pet::setName)
        .def("getName", &Pet::getName)
        .def_readwrite("name", &Pet::name);

    py::class_<Dog, Pet>(handle, "Dog")
        .def(py::init<const std::string&>())
        .def("bark", &Dog::bark);
}
