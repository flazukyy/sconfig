#ifndef SCONFIG
#define SCONFIG
#include <stdexcept>
#include <fstream>
#include <filesystem>
#include <string>
#include <ranges>
#include <compare>

class [[nodiscard]] sconfig_field_not_found final : public std::runtime_error{
public:
	explicit sconfig_field_not_found(const std::string& err)
		: std::runtime_error{ err }
	{
	}
};

class sconfig;

template <typename _Ty>
	requires (!std::is_pointer_v<_Ty> && !std::is_reference_v<_Ty>)
class sfield {
	sconfig& _config;
	const std::string _field_name;
	_Ty _value;

	sfield(sconfig& config, const std::string& name, const _Ty& val)
		: _config(config), _field_name(name), _value(val)
	{
	}
public:
	using value_type = _Ty;

	sfield() = delete;

	[[nodiscard]] const _Ty& get_value() const noexcept {
		return _value;
	}

	void set_value(const _Ty& val) {
		_config.field_set_value(_field_name, val);
		_value = val;
	}

	[[nodiscard]] std::string get_config_path() const {
		return _config._path;
	}

	[[nodiscard]] std::string get_name() const noexcept {
		return _field_name;
	}

	void remove_field() {
		_config.remove_field(_field_name);
	}

	[[nodiscard]] bool operator==(const sfield&) const noexcept = default;
	[[nodiscard]] bool operator!=(const sfield&) const noexcept = default;
	[[nodiscard]] bool operator>=(const sfield&) const noexcept = default;
	[[nodiscard]] bool operator<=(const sfield&) const noexcept = default;
	[[nodiscard]] bool operator>(const sfield&) const noexcept = default;
	[[nodiscard]] bool operator<(const sfield&) const noexcept = default;
	[[nodiscard]] auto operator<=>(const sfield&) const noexcept = default;

	friend class sconfig;
};

class sconfig {
	const std::string _path;
	std::fstream _stream;
public:
	explicit sconfig(const std::string& path, bool create_if_not_found = false)
		: _path{ path } 
	{
		if (!std::filesystem::exists({ path }) && !create_if_not_found) 
			throw std::runtime_error{ path + " is an invalid path." };

		std::string line{};
		
		_stream = std::fstream{ path, std::ios::app | std::ios::out | std::ios::in };
		std::getline(_stream, line);

		if (line[0] != '{') {
			std::ofstream write{ path, std::ios::trunc };
			write << "{\n}";
		}
	}

	[[nodiscard]] bool field_exists(const std::string& field_name) {
		_stream.clear();
		_stream.seekp(0);

		std::string line{};
		while (std::getline(_stream, line)) {
			if (line.find(field_name + ": ") != std::string::npos)
				return true;
		}

		return {};
	}

	[[nodiscard]] bool field_has_value(const std::string& field_name) {
		_stream.clear();
		_stream.seekp(0);

		std::string line{};
		while (std::getline(_stream, line)) {
			if (line.find(field_name + ": ") != std::string::npos) {
				try {
					char c{ line.at(line.find(field_name) + field_name.length() + 3) };
					return true;
				}
				catch (...) {
					return {};
				}
			}
		}

		return {};
	}

	template <typename _Ty>
		requires (!std::is_pointer_v<_Ty> && !std::is_reference_v<_Ty>)
	[[nodiscard]] _Ty field_get_value(const std::string& field_name) {
		_stream.clear();
		_stream.seekp(0);

		std::string line{};
		while (std::getline(_stream, line)) {
			if (line.find(field_name + ": ") != std::string::npos) {
				std::string value_str{ line.substr(line.find(field_name) + field_name.length() + 2) };

				if constexpr (std::is_same_v<std::decay_t<_Ty>, bool>) {
					if (value_str[0] == 't') return true;
					return {};
				}
				else if constexpr (std::is_same_v<std::decay_t<_Ty>, float>)
					return std::stof(value_str);
				else if constexpr (std::is_same_v<std::decay_t<_Ty>, double>)
					return std::stod(value_str);
				else if constexpr (std::is_same_v<std::decay_t<_Ty>, int>)
					return std::stoi(value_str);
				else if constexpr (std::is_same_v<std::decay_t<_Ty>, long>)
					return std::stol(value_str);
				else if constexpr (std::is_same_v<std::decay_t<_Ty>, long long>)
					return std::stoll(value_str);
				else if constexpr (std::is_same_v<std::decay_t<_Ty>, long double>)
					return std::stold(value_str);
				else if constexpr (std::is_same_v<std::decay_t<_Ty>, unsigned long>)
					return std::stoul(value_str);
				else if constexpr (std::is_same_v<std::decay_t<_Ty>, unsigned long long>)
					return std::stoull(value_str);
				else if constexpr (std::is_same_v<std::decay_t<_Ty>, char>)
					return value_str[0];
				else if constexpr (std::is_same_v<std::decay_t<_Ty>, const char*>)
					return value_str.c_str();
				else if constexpr (std::is_same_v<std::decay_t<_Ty>, std::string>)
					return value_str;
			}
		}

		throw sconfig_field_not_found{ "field not found." };
	}

