#include "GLRendererAPI.h"

#include "Athena/Platform/OpenGL/GLUtils.h"

#include <glad/glad.h>


namespace Athena
{
	static void GLMessageCallback(
		unsigned source,
		unsigned type,
		unsigned id,
		unsigned severity,
		int length,
		const char* message,
		const void* userParam)
	{
		// HACK
		const char* messageToAvoid = "Program/shader state performance warning:";
		if (length >= strlen(messageToAvoid) && strncmp(message, messageToAvoid, strlen(messageToAvoid)) == 0)
			return;

		switch (severity)
		{
		case GL_DEBUG_SEVERITY_HIGH:         ATN_CORE_FATAL(message); return;
		case GL_DEBUG_SEVERITY_MEDIUM:       ATN_CORE_ERROR(message); return;
		case GL_DEBUG_SEVERITY_LOW:          ATN_CORE_WARN(message); return;
		case GL_DEBUG_SEVERITY_NOTIFICATION: ATN_CORE_TRACE(message); return;
		}

		ATN_CORE_ASSERT(false, "Unknown severity level!");
	}

	void GLRendererAPI::Init()
	{
#ifdef ATN_DEBUG
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(GLMessageCallback, nullptr);

		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, NULL, GL_FALSE);
#endif

		glEnable(GL_LINE_SMOOTH);
		glEnable(GL_MULTISAMPLE);
		glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
	}

	void GLRendererAPI::BindPipeline(const Pipeline& pipeline)
	{
		if (pipeline.CullFace != CullFace::NONE)
		{
			glEnable(GL_CULL_FACE);

			glCullFace(Utils::CullFaceToGLenum(pipeline.CullFace));
			glFrontFace(Utils::CullDirectionToGLenum(pipeline.CullDirection));
		}
		else
		{
			glDisable(GL_CULL_FACE);
		}

		if (pipeline.DepthFunc != DepthFunc::NONE)
		{
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(Utils::DepthFuncToGLenum(pipeline.DepthFunc));
		}
		else
		{
			glDisable(GL_DEPTH_TEST);
		}


		if (pipeline.BlendFunc != BlendFunc::NONE)
		{
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, Utils::BlendFuncToGLenum(pipeline.BlendFunc));
		}
		else
		{
			glDisable(GL_BLEND);
		}
	}

	void GLRendererAPI::BeginRenderPass(const RenderPass& pass)
	{
		m_CurrentRenderPass = pass;
		const auto& fbInfo = pass.TargetFramebuffer->GetCreateInfo();

		pass.TargetFramebuffer->Bind();
		glViewport(0, 0, fbInfo.Width, fbInfo.Height);

		if (pass.ClearBit != CLEAR_NONE_BIT)
		{
			GLbitfield bitfield = 0;
			if (pass.ClearBit | CLEAR_COLOR_BIT)
			{
				bitfield |= GL_COLOR_BUFFER_BIT;
				glClearColor(pass.ClearColor.r, pass.ClearColor.g, pass.ClearColor.b, pass.ClearColor.a);
			}

			if (pass.ClearBit | CLEAR_DEPTH_BIT)
				bitfield |= GL_DEPTH_BUFFER_BIT;

			if (pass.ClearBit | CLEAR_STENCIL_BIT)
				bitfield |= GL_STENCIL_BUFFER_BIT;

			glClear(bitfield);
		}
	}

	void GLRendererAPI::EndRenderPass()
	{
		m_CurrentRenderPass.TargetFramebuffer->UnBind();
	}

	void GLRendererAPI::BeginComputePass(const ComputePass& pass)
	{
		m_CurrentComputePass = pass;
	}

	void GLRendererAPI::EndComputePass()
	{

	}

	void GLRendererAPI::DrawTriangles(const Ref<VertexBuffer>& vertexBuffer, uint32 indexCount)
	{
		vertexBuffer->Bind();
		uint32 count = indexCount ? indexCount : vertexBuffer->GetIndexBuffer()->GetCount();
		glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, nullptr);
	}

	void GLRendererAPI::DrawLines(const Ref<VertexBuffer>& vertexBuffer, uint32 vertexCount)
	{
		vertexBuffer->Bind();
		glDrawArrays(GL_LINES, 0, vertexCount);
	}

	void GLRendererAPI::Dispatch(uint32 x, uint32 y, uint32 z, Vector3i workGroupSize)
	{
		glDispatchCompute(Math::Ceil((float)x / workGroupSize.x), Math::Ceil((float)y / workGroupSize.y), Math::Ceil((float)z / workGroupSize.z));

		uint32 barrier = m_CurrentComputePass.BarrierBit;
		if (barrier == ALL_BARRIERS)
		{
			glMemoryBarrier(GL_ALL_BARRIER_BITS);
		}
		else if(barrier != BARRIER_BIT_NONE)
		{
			GLbitfield bitfield = 0;
			if (barrier | BUFFER_UPDATE_BARRIER_BIT)
				bitfield |= GL_BUFFER_UPDATE_BARRIER_BIT;

			if (barrier | FRAMEBUFFER_BARRIER_BIT)
				bitfield |= GL_FRAMEBUFFER_BARRIER_BIT;

			if (barrier | SHADER_IMAGE_BARRIER_BIT)
				bitfield |= GL_SHADER_IMAGE_ACCESS_BARRIER_BIT;

			glMemoryBarrier(bitfield);
		}
	}
}
