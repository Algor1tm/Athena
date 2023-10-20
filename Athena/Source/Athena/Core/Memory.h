#pragma once

#include <memory>


namespace Athena
{
	// TODO:
	// For now these classes just wrappers of std
	// In the future implement own std-like library


	template <typename T>
	class Ref
	{
	public:
		template <typename... Args>
		static Ref<T> Create(Args&&... args)
		{
			return Ref<T>(std::make_shared<T>(std::forward<Args>(args)...));
		}

		Ref()
		{

		}

		Ref(T* ptr)
			: m_Impl(ptr)
		{
			
		}

		Ref(const Ref<T>& other)
			: m_Impl(other.m_Impl)
		{

		}

		template <typename U>
		Ref(const Ref<U>& other)
			: m_Impl(other.GetImpl())
		{

		}

		Ref(Ref<T>&& other) noexcept
			: m_Impl(std::move(other.m_Impl))
		{

		}

		template <typename U>
		Ref(Ref<U>&& other) noexcept
			: m_Impl(std::move(other.GetImpl()))
		{

		}

		Ref<T>& operator=(const Ref<T>& other)
		{
			m_Impl = other.m_Impl;
			return *this;
		}

		template <typename U>
		Ref<T>& operator=(const Ref<U>& other)
		{
			m_Impl = other.GetImpl();
			return *this;
		}

		Ref<T>& operator=(Ref<T>&& other) noexcept
		{
			m_Impl = std::move(other.m_Impl);
			return *this;
		}

		template <typename U>
		Ref<T>& operator=(Ref<U>&& other) noexcept
		{
			m_Impl = std::move(other.GetImpl());
			return *this;
		}

		~Ref()
		{
			
		}

		T* Raw() const
		{
			return m_Impl.get();
		}

		void Reset(T* otherPtr)
		{
			m_Impl.reset(otherPtr);
		}

		void Release()
		{
			m_Impl.reset();
		}

		template <typename U>
		Ref<U> As() const
		{
			return std::static_pointer_cast<U>(m_Impl);
		}

		template <typename U>
		Ref<U> AsDynamic() const
		{
			return std::dynamic_pointer_cast<U>(m_Impl);
		}

		const std::shared_ptr<T>& GetImpl() const
		{
			return m_Impl;
		}

		std::shared_ptr<T>& GetImpl()
		{
			return m_Impl;
		}

		explicit operator bool() const
		{
			return (bool)m_Impl;
		}

		T& operator*() const
		{
			return *m_Impl;
		}

		T* operator->() const
		{
			return Raw();
		}

		bool operator==(const Ref<T>& other) const
		{
			return other.m_Impl == m_Impl;
		}

		bool operator!=(const Ref<T>& other) const
		{
			return other.m_Impl != m_Impl;
		}

		bool operator==(std::nullptr_t) const
		{
			return m_Impl == nullptr;
		}

		bool operator!=(std::nullptr_t) const
		{
			return m_Impl != nullptr;
		}

	public:
		Ref(const std::shared_ptr<T>& impl)
			: m_Impl(impl)
		{

		}

		Ref(std::shared_ptr<T>&& impl)
			: m_Impl(std::move(impl))
		{

		}

	private:
		std::shared_ptr<T> m_Impl;
	};


	template <typename T>
	class Scope
	{
	public:
		template <typename... Args>
		static Scope<T> Create(Args&&... args)
		{
			return Scope<T>(std::make_unique<T>(std::forward<Args>(args)...));
		}

		Scope()
		{

		}

		Scope(T* ptr)
			: m_Impl(ptr)
		{

		}

		Scope(const Ref<T>& other) = delete;
		Scope<T> operator=(const Ref<T>& other) = delete;

		Scope(Scope<T>&& other) noexcept
			: m_Impl(std::move(other.m_Impl))
		{

		}

		template <typename U>
		Scope(Scope<U>&& other) noexcept
			: m_Impl(std::move(other.GetImpl()))
		{

		}

		Scope<T>& operator=(Scope<T>&& other) noexcept
		{
			m_Impl = std::move(other.m_Impl);
			return *this;
		}

		template <typename U>
		Scope<T>& operator=(Scope<U>&& other) noexcept
		{
			m_Impl = std::move(other.GetImpl());
			return *this;
		}

		~Scope()
		{

		}

		T* Raw() const
		{
			return m_Impl.get();
		}

		void Reset(T* otherPtr)
		{
			m_Impl.reset(otherPtr);
		}

		void Release()
		{
			m_Impl.reset();
		}

		template <typename U>
		Scope<U> As() const
		{
			return std::static_pointer_cast<U>(m_Impl);
		}

		template <typename U>
		Scope<U> AsDynamic() const
		{
			return std::dynamic_pointer_cast<U>(m_Impl);
		}

		const std::unique_ptr<T>& GetImpl() const
		{
			return m_Impl;
		}

		std::unique_ptr<T>& GetImpl()
		{
			return m_Impl;
		}

		explicit operator bool() const
		{
			return (bool)m_Impl;
		}

		T& operator*() const
		{
			return *m_Impl;
		}

		T* operator->() const
		{
			return Raw();
		}

		bool operator==(const Ref<T>& other) const
		{
			return other.m_Impl == m_Impl;
		}

		bool operator!=(const Ref<T>& other) const
		{
			return other.m_Impl != m_Impl;
		}

		bool operator==(std::nullptr_t) const
		{
			return m_Impl == nullptr;
		}

		bool operator!=(std::nullptr_t) const
		{
			return m_Impl != nullptr;
		}

	public:
		Scope(std::unique_ptr<T>&& impl)
			: m_Impl(std::move(impl))
		{

		}

	private:
		std::unique_ptr<T> m_Impl;
	};
}

