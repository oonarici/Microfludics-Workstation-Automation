#include "microfluidics/app.h"

#include <cassert>
#include <string>

int main() {
    const std::string name = microfluidics::app_name();
    assert(!name.empty());
    assert(name == "Microfluidics Automation Tool");
    return 0;
}
