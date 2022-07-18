#pragma once


namespace Athena
{
	class ATHENA_API Texture
	{
	public:
		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;

		virtual void SetData(const void* data, uint32_t size) = 0;

		virtual void Bind(uint32_t slot = 0) const = 0;
		virtual void UnBind() const = 0;
	};


	class ATHENA_API Texture2D: public Texture
	{
	public:
		static Ref<Texture2D> Create(uint32_t width, uint32_t height);
		static Ref<Texture2D> Create(const std::string& path);

		virtual bool operator==(const Texture2D& other) const = 0;
	};

}

