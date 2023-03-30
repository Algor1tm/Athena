#pragma once

// For use by Athena applications

// --------Core--------------------
#include "Athena/Core/Application.h"
#include "Athena/Core/FileSystem.h"
#include "Athena/Core/Layer.h"
#include "Athena/Core/UUID.h"
#include "Athena/Core/Log.h"
#include "Athena/Core/Time.h"


// --------ImGui--------------------
#include "Athena/ImGui/ImGuiLayer.h"


// --------Input--------------------
#include "Athena/Input/ApplicationEvent.h"
#include "Athena/Input/Event.h"
#include "Athena/Input/KeyEvent.h"
#include "Athena/Input/MouseEvent.h"

#include "Athena/Input/Input.h"
#include "Athena/Input/Keyboard.h"
#include "Athena/Input/Mouse.h"


// --------Math--------------------
#include "Athena/Math/Vector.h"
#include "Athena/Math/Matrix.h"
#include "Athena/Math/Quaternion.h"
#include "Athena/Math/Random.h"
#include "Athena/Math/Transforms.h"
#include "Athena/Math/Projections.h"
#include "Athena/Math/TypeCasts.h"
#include "Athena/Math/Constants.h"
#include "Athena/Math/Limits.h"


// --------Renderer--------------------
#include "Athena/Renderer/AABB.h"
#include "Athena/Renderer/Animation.h"
#include "Athena/Renderer/Color.h"
#include "Athena/Renderer/Framebuffer.h"
#include "Athena/Renderer/GPUBuffers.h"
#include "Athena/Renderer/Material.h"
#include "Athena/Renderer/Mesh.h"
#include "Athena/Renderer/Shader.h"
#include "Athena/Renderer/EnvironmentMap.h"
#include "Athena/Renderer/Texture.h"

#include "Athena/Renderer/RenderCommand.h"
#include "Athena/Renderer/Renderer.h"
#include "Athena/Renderer/Renderer2D.h"

#include "Athena/Renderer/EditorCamera.h"


// --------Scene--------------------
#include "Athena/Scene/Components.h"
#include "Athena/Scene/Entity.h"
#include "Athena/Scene/NativeScript.h"
#include "Athena/Scene/Scene.h"
#include "Athena/Scene/SceneCamera.h"
#include "Athena/Scene/SceneSerializer.h"


// --------Scripting--------------------
#include "Athena/Scripting/PublicScriptEngine.h"
