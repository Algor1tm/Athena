#pragma once

#include <Athena.h>

using namespace Athena;


namespace NamespaceName
{
	class ClassName: public Script
	{
	public:
		ClassName();

	private:
		virtual void OnCreate() override;
		virtual void OnUpdate(Time frameTime) override;
		virtual void GetFieldsDescription(ScriptFieldMap* outFields) override;

	private:

	};
}
