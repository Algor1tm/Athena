#include "Athena/Core/Log.h"
#include "Athena/Input/Input.h"

#include "ScriptEngine.h"

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

    class PyLog
    {
    public:
        static void Trace(const std::string& message)
        {
            ATN_CORE_TRACE(message);
        }

        static void Info(const std::string& message)
        {
            ATN_CORE_INFO(message);
        }

        static void Warn(const std::string& message)
        {
            ATN_CORE_WARN(message);
        }

        static void Error(const std::string& message)
        {
            ATN_CORE_ERROR(message);
        }

        static void Fatal(const std::string& message)
        {
            ATN_CORE_FATAL(message);
        }
    };

#pragma endregion

#pragma region INPUT

    class PyInput
    {
    public:
        static bool IsKeyPressed(uint16 keyCode)
        {
            return Input::IsKeyPressed((Keyboard::Key)keyCode);
        }

        static bool IsMouseButtonPressed(uint16 mouseCode)
        {
            return Input::IsMouseButtonPressed((Mouse::Button)mouseCode);
        }

        static Vector2 GetMousePosition()
        {
            return Input::GetMousePosition();
        }
    };

#pragma endregion


    class PyComponent
    {
    public:
        PyComponent(UUID id)
            : _EntityID(id) {}

    protected:
        Entity GetEntity()
        {
            Scene* scene = ScriptEngine::GetSceneContext();
            ATN_CORE_ASSERT(scene);
            Entity entity = scene->GetEntityByUUID(_EntityID);
            ATN_CORE_ASSERT(entity);
            return entity;
        }

    private:
        UUID _EntityID;
    };

    class PyTransformComponent : public PyComponent
    {
    public:
        PyTransformComponent(UUID id)
            : PyComponent(id) {}

        bool _HasThisComponent() { return GetEntity().HasComponent<TransformComponent>(); }

        void SetTranslation(const Vector3& translation) { GetEntity().GetComponent<TransformComponent>().Translation = translation; }
        const Vector3& GetTranslation() { return GetEntity().GetComponent<TransformComponent>().Translation; }

        void SetScale(const Vector3& scale) { GetEntity().GetComponent<TransformComponent>().Scale = scale; }
        const Vector3& GetScale() { return GetEntity().GetComponent<TransformComponent>().Scale; }
        
        void SetRotation(const Vector3& rotation) { GetEntity().GetComponent<TransformComponent>().Rotation = rotation; }
        const Vector3& GetRotation() { return GetEntity().GetComponent<TransformComponent>().Rotation; }
    };


    static UUID Entity_FindEntityByName(const std::string& name)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        ATN_CORE_ASSERT(scene);
        Entity entity = scene->FindEntityByName(name);
        ATN_CORE_ASSERT(entity);
        return entity.GetID();
    }


    PYBIND11_EMBEDDED_MODULE(Internal, handle)
    {
        handle.doc() = "Internal Athena Core Calls";

        py::class_<Vector2>(handle, "Vector2")
            .def(py::init<float, float>())
            .def_readwrite("x", &Vector2::x)
            .def_readwrite("y", &Vector2::y);

        py::class_<Vector3>(handle, "Vector3")
            .def(py::init<float, float, float>())
            .def_readwrite("x", &Vector3::x)
            .def_readwrite("y", &Vector3::y)
            .def_readwrite("z", &Vector3::z);

        py::class_<Vector4>(handle, "Vector4")
            .def(py::init<float, float, float, float>())
            .def_readwrite("x", &Vector4::x)
            .def_readwrite("y", &Vector4::y)
            .def_readwrite("z", &Vector4::z)
            .def_readwrite("w", &Vector4::w);


        py::class_<PyLog>(handle, "Log")
            .def("Trace", static_cast<void (*)(const std::string&)>(PyLog::Trace))
            .def("Info", static_cast<void (*)(const std::string&)>(PyLog::Info))
            .def("Warn", static_cast<void (*)(const std::string&)>(PyLog::Warn))
            .def("Error", static_cast<void (*)(const std::string&)>(PyLog::Error))
            .def("Fatal", static_cast<void (*)(const std::string&)>(PyLog::Fatal));


        py::class_<PyInput>(handle, "Input")
            .def("IsKeyPressed", static_cast<bool (*)(uint16)>(PyInput::IsKeyPressed))
            .def("IsMouseButtonPressed", static_cast<bool (*)(uint16)>(PyInput::IsMouseButtonPressed))
            .def("GetMousePosition", static_cast<Vector2 (*)()>(PyInput::GetMousePosition));


        py::class_<Time>(handle, "Time")
            .def(py::init<>())
            .def(py::init<float>())
            .def("AsSeconds", &Time::AsSeconds)
            .def("AsMilliseconds", &Time::AsMilliseconds)
            .def("AsMicroseconds", &Time::AsMicroseconds);


        py::class_<UUID>(handle, "UUID")
            .def(py::init<>())
            .def(py::init<uint64>());
        
        ADD_INTERNAL_FUNCTION(Entity_FindEntityByName);

        py::class_<PyComponent>(handle, "Component")
            .def(py::init<UUID>());

        py::class_<PyTransformComponent, PyComponent>(handle, "TransformComponent")
            .def(py::init<uint64>())
            .def("_HasThisComponent", &PyTransformComponent::_HasThisComponent)
            .def_property("Translation", &PyTransformComponent::GetTranslation, &PyTransformComponent::SetTranslation)
            .def_property("Scale", &PyTransformComponent::GetScale, &PyTransformComponent::SetScale)
            .def_property("Rotation", &PyTransformComponent::GetRotation, &PyTransformComponent::SetRotation);
    }
}
