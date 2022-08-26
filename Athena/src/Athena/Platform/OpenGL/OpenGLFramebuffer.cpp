#include "atnpch.h"
#include "OpenGLFramebuffer.h"

#include <glad/glad.h>


namespace Athena
{
	static uint32 s_MaxFramebufferSize = 8192;


	static GLenum TextureTarget(bool multisample)
	{
		return multisample ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
	}

	static void CreateTextures(bool multisample, RendererID* outIDs, SIZE_T count)
	{
		glCreateTextures(TextureTarget(multisample), (GLsizei)count, outIDs);
	}

	static void BindTexture(bool multisample, RendererID id)
	{
		glBindTexture(TextureTarget(multisample), id);
	}

	static bool IsDepthFormat(FramebufferTextureFormat format)
	{
		switch (format)
		{
			case FramebufferTextureFormat::DEPTH24STENCIL8: return true;
		}

		return false;
	}

	static GLenum TextureFormatToGLenum(FramebufferTextureFormat format)
	{
		switch (format)
		{
		case FramebufferTextureFormat::RED_INTEGER: return GL_RED_INTEGER;
		case FramebufferTextureFormat::RGBA8: return GL_RGBA8;
		case FramebufferTextureFormat::DEPTH24STENCIL8: return GL_DEPTH24_STENCIL8;
		}

		ATN_CORE_ASSERT(false, "Unknown texture format!");
		return GL_NONE;
	}


	OpenGLFramebuffer::OpenGLFramebuffer(const FramebufferDESC& desc)
		: m_Description(desc)
	{
		m_ColorAttachmentDescriptions.reserve(desc.Attachments.Attachments.size());

		for (auto format : desc.Attachments.Attachments)
		{
			if (!IsDepthFormat(format.TextureFormat))
				m_ColorAttachmentDescriptions.emplace_back(format);
			else
				m_DepthAttachmentDescription = format;
		}

		Recreate();
	}

	OpenGLFramebuffer::~OpenGLFramebuffer()
	{
		DeleteAttachments();
	}

	void OpenGLFramebuffer::Recreate()
	{
		if (m_RendererID)
		{
			DeleteAttachments();

			m_ColorAttachments.clear();
		}

		glCreateFramebuffers(1, &m_RendererID);
		glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID);

		bool multisample = m_Description.Samples > 1;

		if (!m_ColorAttachmentDescriptions.empty())
		{
			m_ColorAttachments.resize(m_ColorAttachmentDescriptions.size());
			CreateTextures(multisample, m_ColorAttachments.data(), m_ColorAttachments.size());
			//Attachments
			for (SIZE_T i = 0; i < m_ColorAttachments.size(); ++i)
			{
				BindTexture(multisample, m_ColorAttachments[i]);
				switch (m_ColorAttachmentDescriptions[i].TextureFormat)
				{
				case FramebufferTextureFormat::RGBA8:
					AttachColorTexture(m_ColorAttachments[i], m_Description.Samples, GL_RGBA8, GL_RGBA, m_Description.Width, m_Description.Height, i);
					break;
				case FramebufferTextureFormat::RED_INTEGER:
					AttachColorTexture(m_ColorAttachments[i], m_Description.Samples, GL_R32I, GL_RED_INTEGER, m_Description.Width, m_Description.Height, i);
				}
			}
		}

		if (m_DepthAttachmentDescription.TextureFormat != FramebufferTextureFormat::None)
		{
			CreateTextures(multisample, &m_DepthAttachment, 1);
			BindTexture(multisample, m_DepthAttachment);
			switch (m_DepthAttachmentDescription.TextureFormat)
			{
			case FramebufferTextureFormat::DEPTH24STENCIL8:
				AttachDepthTexture(m_DepthAttachment, m_Description.Samples, GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL_ATTACHMENT, m_Description.Width, m_Description.Height);
				break;
			}
		}

		if (m_ColorAttachments.size() > 1)
		{
			ATN_CORE_ASSERT(m_ColorAttachments.size() <= 4, "OpenGLFramebuffer supports only up to 4 color attachments!");
			GLenum buffers[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
			glDrawBuffers((GLsizei)m_ColorAttachments.size(), buffers);
		}
		else if(m_ColorAttachments.empty())
		{
			glDrawBuffer(GL_NONE);
		}

		ATN_CORE_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Framebuffer creation is failed!");

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void OpenGLFramebuffer::Resize(uint32 width, uint32 height)
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

	int OpenGLFramebuffer::ReadPixel(SIZE_T attachmentIndex, int x, int y)
	{
		ATN_CORE_ASSERT(attachmentIndex < m_ColorAttachments.size(), "ReadPixel() invalid attachmentIndex!");
		glReadBuffer(GL_COLOR_ATTACHMENT0 + (GLenum)attachmentIndex);
		int pixelData;
		glReadPixels(x, y, 1, 1, GL_RED_INTEGER, GL_INT, &pixelData);
		return pixelData;
	}

	void OpenGLFramebuffer::ClearAttachment(SIZE_T attachmentIndex, int value)
	{
		ATN_CORE_ASSERT(attachmentIndex < m_ColorAttachments.size(), "ClearAttachment() invalid attachmentIndex!");

		auto& desc = m_ColorAttachmentDescriptions[attachmentIndex];

		glClearTexImage(m_ColorAttachments[attachmentIndex], 0, TextureFormatToGLenum(desc.TextureFormat), GL_INT, &value);
	}

	void OpenGLFramebuffer::Bind() const
	{
		glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID);
		glViewport(0, 0, m_Description.Width, m_Description.Height);
	}

	void OpenGLFramebuffer::UnBind() const
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void OpenGLFramebuffer::DeleteAttachments()
	{
		glDeleteFramebuffers(1, &m_RendererID);
		glDeleteTextures((GLsizei)m_ColorAttachments.size(), m_ColorAttachments.data());
		glDeleteTextures(1, &m_DepthAttachment);
	}

	void OpenGLFramebuffer::AttachColorTexture(RendererID id, uint32 samples, unsigned int internalFormat, unsigned int format, uint32 width, uint32 height, SIZE_T index)
	{
		bool multisampled = samples > 1;
		if (multisampled)
		{
			glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, format, width, height, GL_FALSE);
		}
		else
		{
			glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, nullptr);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + (GLenum)index, TextureTarget(multisampled), id, 0);
	}

	void OpenGLFramebuffer::AttachDepthTexture(RendererID id, uint32 samples, unsigned int format, unsigned int attachmentType, uint32 width, uint32 height)
	{
		bool multisampled = samples > 1;
		if (multisampled)
		{
			glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, format, width, height, GL_FALSE);
		}
		else
		{
			glTexStorage2D(GL_TEXTURE_2D, 1, format, width, height);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}

		glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentType, TextureTarget(multisampled), id, 0);
	}
}