	template <typename _Ty> 
		requires (!std::is_pointer_v<_Ty> && !std::is_reference_v<_Ty>)
	void field_set_value(const std::string& field_name, const _Ty& val) {
		_stream.clear();
		_stream.seekp(0);

		std::vector<std::string> lines{};
		std::string line{};

		bool found_line{};

		while (std::getline(_stream, line)) {
			if (line.find(field_name + ": ") != std::string::npos) {
				found_line = true;
				line = line.substr(0, line.find(field_name) + field_name.length()) + ": ";

				if constexpr (std::is_same_v<std::decay_t<_Ty>, bool>)
					line += val ? "true" : "false";
				else if constexpr (std::is_arithmetic_v<std::decay_t<_Ty>>)
					line += std::to_string(val);
				else if constexpr (std::is_same_v<std::decay_t<_Ty>, const char*>)
					line += val;
				else if constexpr (std::is_same_v<std::decay_t<_Ty>, std::string>)
					line += val;
			}

			lines.emplace_back(line + '\n');
		}

		if (!found_line)
			throw sconfig_field_not_found{ "field not found." };

		std::ofstream out{ _path, std::ios::trunc };
		for (const std::string& line : lines) 
			out << line;
	}

	template <typename _Ty>
		requires (!std::is_pointer_v<_Ty> && !std::is_reference_v<_Ty>)
	void add_field(const std::string& field_name, const _Ty& val) {
		_stream.clear();
		_stream.seekp(0);

		std::vector<std::string> lines{};
		std::string line{};
		std::string last_line{};

		std::string name{ "    " + field_name + ": " };

		while (std::getline(_stream, line)) {
			if (line == "}") {
				if constexpr (std::is_same_v<std::decay_t<_Ty>, bool>)
					lines.emplace_back(name + (val ? "true" : "false") + '\n');
				else if constexpr (std::is_arithmetic_v<std::decay_t<_Ty>>)
					lines.emplace_back(name + std::to_string(val) + '\n');
				else if constexpr (std::is_same_v<std::decay_t<_Ty>, const char*>)
					lines.emplace_back(name + val + '\n');
				else if constexpr (std::is_same_v<std::decay_t<_Ty>, std::string>)
					lines.emplace_back(name + val + '\n');
			}

			last_line = line;
			lines.emplace_back(line + '\n');
		}

		std::ofstream out{ _path, std::ios::trunc };
		for (const std::string& line : lines)
			out << line;
	}

	void remove_field(const std::string& field_name) {
		_stream.clear();
		_stream.seekp(0);

		std::vector<std::string> lines{};
		std::string line{};

		while (std::getline(_stream, line)) {
			if (line.find(field_name + ": ") == std::string::npos)
				lines.emplace_back(line + '\n');
		}

		std::ofstream out{ _path, std::ios::trunc };
		for (const std::string& line : lines)
			out << line;
	}

	template <typename _Ty>
		requires (!std::is_pointer_v<_Ty> && !std::is_reference_v<_Ty>)
	[[nodiscard]] sfield<_Ty> get_field(const std::string& field_name) {
		return sfield<_Ty>{ *this, field_name, field_get_value<_Ty>(field_name) };
	}
};
#endif