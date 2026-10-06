#pragma once

#include <string_view>

namespace Qiantong
{
// Library version, not a gameplay/save/replay schema version.
std::string_view CoreVersion() noexcept;
}
