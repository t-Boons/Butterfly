#pragma once
#include "Core/Common.hpp"

namespace Butterfly
{
	struct NameComponent
	{
		NameComponent(const Entity& entity)
		{
		}

		std::string Tag;
		std::string Name;
	};
}