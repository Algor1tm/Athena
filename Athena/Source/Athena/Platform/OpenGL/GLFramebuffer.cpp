#include "GLFramebuffer.h"

#include "Athena/Platform/OpenGL/GLUtils.h"

#include <glad/glad.h>


namespace Athena
{
	static uint32 s_MaxFramebufferSize = 8192;

	static GLenum GLTextureTarget(bool multisample, uint32 depth)
	{
		if (depth > 1)
			return multisample ? GL_TEXTURE_2D_MULTISAMPLE_ARRAY : GL_TEXTURE_2D_ARRAY;

		return multisample ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
	}

	static void CreateTextures(bool multisample, uint32 depth, uint32* outIDs, uint32 count)
	{
		glCreateTextures(GLTextureTarget(multisample, depth), (GLsizei)count, outIDs);
	}

	static void BindTexture(bool multisample, uint32 depth, uint32 id)
	{
		glBindTexture(GLTextureTarget(multisample, depth), id);
	}

	GLFramebuffer::GLFramebuffer(const FramebufferDescription& desc)
		: m_Description(desc)
	{
		m_ColorAttachmentDescriptions.reserve(desc.Attachments.size());

		for (auto format : desc.Attachments)
		{
			if (!Utils::IsDepthFormat(format.Format))
				m_ColorAttachmentDescriptions.emplace_back(format);
			else
				m_DepthAttachmentDescription = format;
		}

		Recreate();
	}

	GLFramebuffer::~GLFramebuffer()
	{
		DeleteAttachments();
	}

