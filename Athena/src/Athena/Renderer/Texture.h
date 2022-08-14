#pragma once


namespace Athena
{
	class ATHENA_API Texture
	{
	public:
		virtual uint32 GetWidth() const = 0;
		virtual uint32 GetHeight() const = 0;

		virtual void SetData(const void* data, uint32 size) = 0;

		virtual void Bind(uint32 slot = 0) const = 0;
		virtual void UnBind() const = 0;
	};


	class ATHENA_API Texture2D: public Texture
	{
	public:
		static Ref<Texture2D> Create(uint32 width, uint32 height);
		static Ref<Texture2D> Create(const String& path);

		virtual bool operator==(const Texture2D& other) const = 0;
	};

}

