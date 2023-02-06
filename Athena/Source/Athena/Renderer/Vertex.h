#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Math/Vector.h"

#include "GPUBuffers.h"


namespace Athena
{
	struct StaticVertex
	{
		Vector3 Position;
		Vector2 TexCoords;
		Vector3 Normal;
		Vector3 Tangent;
		Vector3 Bitangent;

		static inline BufferLayout GetLayout()
		{
			return 
			{
				{ ShaderDataType::Float3, "a_Position"  },
				{ ShaderDataType::Float2, "a_TexCoord"  },
				{ ShaderDataType::Float3, "a_Normal"    },
				{ ShaderDataType::Float3, "a_Tangent"   },
				{ ShaderDataType::Float3, "a_Bitangent" }
			};
		}
	};

	struct AnimVertex
	{
		Vector3 Position;
		Vector2 TexCoords;
		Vector3 Normal;
		Vector3 Tangent;
		Vector3 Bitangent;
		Vector4i BoneIDs;
		float Weights[4];

		static inline BufferLayout GetLayout()
		{
			return
			{
				{ ShaderDataType::Float3, "a_Position"  },
				{ ShaderDataType::Float2, "a_TexCoord"  },
				{ ShaderDataType::Float3, "a_Normal"    },
				{ ShaderDataType::Float3, "a_Tangent"   },
				{ ShaderDataType::Float3, "a_Bitangent" },
				{ ShaderDataType::Int4,	  "a_BoneIDs"   },
				{ ShaderDataType::Float4, "a_Weights"   },
			};
		}
	};
}
