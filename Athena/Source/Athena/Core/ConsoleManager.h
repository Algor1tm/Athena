#pragma once

#include "Athena/Core/Core.h"

#include <unordered_map>
#include <functional>


namespace Athena
{
    class CVarBase;

    class ConsoleManager
    {
    public:
        static ConsoleManager& Get()
        {
            static ConsoleManager s_Instance;
            return s_Instance;
        }

        void RegisterCVar(std::string_view name, CVarBase* cvar)
        {
            m_CVars.insert({ name, cvar });
        }

        CVarBase* FindCVar(std::string_view name)
        {
            if (m_CVars.contains(name))
            {
                return m_CVars.at(name);
            }

            return nullptr;
        }

    private:
        std::unordered_map<std::string_view, CVarBase*> m_CVars;
    };


    class CVarBase {
    public:
        CVarBase(std::string_view name, std::string_view help)
            : m_Name(name), m_Help(help)
        {
            ConsoleManager::Get().RegisterCVar(name, this);
        }

        virtual ~CVarBase() = default;

        std::string_view GetName() const { return m_Name; }
        std::string_view GetHelp() const { return m_Help; }

        void SetOnChangedCallback(std::function<void(CVarBase*)> callback)
        {
            OnChangeCallback = std::move(callback);
        }

        virtual bool GetBool() const = 0;
        virtual int32 GetInt() const = 0;
        virtual float GetFloat() const = 0;
        virtual String GetString() const = 0;

        virtual void SetBool(bool inValue) = 0;
        virtual void SetInt(int32 inValue) = 0;
        virtual void SetFloat(float inValue) = 0;
        virtual void SetString(const String& inValue) = 0;

    protected:
        void OnValueChanged()
        {
            if(OnChangeCallback)
                OnChangeCallback(this);
        }

    private:
        std::string_view m_Name;
        std::string_view m_Help;
        std::function<void(CVarBase*)> OnChangeCallback;
    };

    template <typename T>
    struct CVarStorage
    {
        template<typename U>
        U Get() const
        {
            return static_cast<U>(Value);
        }

        template<typename U>
        void Set(const U& inValue)
        {
            Value = static_cast<U>(inValue);
        }

        T Value;
    };


    template<typename T>
    class AutoCVar : public CVarBase
    {
        static_assert(false, "Incorrect CVar type, type must be bool, int32, float or String");
    };

    template<>
    class AutoCVar<bool> : public CVarBase
    {
    public:
        AutoCVar(std::string_view name, bool value, std::string_view help)
            : CVarBase(name, help), Storage(value)
        {

        }

        virtual bool GetBool() const override
        {
            return Storage.Get<bool>();
        }

        virtual int32 GetInt() const override
        {
            return Storage.Get<int32>();
        }

        virtual float GetFloat() const override
        {
            return Storage.Get<float>();
        }

        virtual String GetString() const override
        {
            ensure(false, "Invalid CVar usage!");
            return "";
        }

        virtual void SetBool(bool inValue) override
        {
            Storage.Set<bool>(inValue);
            OnValueChanged();
        }

        virtual void SetInt(int32 inValue) override
        {
            Storage.Set<int32>(inValue);
            OnValueChanged();
        }

        virtual void SetFloat(float inValue) override
        {
            Storage.Set<float>(inValue);
            OnValueChanged();
        }

        virtual void SetString(const String& inValue) override
        {
            ensure(false, "Invalid CVar usage!");
        }

    private:
        CVarStorage<bool> Storage;
    };

    template<>
    class AutoCVar<int32> : public CVarBase
    {
    public:
        AutoCVar(std::string_view name, int32 value, std::string_view help)
            : CVarBase(name, help), Storage(value)
        {

        }

        virtual bool GetBool() const override
        {
            return Storage.Get<bool>();
        }

        virtual int32 GetInt() const override
        {
            return Storage.Get<int32>();
        }

        virtual float GetFloat() const override
        {
            return Storage.Get<float>();
        }

        virtual String GetString() const override
        {
            ensure(false, "Invalid CVar usage!");
            return "";
        }

        virtual void SetBool(bool inValue) override
        {
            Storage.Set<bool>(inValue);
            OnValueChanged();
        }

        virtual void SetInt(int32 inValue) override
        {
            Storage.Set<int32>(inValue);
            OnValueChanged();
        }

        virtual void SetFloat(float inValue) override
        {
            Storage.Set<float>(inValue);
            OnValueChanged();
        }

        virtual void SetString(const String& inValue) override
        {
            ensure(false, "Invalid CVar usage!");
        }

    private:
        CVarStorage<int32> Storage;
    };

    template<>
    class AutoCVar<float> : public CVarBase
    {
    public:
        AutoCVar(std::string_view name, float value, std::string_view help)
            : CVarBase(name, help), Storage(value)
        {

        }

        virtual bool GetBool() const override
        {
            return Storage.Get<bool>();
        }

        virtual int32 GetInt() const override
        {
            return Storage.Get<int32>();
        }

        virtual float GetFloat() const override
        {
            return Storage.Get<float>();
        }

        virtual String GetString() const override
        {
            ensure(false, "Invalid CVar usage!");
            return "";
        }

        virtual void SetBool(bool inValue) override
        {
            Storage.Set<bool>(inValue);
            OnValueChanged();
        }

        virtual void SetInt(int32 inValue) override
        {
            Storage.Set<int32>(inValue);
            OnValueChanged();
        }

        virtual void SetFloat(float inValue) override
        {
            Storage.Set<float>(inValue);
            OnValueChanged();
        }

        virtual void SetString(const String& inValue) override
        {
            ensure(false, "Invalid CVar usage!");
        }

    private:
        CVarStorage<float> Storage;
    };

    template<>
    class AutoCVar<String> : public CVarBase
    {
    public:
        AutoCVar(std::string_view name, String value, std::string_view help)
            : CVarBase(name, help), Storage(value)
        {

        }

        virtual bool GetBool() const override
        {
            ensure(false, "Invalid CVar usage!");
            return false;
        }

        virtual int32 GetInt() const override
        {
            ensure(false, "Invalid CVar usage!");
            return 0;
        }

        virtual float GetFloat() const override
        {
            ensure(false, "Invalid CVar usage!");
            return 0.f;
        }

        virtual String GetString() const override
        {
            return Storage.Get<String>();
        }

        virtual void SetBool(bool inValue) override
        {
            ensure(false, "Invalid CVar usage!");
        }

        virtual void SetInt(int32 inValue) override
        {
            ensure(false, "Invalid CVar usage!");
        }

        virtual void SetFloat(float inValue) override
        {
            ensure(false, "Invalid CVar usage!");
        }

        virtual void SetString(const String& inValue) override
        {
            Storage.Set(inValue);
            OnValueChanged();
        }

    private:
        CVarStorage<String> Storage;
    };
}
