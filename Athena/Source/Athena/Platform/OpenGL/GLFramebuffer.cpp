#include "GLFramebuffer.h"

#include <glad/glad.h>


namespace Athena
{
	static uint32 s_MaxFramebufferSize = 8192;


	static GLenum TextureTarget(bool multisample)
	{
		return multisample ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
	}

	static void CreateTextures(bool multisample, uint32* outIDs, SIZE_T count)
	{
		glCreateTextures(TextureTarget(multisample), (GLsizei)count, outIDs);
	}

	static void BindTexture(bool multisample, uint32 id)
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


	GLFramebuffer::GLFramebuffer(const FramebufferDescription& desc)
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

	void* GLFramebuffer::GetColorAttachmentRendererID(SIZE_T index) const
	{ 
		if (IsMultisample())
		{
			ATN_CORE_ASSERT(index < m_ColorAttachmentsResolved.size(), "subscript out of range");
			return reinterpret_cast<void*>((uint64)m_ColorAttachmentsResolved[index]);
		}

		ATN_CORE_ASSERT(index < m_ColorAttachments.size(), "subscript out of range");
		return reinterpret_cast<void*>((uint64)m_ColorAttachments[index]); 
	}

	int GLFramebuffer::ReadPixel(SIZE_T attachmentIndex, int x, int y)
	{
		ATN_CORE_ASSERT(attachmentIndex < m_ColorAttachments.size(), "Subscript out of range!");
		glBindFramebuffer(GL_FRAMEBUFFER, IsMultisample() ? m_ResolvedFramebufferID : m_FramebufferID); 

		glReadBuffer(GL_COLOR_ATTACHMENT0 + (GLenum)attachmentIndex);
		int pixelData;
		glReadPixels(x, y, 1, 1, GL_RED_INTEGER, GL_INT, &pixelData);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		return pixelData;
	}

	void GLFramebuffer::ClearAttachment(SIZE_T attachmentIndex, int value)
	{
		ATN_CORE_ASSERT(attachmentIndex < m_ColorAttachments.size(), "Subscript out of range!");
		
		auto& desc = m_ColorAttachmentDescriptions[attachmentIndex];
		
		glClearTexImage(m_ColorAttachments[attachmentIndex], 0, TextureFormatToGLenum(desc.TextureFormat), GL_INT, &value);
	}

	void GLFramebuffer::ClearColorAndDepth(const LinearColor& color)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, m_FramebufferID);
		glViewport(0, 0, m_Description.Width, m_Description.Height);

		glClearColor(color.r, color.g, color.b, color.a);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void GLFramebuffer::DeleteAttachments()
	{
		glDeleteFramebuffers(1, &m_FramebufferID);
		glDeleteTextures((GLsizei)m_ColorAttachments.size(), m_ColorAttachments.data());
		if (IsMultisample())
			glDeleteTextures((GLsizei)m_ColorAttachmentsResolved.size(), m_ColorAttachmentsResolved.data());
		glDeleteTextures(1, &m_DepthAttachment);
	}

	void GLFramebuffer::ResolveMutlisampling()
	{
		if (IsMultisample())
		{
			for (SIZE_T i = 0; i < m_ColorAttachmentDescriptions.size(); ++i)
			{
				glBindFramebuffer(GL_READ_FRAMEBUFFER, m_FramebufferID);
				glReadBuffer(GL_COLOR_ATTACHMENT0 + (GLenum)i);
				glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_ResolvedFramebufferID);
				glDrawBuffer(GL_COLOR_ATTACHMENT0 + (GLenum)i);

				glBlitFramebuffer(0, 0, m_Description.Width, m_Description.Height, 0, 0, m_Description.Width, m_Description.Height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
			}
		}
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
			CreateTextures(multisample, attachments.data(), attachments.size());
			//Attachments
			for (SIZE_T i = 0; i < attachments.size(); ++i)
			{
				BindTexture(multisample, attachments[i]);
				switch (m_ColorAttachmentDescriptions[i].TextureFormat)
				{
				case FramebufferTextureFormat::RGBA8:
					AttachColorTexture(attachments[i], samples, GL_RGBA8, GL_RGBA, m_Description.Width, m_Description.Height, i);
					break;

				case FramebufferTextureFormat::RED_INTEGER:
					AttachColorTexture(attachments[i], samples, GL_R32I, GL_RED_INTEGER, m_Description.Width, m_Description.Height, i);
					
				}
			}
		}

		if (m_DepthAttachmentDescription.TextureFormat != FramebufferTextureFormat::NONE)
		{
			CreateTextures(multisample, &m_DepthAttachment, 1);
			BindTexture(multisample, m_DepthAttachment);
			switch (m_DepthAttachmentDescription.TextureFormat)
			{
			case FramebufferTextureFormat::DEPTH24STENCIL8:
				AttachDepthTexture(m_DepthAttachment, samples, GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL_ATTACHMENT, m_Description.Width, m_Description.Height);
				break;
			}
		}

		ATN_CORE_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Framebuffer creation is failed!");
	}

	void GLFramebuffer::AttachColorTexture(uint32 id, uint32 samples, GLenum internalFormat, GLenum format, uint32 width, uint32 height, SIZE_T index)
	{
		if (samples > 1)
		{
			glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, internalFormat, width, height, GL_FALSE);
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

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + (GLenum)index, TextureTarget(samples > 1), id, 0);
	}

	void GLFramebuffer::AttachDepthTexture(uint32 id, uint32 samples, GLenum format, GLenum attachmentType, uint32 width, uint32 height)
	{
		if (samples > 1)
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

		glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentType, TextureTarget(samples > 1), id, 0);
	}
}
