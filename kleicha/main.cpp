#include "Kleicha.h"
#include "format.h"
#include <stdexcept>

int main()
{
	try {
		Kleicha kleicha{};
		kleicha.init();

		kleicha.cleanup();
	}
	catch (const std::exception& e) {
		fmt::println("[Exception] {}", e.what());
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}