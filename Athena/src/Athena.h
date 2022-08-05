#pragma once

// For use by Athena applications

#include "Athena/Core/Application.h"
#include "Athena/Core/Layer.h"
#include "Athena/ImGui/ImGuiLayer.h"
#include "Athena/Core/Log.h"

#include "Athena/Core/Time.h"

#include "Athena/Core/Input.h"
#include "Athena/Core/KeyCodes.h"
#include "Athena/Core/MouseCodes.h"

// ---Renderer--------------------
#include "Athena/Renderer/Renderer.h"
#include "Athena/Renderer/Renderer2D.h"
#include "Athena/Renderer/RenderCommand.h"

#include "Athena/Renderer/OrthographicCamera.h"
#include "Athena/Core/OrthographicCameraController.h"

#include "Athena/Renderer/Color.h"
#include "Athena/Renderer/Shader.h"
#include "Athena/Renderer/Texture.h"
#include "Athena/Renderer/Framebuffer.h"
#include "Athena/Renderer/VertexArray.h"
#include "Athena/Renderer/Buffer.h"

// ---Math--------------------
#include "Athena/Math/Vector.h"
#include "Athena/Math/Matrix.h"
#include "Athena/Math/Utils.h"
