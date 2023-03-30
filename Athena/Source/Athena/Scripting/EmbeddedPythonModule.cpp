#include "Athena/Core/Log.h"

#include "Athena/Input/Input.h"

#include "Athena/Scripting/ScriptEngine.h"

#include "Athena/Scene/Components.h"

#include <Box2D/b2_body.h>

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#pragma warning(push, 0)
#endif

#include <pybind11/embed.h>
#include <pybind11/operators.h>

#ifdef _MSC_VER
#undef _CRT_SECURE_NO_WARNINGS
#pragma warning(pop)
#endif

namespace py = pybind11;

#define ADD_INTERNAL_FUNCTION(Name) handle.def(ATN_STRINGIFY_MACRO(Name), &Athena::Name)

#define DEF_MATH_OPERATORS()        .def(py::self + py::self)   \
                                    .def(py::self - py::self)   \
                                    .def(py::self * py::self)   \
                                    .def(py::self / py::self)   \
                                    .def(py::self += py::self)  \
                                    .def(py::self -= py::self)  \
                                    .def(py::self *= py::self)  \
                                    .def(py::self /= py::self)  \
                                    .def(py::self + float())    \
                                    .def(py::self - float())    \
                                    .def(py::self * float())    \
                                    .def(py::self / float())    \
                                    .def(py::self += float())   \
                                    .def(py::self -= float())   \
                                    .def(py::self *= float())   \
                                    .def(py::self /= float())   \
                                    .def(float() + py::self)    \
                                    .def(float() * py::self)    \
                                    .def(-py::self)             \
                                    .def(py::self == py::self)  \
                                    .def(py::self != py::self)


namespace Athena
{
#pragma region LOG
    class Python_Log
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

#pragma endregion LOG


#pragma region INPUT
    class Python_Input
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

#pragma endregion INPUT


#pragma region COMPONENTS
    class Python_Component
    {
    public:
        Python_Component(UUID id = 0)
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

    class Python_TransformComponent : public Python_Component
    {
    public:
        Python_TransformComponent(UUID id = 0)
            : Python_Component(id) {}

        bool _HasThisComponent() { return GetEntity().HasComponent<TransformComponent>(); }

        void SetTranslation(const Vector3& translation) { GetEntity().GetComponent<TransformComponent>().Translation = translation; }
        const Vector3& GetTranslation() { return GetEntity().GetComponent<TransformComponent>().Translation; }

        void SetScale(const Vector3& scale) { GetEntity().GetComponent<TransformComponent>().Scale = scale; }
        const Vector3& GetScale() { return GetEntity().GetComponent<TransformComponent>().Scale; }
        
        void SetRotation(const Quaternion& rotation) { GetEntity().GetComponent<TransformComponent>().Rotation = rotation; }
        const Quaternion& GetRotation() { return GetEntity().GetComponent<TransformComponent>().Rotation; }
    };

    class Python_Rigidbody2DComponent : public Python_Component
    {
    public:
        Python_Rigidbody2DComponent(UUID id = 0)
            : Python_Component(id) {}


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

            Vector4(Vector2(), Vector2());
        }
    };
#pragma endregion COMPONENTS


#pragma region UTILS
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
#pragma endregion UTILS

