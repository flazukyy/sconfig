#include "SConfig.h"
#include <iostream>

int main() {
	sconfig config{ R"(test.srip)", true };

	config.add_field("test_field", 20);

	int x{ config.field_get_value<int>("test_field") };

	std::cout << x << '\n';

	sfield field{ config.get_field<int>("test_field") };

	field.remove_field();

	try {
		int test_field{ config.field_get_value<int>("test_field") };
	} 
	catch (const sconfig_field_not_found& e) {
		std::cout << e.what() << '\n';
	}

	sconfig::create_config( R"(test2.srip)",
		std::pair<std::string, int>{ "field_1", 2 },
		std::pair<std::string, bool>{ "field_2", true }
	);

	sconfig config2{ R"(test2.srip)" };

	std::cout << config2.field_get_value<int>("field_1") << '\n';

	sconfig::delete_config(R"(test2.srip)");

	return 0;
}