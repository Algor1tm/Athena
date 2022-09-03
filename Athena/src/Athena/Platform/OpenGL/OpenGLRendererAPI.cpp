#include "atnpch.h"
#include "OpenGLRendererAPI.h"

#include <glad/glad.h>


namespace Athena
{
	void OpenGLMessageCallback(
		unsigned source,
		unsigned type,
		unsigned id,
		unsigned severity,
		int length,
		const char* message,
		const void* userParam)
	{
		switch (severity)
		{
		case GL_DEBUG_SEVERITY_HIGH:         ATN_CORE_FATAL(message); return;
		case GL_DEBUG_SEVERITY_MEDIUM:       ATN_CORE_ERROR(message); return;
		case GL_DEBUG_SEVERITY_LOW:          ATN_CORE_WARN(message); return;
		case GL_DEBUG_SEVERITY_NOTIFICATION: ATN_CORE_TRACE(message); return;
		}

		ATN_CORE_ASSERT(false, "Unknown severity level!");
	}

	void OpenGLRendererAPI::Init()
	{
		ATN_PROFILE_FUNCTION();

#ifdef ATN_DEBUG
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(OpenGLMessageCallback, nullptr);

		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, NULL, GL_FALSE);
#endif

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_LINE_SMOOTH);
	}

	void OpenGLRendererAPI::SetViewport(uint32 x, uint32 y, uint32 width, uint32 height)
	{
		glViewport(x, y, width, height);
	}

	void OpenGLRendererAPI::Clear(const LinearColor& color)
	{
		glClearColor(color.r, color.g, color.g, color.a);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void OpenGLRendererAPI::DrawTriangles(const Ref<VertexArray>& vertexArray, uint32 indexCount)
	{
		vertexArray->Bind();
		uint32 count = indexCount ? indexCount : vertexArray->GetIndexBuffer()->GetCount();
		glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, nullptr);
	}

	void OpenGLRendererAPI::DrawLines(const Ref<VertexArray>& vertexArray, uint32 vertexCount)
	{
		vertexArray->Bind();
		glDrawArrays(GL_LINES, 0, vertexCount);
	}

	void OpenGLRendererAPI::SetLineWidth(float width)
	{
		glLineWidth(width);
	}
}