    PYBIND11_EMBEDDED_MODULE(Internal, handle)
    {
        handle.doc() = "Internal Athena Core Calls";


#pragma region MATH
        py::class_<Vector2>(handle, "Vector2")
            .def(py::init([]() {return Vector2(0, 0); }))
            .def(py::init<float, float>())
            .def_readwrite("x", &Vector2::x)
            .def_readwrite("y", &Vector2::y)
            DEF_MATH_OPERATORS()
            .def("Length", &Vector2::Length)
            .def("Normalize", &Vector2::Normalize)
            .def("Dot", (float (*)(const Vector2&, const Vector2&))Math::Dot)
            .def("Cross", (float (*)(const Vector2&, const Vector2&))Math::Cross)
            .def("__repr__", [](const Vector2& vec2) { return Athena::ToString(vec2); });


        py::class_<Vector3>(handle, "Vector3")
            .def(py::init([]() {return Vector3(0, 0, 0); }))
            .def(py::init<float, float, float>())
            .def(py::init<Vector2, float>())
            .def(py::init<float, Vector2>())
            .def_readwrite("x", &Vector3::x)
            .def_readwrite("y", &Vector3::y)
            .def_readwrite("z", &Vector3::z)
            DEF_MATH_OPERATORS()
            .def("Length", &Vector3::Length)
            .def("Normalize", &Vector3::Normalize)
            .def("Dot", (float (*)(const Vector3&, const Vector3&))Math::Dot)
            .def("Cross", (Vector3(*)(const Vector3&, const Vector3&))Math::Cross)
            .def("__repr__", [](Vector3 vec3) { return Athena::ToString(vec3); });

        py::class_<Vector4>(handle, "Vector4")
            .def(py::init([]() {return Vector4(0, 0, 0, 0); }))
            .def(py::init<float, float, float, float>())
            .def(py::init<Vector2, Vector2>())
            .def(py::init<float, Vector3>())
            .def(py::init<Vector3, float>())
            .def_readwrite("x", &Vector4::x)
            .def_readwrite("y", &Vector4::y)
            .def_readwrite("z", &Vector4::z)
            .def_readwrite("w", &Vector4::w)
            DEF_MATH_OPERATORS()
            .def("Length", &Vector4::Length)
            .def("Normalize", &Vector4::Normalize)
            .def("Dot", (float (*)(const Vector4&, const Vector4&))Math::Dot)
            .def("__repr__", [](Vector4 vec4) { return Athena::ToString(vec4); });

        py::class_<Quaternion>(handle, "Quaternion")
            .def(py::init([]() {return Quaternion(1, 0, 0, 0); }))
            .def(py::init<Vector3>())
            .def(py::init<Vector4>())
            .def_readwrite("w", &Quaternion::w)
            .def_readwrite("x", &Quaternion::x)
            .def_readwrite("y", &Quaternion::y)
            .def_readwrite("z", &Quaternion::z)
            .def("__repr__", [](Quaternion quat) { return Athena::ToString(quat); });
#pragma endregion MATH


#pragma region LOG
        py::class_<Python_Log>(handle, "Log")
            .def("Trace", static_cast<void (*)(const std::string&)>(Python_Log::Trace))
            .def("Info", static_cast<void (*)(const std::string&)>(Python_Log::Info))
            .def("Warn", static_cast<void (*)(const std::string&)>(Python_Log::Warn))
            .def("Error", static_cast<void (*)(const std::string&)>(Python_Log::Error))
            .def("Fatal", static_cast<void (*)(const std::string&)>(Python_Log::Fatal));
#pragma endregion LOG


#pragma region INPUT
        py::class_<Python_Input>(handle, "Input")
            .def("IsKeyPressed", static_cast<bool (*)(uint16)>(Python_Input::IsKeyPressed))
            .def("IsMouseButtonPressed", static_cast<bool (*)(uint16)>(Python_Input::IsMouseButtonPressed))
            .def("GetMousePosition", static_cast<Vector2 (*)()>(Python_Input::GetMousePosition));
#pragma endregion INPUT


#pragma region UTILS
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
#pragma endregion UTILS


#pragma region COMPONENTS
        py::class_<Python_Component>(handle, "Component")
            .def(py::init<>())
            .def(py::init<UUID>());

        py::class_<Python_TransformComponent, Python_Component>(handle, "TransformComponent")
            .def(py::init<>())
            .def(py::init<UUID>())
            .def("_HasThisComponent", &Python_TransformComponent::_HasThisComponent)
            .def_property("Translation", &Python_TransformComponent::GetTranslation, &Python_TransformComponent::SetTranslation)
            .def_property("Scale", &Python_TransformComponent::GetScale, &Python_TransformComponent::SetScale)
            .def_property("Rotation", &Python_TransformComponent::GetRotation, &Python_TransformComponent::SetRotation);

        py::class_<Python_Rigidbody2DComponent, Python_Component>(handle, "Rigidbody2DComponent")
            .def(py::init<>())
            .def(py::init<UUID>())
            .def("_HasThisComponent", &Python_Rigidbody2DComponent::_HasThisComponent)
            .def("ApplyLinearImpulse", &Python_Rigidbody2DComponent::ApplyLinearImpulse)
            .def("ApplyLinearImpulseToCenter", &Python_Rigidbody2DComponent::ApplyLinearImpulseToCenter);
#pragma endregion COMPONENTS
    }
}
