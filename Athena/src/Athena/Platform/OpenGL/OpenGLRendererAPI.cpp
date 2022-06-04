#include "atnpch.h"
#include "OpenGLRendererAPI.h"

#include "glad/glad.h"


namespace Athena
{
	void OpenGLRendererAPI::Clear(const Color& color)
	{
		glClearColor(color.r, color.g, color.g, color.a);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void OpenGLRendererAPI::DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray)
	{
		glDrawElements(GL_TRIANGLES, vertexArray->GetIndexBuffer()->GetCount(), GL_UNSIGNED_INT, nullptr);
	}
}
