#include "SceneRenderer.h"
#include "Shader.h"


namespace Athena
{
	class TestShaderVS : public Shader
	{
	public:
		virtual PermutationDomain BuildPermutationDomain() override
		{
			PermutationDomain domain;
			//ADD_PERMUTATION_BOOL(domain, "MOBILE");
			//ADD_PERMUTATION_ARRAY(domain, "QUALITY", 1, 2, 3, 4, 5);

			//domain.Parameters = {
			//	{ "QUALITY", { 1, 2, 3, 4, 5 }, 1 },
			//	{ "MOBILE", { 0, 1 }, 0 }
			//};
			//
			return domain;
		}

		virtual void ModifyCompilerEnvironment(const PermutationVector& permutation, ShaderCompilerEnvironment& env) override
		{
			env.Enable16BitTypes = true;
		}
	};

	IMPLEMENT_SHADER(TestVS, TestShaderVS, "Test.hlsl", "VSMain", ShaderType::VertexShader);
	IMPLEMENT_SHADER(TestPS, Shader, "Test.hlsl", "PSMain", ShaderType::PixelShader);
}
