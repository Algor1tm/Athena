#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Math/Vector.h"

#include <array>


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


	class ATHENA_API SubTexture2D
	{
	public:
		SubTexture2D(const Ref<Texture2D>& texture, const Vector2& min, const Vector2& max);

		inline const Ref<Texture2D>& GetTexture() const { return m_Texture; }
		inline const std::array<Vector2, 4>& GetTexCoords() const { return m_TexCoords; };

		static Ref<SubTexture2D> CreateFromCoords(const Ref<Texture2D>& texture, const Vector2& coords, const Vector2& cellSize, const Vector2& spriteSize = { 1, 1 });
	private:
		Ref<Texture2D> m_Texture;
		std::array<Vector2, 4> m_TexCoords;
	};


	class ATHENA_API Texture2DStorage
	{
	public:
		Texture2DStorage(const Ref<Texture2D>& texture);
		Texture2DStorage(const Ref<SubTexture2D>& subtexture);

		inline const Ref<Texture2D>& GetTexture() const { return m_Texture; }
		inline const std::array<Vector2, 4>& GetTexCoords() const { return m_TexCoords; };

		inline void SetTextureCoords(const std::array<Vector2, 4>& coords) { m_TexCoords = coords; }

	private:
		Ref<Texture2D> m_Texture;
		std::array<Vector2, 4> m_TexCoords;
	};
}

