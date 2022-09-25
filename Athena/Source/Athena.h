#pragma once

// For use by Athena applications

// --------Core--------------------
#include "Athena/Core/Application.h"
#include "Athena/Core/Color.h"
#include "Athena/Core/Layer.h"
#include "Athena/Core/Log.h"
#include "Athena/Core/Time.h"

#include "Athena/ImGui/ImGuiLayer.h"

#include "Athena/Input/Events/ApplicationEvent.h"
#include "Athena/Input/Events/KeyEvent.h"
#include "Athena/Input/Events/MouseEvent.h"

#include "Athena/Input/Input.h"
#include "Athena/Input/Keyboard.h"
#include "Athena/Input/Mouse.h"


// --------Scene--------------------
#include "Athena/Scene/Components.h"
#include "Athena/Scene/NativeScript.h"
#include "Athena/Scene/Entity.h"
#include "Athena/Scene/Scene.h"
#include "Athena/Scene/SceneCamera.h"
#include "Athena/Scene/SceneSerializer.h"


// --------Renderer--------------------
#include "Athena/Renderer/Renderer.h"
#include "Athena/Renderer/Renderer2D.h"
#include "Athena/Renderer/RenderCommand.h"

#include "Athena/Renderer/Buffer.h"
#include "Athena/Renderer/Framebuffer.h"
#include "Athena/Renderer/ConstantBuffer.h"
#include "Athena/Renderer/Shader.h"
#include "Athena/Renderer/Texture.h"

#include "Athena/Renderer/OrthographicCamera.h"
#include "Athena/Renderer/OrthographicCameraController.h"


// --------Math--------------------
#include "Athena/Math/Vector.h"
#include "Athena/Math/Matrix.h"
#include "Athena/Math/Quaternion.h"
#include "Athena/Math/Random.h"
#include "Athena/Math/Transforms.h"
#include "Athena/Math/Projections.h"
#include "Athena/Math/Constants.h"