	void GLFramebuffer::Recreate()
	{
		if (m_FramebufferID)
		{
			DeleteAttachments();

			m_ColorAttachments.clear();
			m_ColorAttachmentsResolved.clear();
			m_DepthAttachment = 0;
		}

		if (IsMultisample())
		{
			CreateFramebufferObject(&m_ResolvedFramebufferID, true);
		}
		
		CreateFramebufferObject(&m_FramebufferID, false);

		if (m_ColorAttachments.size() > 1)
		{
			ATN_CORE_ASSERT(m_ColorAttachments.size() <= 4, "GLFramebuffer supports only up to 4 color attachments!");
			GLenum buffers[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
			glDrawBuffers((GLsizei)m_ColorAttachments.size(), buffers);
		}
		else if (m_ColorAttachments.empty())
		{
			glDrawBuffer(GL_NONE);
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void GLFramebuffer::Resize(uint32 width, uint32 height)
	{
		if (width == 0 || height == 0 || width > s_MaxFramebufferSize || height > s_MaxFramebufferSize)
		{
			ATN_CORE_WARN("Attempted to resize Framebuffer to invalid size ({0}, {1})", width, height);
			return;
		}

		m_Description.Width = width;
		m_Description.Height = height;

		Recreate();
	}

	void GLFramebuffer::Bind()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, m_FramebufferID);
		glViewport(0, 0, m_Description.Width, m_Description.Height);
	}

	void GLFramebuffer::UnBind()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void GLFramebuffer::BindColorAttachmentAsImage(uint32 index, uint32 slot, uint32 mipLevel) const
	{
		if (index >= m_ColorAttachments.size())
		{
			ATN_CORE_ERROR("Invalid index for color attachment!");
			ATN_CORE_ASSERT(false);
		}
		else
		{
			GLenum internalFormat, dataFormat, dataType;
			Utils::TextureFormatToGLenum(m_ColorAttachmentDescriptions[index].Format, internalFormat, dataFormat, dataType);

			glBindImageTexture(slot, m_ColorAttachments[index], mipLevel, GL_FALSE, 0, GL_READ_WRITE, internalFormat);
		}
	}

	void GLFramebuffer::BindColorAttachment(uint32 index, uint32 slot) const
	{
		if (index >= m_ColorAttachments.size())
		{
			ATN_CORE_ERROR("Invalid index for color attachment!");
			ATN_CORE_ASSERT(false);
		}
		else
		{
			glBindTextureUnit(slot, IsMultisample() ? m_ColorAttachmentsResolved[index] : m_ColorAttachments[index]);
		}
	}

	void GLFramebuffer::BindDepthAttachment(uint32 slot) const
	{
		if (m_DepthAttachmentDescription.Format == TextureFormat::NONE)
		{
			ATN_CORE_ERROR("Framebuffer does not have depth attachment!");
			ATN_CORE_ASSERT(false);
		}
		else
		{
			glBindTextureUnit(slot, IsMultisample() ? m_DepthAttachmentResolved : m_DepthAttachment);
		}
	}

	void* GLFramebuffer::GetColorAttachmentRendererID(uint32 index) const
	{ 
		if (IsMultisample())
		{
			ATN_CORE_ASSERT(index < m_ColorAttachmentsResolved.size(), "Invalid index for color attachment!");
			return reinterpret_cast<void*>((uint64)m_ColorAttachmentsResolved[index]);
		}

		ATN_CORE_ASSERT(index < m_ColorAttachments.size(), "Invalid index for color attachment!");
		return reinterpret_cast<void*>((uint64)m_ColorAttachments[index]); 
	}

	void* GLFramebuffer::GetDepthAttachmentRendererID() const
	{
		if (m_DepthAttachmentDescription.Format == TextureFormat::NONE)
		{
			ATN_CORE_ERROR("Framebuffer does not have depth attachment!");
			ATN_CORE_ASSERT(false);
			return 0;
		}

		if (IsMultisample())
			return reinterpret_cast<void*>((uint64)m_DepthAttachmentResolved);

		return reinterpret_cast<void*>((uint64)m_DepthAttachment);
	}

	int GLFramebuffer::ReadPixel(uint32 attachmentIndex, int x, int y)
	{
		ATN_CORE_ASSERT(attachmentIndex < m_ColorAttachments.size(), "Invalid index for color attachment!");
		glBindFramebuffer(GL_FRAMEBUFFER, IsMultisample() ? m_ResolvedFramebufferID : m_FramebufferID); 

		glReadBuffer(GL_COLOR_ATTACHMENT0 + (GLenum)attachmentIndex);
		int pixelData;
		glReadPixels(x, y, 1, 1, GL_RED_INTEGER, GL_INT, &pixelData);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		return pixelData;
	}

	void GLFramebuffer::ClearAttachment(uint32 attachmentIndex, int value)
	{
		ATN_CORE_ASSERT(attachmentIndex < m_ColorAttachments.size(), "Invalid index for color attachment!");
		
		auto& desc = m_ColorAttachmentDescriptions[attachmentIndex];
		
		glClearTexImage(m_ColorAttachments[attachmentIndex], 0, Utils::TextureFormatToGLenum(desc.Format), GL_INT, &value);
	}

	void GLFramebuffer::DeleteAttachments()
	{
		glDeleteFramebuffers(1, &m_FramebufferID);

		glDeleteTextures((GLsizei)m_ColorAttachments.size(), m_ColorAttachments.data());
		if (IsMultisample())
			glDeleteTextures((GLsizei)m_ColorAttachmentsResolved.size(), m_ColorAttachmentsResolved.data());

		glDeleteTextures(1, &m_DepthAttachment);
		if (IsMultisample())
			glDeleteTextures(1, &m_DepthAttachmentResolved);
	}

	void GLFramebuffer::ResolveMutlisampling()
	{
		if (IsMultisample())
		{
			for (uint32 i = 0; i < m_ColorAttachmentDescriptions.size(); ++i)
			{
				glBindFramebuffer(GL_READ_FRAMEBUFFER, m_FramebufferID);
				glReadBuffer(GL_COLOR_ATTACHMENT0 + (GLenum)i);
				glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_ResolvedFramebufferID);
				glDrawBuffer(GL_COLOR_ATTACHMENT0 + (GLenum)i);

				glBlitFramebuffer(0, 0, m_Description.Width, m_Description.Height, 0, 0, m_Description.Width, m_Description.Height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
			}

			if (m_DepthAttachmentDescription.Format != TextureFormat::NONE)
			{
				glBindFramebuffer(GL_READ_FRAMEBUFFER, m_FramebufferID);
				glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_ResolvedFramebufferID);

				glBlitFramebuffer(0, 0, m_Description.Width, m_Description.Height, 0, 0, m_Description.Width, m_Description.Height, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
			}
		}
	}

	void GLFramebuffer::BlitToScreen() const
	{
		if (IsMultisample())
			glBindFramebuffer(GL_READ_FRAMEBUFFER, m_ResolvedFramebufferID);
		else
			glBindFramebuffer(GL_READ_FRAMEBUFFER, m_FramebufferID);

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glBlitFramebuffer(0, 0, m_Description.Width, m_Description.Height, 0, 0, m_Description.Width, m_Description.Height,
			GL_COLOR_BUFFER_BIT, GL_NEAREST);
	}

	void GLFramebuffer::CreateFramebufferObject(uint32* rendererID, bool resolved)
	{
		glCreateFramebuffers(1, rendererID);
		glBindFramebuffer(GL_FRAMEBUFFER, *rendererID);

		uint32 samples = resolved ? 1 : m_Description.Samples;
		bool multisample = samples > 1;
		std::vector<uint32>& attachments = resolved ? m_ColorAttachmentsResolved : m_ColorAttachments;

		if (!m_ColorAttachmentDescriptions.empty())
		{
			attachments.resize(m_ColorAttachmentDescriptions.size());
			CreateTextures(multisample, m_Description.Layers, attachments.data(), attachments.size());
			//Attachments
			for (uint32 i = 0; i < attachments.size(); ++i)
			{
				BindTexture(multisample, m_Description.Layers, attachments[i]);
				AttachColorTexture(attachments[i], samples, i);
			}
		}

		uint32& depthAttachment = resolved ? m_DepthAttachmentResolved : m_DepthAttachment;

		if (m_DepthAttachmentDescription.Format != TextureFormat::NONE)
		{
			CreateTextures(multisample, m_Description.Layers, &depthAttachment, 1);
			BindTexture(multisample, m_Description.Layers, depthAttachment);

			AttachDepthTexture(depthAttachment, samples);
		}

		ATN_CORE_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Framebuffer creation is failed!");
	}

	void GLFramebuffer::AttachColorTexture(uint32 id, uint32 samples, uint32 index)
	{
		uint32 width = m_Description.Width;
		uint32 height = m_Description.Height;
		uint32 layers = m_Description.Layers;

		bool generateMipMap = samples == 1 && m_ColorAttachmentDescriptions[index].GenerateMipMap;

		GLenum internalFormat, dataFormat, dataType;
		Utils::TextureFormatToGLenum(m_ColorAttachmentDescriptions[index].Format, internalFormat, dataFormat, dataType);

		if (samples > 1)
		{
			if (layers > 1)
				glTexImage3DMultisample(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, samples, internalFormat, width, height, layers, GL_FALSE);
			else
				glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, internalFormat, width, height, GL_FALSE);
		}
		else
		{
			if(layers > 1)
				glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, internalFormat, width, height, layers, 0, dataFormat, dataType, nullptr);
			else
				glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, dataType, nullptr);

