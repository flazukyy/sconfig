#include "SConfig.h"
#include <iostream>

int main() {
	sconfig config{ R"(test.srip)", true }; 

	config.add_field("test_bool", true);
	bool x{ config.field_get_value<bool>("test_bool") };

	std::cout << x << '\n';

	config.remove_field("test_bool");

	try {
		bool y{ config.field_get_value<bool>("test_bool") };
	}
	catch (const sconfig_field_not_found& e) {
		std::cout << e.what() << '\n';
	}

	return 0;
}