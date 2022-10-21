#include "Athena/Core/Log.h"
#include "Athena/Input/Input.h"

#include "ScriptEntity.h"

#ifdef _MSC_VER
#pragma warning(push, 0)
#endif

#include <pybind11/embed.h>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

namespace py = pybind11;

#define ADD_INTERNAL_FUNCTION(Name) handle.def(ATN_STRINGIFY_MACRO(Name), &Athena::Name)


namespace Athena
{
#pragma region LOG

    void Log_Trace(const std::string& message)
    {
        ATN_CORE_TRACE(message);
    }

    void Log_Info(const std::string& message)
    {
        ATN_CORE_INFO(message);
    }

    void Log_Warn(const std::string& message)
    {
        ATN_CORE_WARN(message);
    }

    void Log_Error(const std::string& message)
    {
        ATN_CORE_ERROR(message);
    }

    void Log_Fatal(const std::string& message)
    {
        ATN_CORE_FATAL(message);
    }

#pragma endregion

#pragma region INPUT

    bool Input_IsKeyPressed(uint16 keyCode)
    {
        ATN_CORE_ERROR(keyCode);
        return Input::IsKeyPressed((Keyboard::Key)keyCode);
    }

    bool Input_IsMouseButtonPressed(uint16 mouseCode)
    {
        return Input::IsMouseButtonPressed((Mouse::Button)mouseCode);
    }

    Vector2 Input_GetMousePosition()
    {
        return Input::GetMousePosition();
    }

#pragma endregion


    PYBIND11_EMBEDDED_MODULE(Internal, handle)
    {
        handle.doc() = "Internal Athena Core Calls";

        py::class_<Vector2>(handle, "Vector2")
            .def_readwrite("x", &Vector2::x)
            .def_readwrite("y", &Vector2::y);

        py::class_<Vector3>(handle, "Vector3")
            .def_readwrite("x", &Vector3::x)
            .def_readwrite("y", &Vector3::y)
            .def_readwrite("z", &Vector3::z);

        py::class_<Vector4>(handle, "Vector4")
            .def_readwrite("x", &Vector4::x)
            .def_readwrite("y", &Vector4::y)
            .def_readwrite("z", &Vector4::z)
            .def_readwrite("w", &Vector4::w);


        py::class_<ScriptEntity>(handle, "Entity")
            .def(py::init<>())
            .def_readwrite("_ID", &ScriptEntity::m_ID)
            .def("GetTranslation", &ScriptEntity::GetTranslation);

        ADD_INTERNAL_FUNCTION(Log_Trace);
        ADD_INTERNAL_FUNCTION(Log_Info);
        ADD_INTERNAL_FUNCTION(Log_Warn);
        ADD_INTERNAL_FUNCTION(Log_Error);
        ADD_INTERNAL_FUNCTION(Log_Fatal);

        ADD_INTERNAL_FUNCTION(Input_IsKeyPressed);
        ADD_INTERNAL_FUNCTION(Input_IsMouseButtonPressed);
        ADD_INTERNAL_FUNCTION(Input_GetMousePosition);
    }
}