			GLenum target = layers > 1 ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D;

			glTexParameteri(target, GL_TEXTURE_MIN_FILTER, generateMipMap ? GL_LINEAR_MIPMAP_NEAREST : GL_LINEAR);
			glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
			glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

			if (generateMipMap)
				glGenerateMipmap(target);
		}

		if(layers > 1)
			glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + (GLenum)index, id, 0);
		else
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + (GLenum)index, GLTextureTarget(samples > 1, layers), id, 0);
	}

	void GLFramebuffer::AttachDepthTexture(uint32 id, uint32 samples)
	{
		uint32 width = m_Description.Width;
		uint32 height = m_Description.Height;
		uint32 layers = m_Description.Layers;

		bool generateMipMap = m_DepthAttachmentDescription.GenerateMipMap;

		GLenum internalFormat, dataFormat, dataType;
		Utils::TextureFormatToGLenum(m_DepthAttachmentDescription.Format, internalFormat, dataFormat, dataType);

		GLenum attachmentType = Utils::GetDepthAttachmentType(m_DepthAttachmentDescription.Format);

		if (samples > 1)
		{
			if (layers > 1)
				glTexImage3DMultisample(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, samples, internalFormat, width, height, layers, GL_FALSE);
			else
				glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, internalFormat, width, height, GL_FALSE);
		}
		else
		{
			if(layers > 1)
				glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, internalFormat, width, height, layers);
			else
				glTexStorage2D(GL_TEXTURE_2D, 1, internalFormat, width, height);

			GLenum target = layers > 1 ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D;

			glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
			glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

			if (samples == 1 && generateMipMap)
				glGenerateMipmap(target);
		}

		if (layers > 1)
			glFramebufferTexture(GL_FRAMEBUFFER, attachmentType, id, 0);
		else
			glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentType, GLTextureTarget(samples > 1, layers), id, 0);
	}
}
