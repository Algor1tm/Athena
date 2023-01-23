#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Math/Vector.h"

#include <array>


namespace Athena
{
	class ATHENA_API Texture
	{
	public:
		virtual ~Texture() = default;

		inline void* GetRendererID() const { return (void*)m_RendererID; };

		virtual void Bind(uint32 slot = 0) const = 0;
		virtual bool IsLoaded() const = 0;

	protected:
		uint64 m_RendererID = 0;
	};


	class ATHENA_API Texture2D: public Texture
	{
	public:
		static Ref<Texture2D> Create(uint32 width, uint32 height);
		static Ref<Texture2D> Create(const Filepath& path);
		static Ref<Texture2D> WhiteTexture();

		virtual uint32 GetWidth() const = 0;
		virtual uint32 GetHeight() const = 0;

		virtual void SetData(const void* data, uint32 size) = 0;
		virtual const Filepath& GetFilepath() const = 0;

		bool operator==(const Texture2D& other) const { return GetRendererID() == other.GetRendererID(); }

	private:
		static Ref<Texture2D> m_WhiteTexture;
	};

	class ATHENA_API Texture2DInstance
	{
	public:
		Texture2DInstance();
		Texture2DInstance(const Ref<Texture2D>& texture);
		Texture2DInstance(const Ref<Texture2D>& texture, const std::array<Vector2, 4>& texCoords);
		Texture2DInstance(const Ref<Texture2D>& texture, const Vector2& min, const Vector2& max);

		inline const Ref<Texture2D>& GetNativeTexture() const { return m_Texture; }
		inline const std::array<Vector2, 4>& GetTexCoords() const { return m_TexCoords; };

		inline void SetTexture(const Ref<Texture2D>& texture) { m_Texture = texture; }
		inline void SetTexCoords(const std::array<Vector2, 4>& texCoords) { m_TexCoords = texCoords; }
		void SetTexCoords(const Vector2& min, const Vector2& max);

	private:
		Ref<Texture2D> m_Texture;
		std::array<Vector2, 4> m_TexCoords;
	};


	class ATHENA_API Cubemap: public Texture
	{
	public:
		static Ref<Cubemap> Create(const std::array<Filepath, 6>& faces);
	};
}
