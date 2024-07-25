#pragma once

// For use by Athena Scripting

// --------Core--------------------
#include "Athena/Core/Core.h"
#include "Athena/Core/FileSystem.h"
#include "Athena/Core/UUID.h"
#include "Athena/Core/Log.h"
#include "Athena/Core/Time.h"


// --------Input--------------------
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
#include "Athena/Math/Constants.h"
#include "Athena/Math/Limits.h"


// --------Renderer--------------------
#include "Athena/Renderer/Animation.h"
#include "Athena/Renderer/Color.h"
#include "Athena/Renderer/Material.h"
#include "Athena/Renderer/Mesh.h"

// --------Scene--------------------
#include "Athena/Scene/Components.h"
#include "Athena/Scene/Entity.h"
#include "Athena/Scene/Scene.h"


// --------Scripting--------------------
#include "Athena/Scripting/ScriptEngine.h"
#include "Athena/Scripting/Script.h"


using Athena::byte;

using Athena::int8;
using Athena::int16;
using Athena::int32;
using Athena::int64;

using Athena::uint8;
using Athena::uint16;
using Athena::uint32;
using Athena::uint64;

using Athena::String;
using Athena::FilePath;
