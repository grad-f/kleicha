#include "Kleicha.h"
#include "format.h"
#include <stdexcept>

int main()
{
	try {
		Kleicha kleicha{};
		kleicha.init();
	}
	catch (const std::exception& e) {
		fmt::println("{}", e.what());
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}