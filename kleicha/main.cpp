#include "Kleicha.h"

#pragma warning(push, 0)
#pragma warning(disable : 6285 26498)
#include "format.h"
#pragma warning(pop)

#include <stdexcept>

int main()
{
	Kleicha kleicha{};
	try {
		kleicha.init();
		kleicha.start();
		kleicha.cleanup();
	}
	catch (const std::exception& e) {
		fmt::println("[Exception] {}", e.what());
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}