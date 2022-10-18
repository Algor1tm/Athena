#include "ScriptEngine.h"

#ifdef _MSC_VER
	#pragma warning(push, 0)
#endif

#include <pybind11/embed.h>

#ifdef _MSC_VER
	#pragma warning(pop)
#endif

namespace py = pybind11;


namespace Athena
{
	struct ScriptEngineData
	{
		py::scoped_interpreter PythonInterpreter;
	};

	static ScriptEngineData* s_Data;

	void ScriptEngine::Init()
	{
		s_Data = new ScriptEngineData();

		py::module_ sys = py::module_::import("sys");
		auto& path = sys.attr("path");
		path.attr("insert")(0, "Assets/Scripts/");

		py::module_ athena = py::module_::import("Athena");
		ATN_CORE_ASSERT(athena);
	}

	void ScriptEngine::Shutdown()
	{
		delete s_Data;
	}
}
