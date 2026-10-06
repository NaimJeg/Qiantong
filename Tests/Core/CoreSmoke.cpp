#include "Qiantong/CoreVersion.h"

#include <string_view>

int main()
{
    // A real link to the standalone library, with no Unreal include directories.
    return Qiantong::CoreVersion() == std::string_view("0.1.0") ? 0 : 1;
}
