#include "Athena/Core/Log.h"
#include "Athena/Input/Input.h"

#include "ScriptEngine.h"

#include <Box2D/b2_body.h>

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#pragma warning(push, 0)
#endif

#include <pybind11/embed.h>

#ifdef _MSC_VER
#undef _CRT_SECURE_NO_WARNINGS
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
        PyComponent(UUID id = 0)
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
        PyTransformComponent(UUID id = 0)
            : PyComponent(id) {}

        bool _HasThisComponent() { return GetEntity().HasComponent<TransformComponent>(); }

        void SetTranslation(const Vector3& translation) { GetEntity().GetComponent<TransformComponent>().Translation = translation; }
        const Vector3& GetTranslation() { return GetEntity().GetComponent<TransformComponent>().Translation; }

        void SetScale(const Vector3& scale) { GetEntity().GetComponent<TransformComponent>().Scale = scale; }
        const Vector3& GetScale() { return GetEntity().GetComponent<TransformComponent>().Scale; }
        
        void SetRotation(const Vector3& rotation) { GetEntity().GetComponent<TransformComponent>().Rotation = rotation; }
        const Vector3& GetRotation() { return GetEntity().GetComponent<TransformComponent>().Rotation; }
    };

    class PyRigidbody2DComponent : public PyComponent
    {
    public:
        PyRigidbody2DComponent(UUID id = 0)
            : PyComponent(id) {}


        bool _HasThisComponent() { return GetEntity().HasComponent<Rigidbody2DComponent>(); }

        void ApplyLinearImpulse(const Vector2& impulse, const Vector2& point, bool wake)
        {
            Entity entity = GetEntity();
            auto& rb2d = entity.GetComponent<Rigidbody2DComponent>();
            b2Body* body = reinterpret_cast<b2Body*>(rb2d.RuntimeBody);
            body->ApplyLinearImpulse({ impulse.x, impulse.y }, {point.x, point.y}, wake);
        }

        void ApplyLinearImpulseToCenter(const Vector2& impulse, bool wake)
        {
            Entity entity = GetEntity();
            auto& rb2d = entity.GetComponent<Rigidbody2DComponent>();
            b2Body* body = reinterpret_cast<b2Body*>(rb2d.RuntimeBody);
            body->ApplyLinearImpulseToCenter({ impulse.x, impulse.y }, wake);
        }
    };


    static UUID Entity_FindEntityByName(const std::string& name)
    {
        Scene* scene = ScriptEngine::GetSceneContext();
        ATN_CORE_ASSERT(scene);
        Entity entity = scene->FindEntityByName(name);
        ATN_CORE_ASSERT(entity);
        return entity.GetID();
    }

    static py::object Entity_GetScriptInstance(UUID uuid)
    {
        return ScriptEngine::GetScriptInstance(uuid).GetInternalInstance();
    }

    static bool Entity_ExistsScriptInstance(UUID uuid)
    {
        return ScriptEngine::EntityInstanceExists(uuid);
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
            .def(py::init<uint64>())
            .def("AsUInt64", [](UUID* self) { return (uint64)*self; });
            
        ADD_INTERNAL_FUNCTION(Entity_FindEntityByName);
        ADD_INTERNAL_FUNCTION(Entity_ExistsScriptInstance);
        handle.def(ATN_STRINGIFY_MACRO(Entity_GetScriptInstance), &Athena::Entity_GetScriptInstance, py::return_value_policy::reference);


        py::class_<PyComponent>(handle, "Component")
            .def(py::init<>())
            .def(py::init<UUID>());

        py::class_<PyTransformComponent, PyComponent>(handle, "TransformComponent")
            .def(py::init<>())
            .def(py::init<UUID>())
            .def("_HasThisComponent", &PyTransformComponent::_HasThisComponent)
            .def_property("Translation", &PyTransformComponent::GetTranslation, &PyTransformComponent::SetTranslation)
            .def_property("Scale", &PyTransformComponent::GetScale, &PyTransformComponent::SetScale)
            .def_property("Rotation", &PyTransformComponent::GetRotation, &PyTransformComponent::SetRotation);

        py::class_<PyRigidbody2DComponent, PyComponent>(handle, "Rigidbody2DComponent")
            .def(py::init<>())
            .def(py::init<UUID>())
            .def("_HasThisComponent", &PyRigidbody2DComponent::_HasThisComponent)
            .def("ApplyLinearImpulse", &PyRigidbody2DComponent::ApplyLinearImpulse)
            .def("ApplyLinearImpulseToCenter", &PyRigidbody2DComponent::ApplyLinearImpulseToCenter);
    }
}
