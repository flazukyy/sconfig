#ifndef SCONFIG
#define SCONFIG
#include <stdexcept>
#include <fstream>
#include <filesystem>
#include <string>
#include <ranges>
#include <compare>
#include <iostream>

class [[nodiscard]] sconfig_field_not_found final : public std::runtime_error{
public:
	explicit sconfig_field_not_found()
		: std::runtime_error{ "field not found." }
	{
	}
};

template <typename T>
concept sconfig_type_is_valid_get =
std::is_object_v<T> &&
!std::is_const_v<T> &&
!std::is_volatile_v<T> &&
!std::is_abstract_v<T> &&
!std::is_pointer_v<T>;

template <typename T>
concept sconfig_type_is_valid =
std::is_object_v<T> &&
!std::is_const_v<T> &&
!std::is_volatile_v<T> &&
!std::is_abstract_v<T> &&
(!std::is_pointer_v<T> || std::is_same_v<T, const char*>);

template <typename T>
concept sconfig_name_and_val_pair = requires
{ typename T::first_type; typename T::second_type; }
&& std::same_as<typename T::first_type, std::string>;

class sconfig;

template <typename _Ty>
	requires sconfig_type_is_valid_get<_Ty>
class sfield {
public:
	using value_type = _Ty;
	using reference = _Ty&;
	using const_reference = const _Ty&;
	using difference_type = std::ptrdiff_t;
private:
	sconfig& _config;
	const std::string _field_name;
	value_type _value;

	sfield(sconfig& config, const std::string& name, const_reference val)
		: _config{ config }, _field_name{ name }, _value{ val }
	{
	}
public:
	void set_value(const_reference val) {
		_config.field_set_value(_field_name, val);
		_value = val;
	}

	void remove_field() {
		_config.remove_field(_field_name);
	}

	[[nodiscard]] const_reference get_value() const noexcept {
		return _value;
	}

	[[nodiscard]] reference get_value() noexcept {
		return _value;
	}

	[[nodiscard]] const std::string& get_config_path() const noexcept {
		return _config._path;
	}

	[[nodiscard]] std::string& get_config_path() noexcept {
		return _config._path;
	}

	[[nodiscard]] const std::string& get_name() const noexcept {
		return _field_name;
	}

	[[nodiscard]] std::string& get_name() noexcept {
		return _field_name;
	}

	sfield& operator=(const_reference val) {
		set_value(val);
		return *this;
	}

	[[nodiscard]] operator value_type() const noexcept {
		return _value;
	}

	[[nodiscard]] bool operator==(const sfield& other) const noexcept {
		return _field_name == other._field_name && _value == other._value;
	}

	[[nodiscard]] bool operator!=(const sfield& other) const noexcept {
		return _field_name != other._field_name && _value != other._value;
	}

	[[nodiscard]] bool operator>=(const sfield& other) const noexcept {
		return _field_name >= other._field_name && _value >= other._value;
	}

	[[nodiscard]] bool operator<=(const sfield& other) const noexcept {
		return _field_name <= other._field_name && _value <= other._value;
	}

	[[nodiscard]] bool operator>(const sfield& other) const noexcept {
		return _field_name > other._field_name && _value > other._value;
	}

	[[nodiscard]] bool operator<(const sfield& other) const noexcept {
		return _field_name < other._field_name && _value < other._value;
	}

	[[nodiscard]] auto operator<=>(const sfield& other) const noexcept {
		return _field_name <=> other._field_name && _value <=> other._value;
	}

	friend class sconfig;
};

class sconfig {
	const std::string _path;
	std::fstream _stream;

	void create_new(const std::string& path) {
		_stream = std::fstream{ path, std::ios::app | std::ios::out | std::ios::in };
		_stream << "{\n}";
	}
public:
	explicit sconfig(const std::string& path, bool create_if_not_found = false)
		: _path{ path }, _stream{}
	{
		if (!std::filesystem::exists({ path })) {
			if (create_if_not_found) { create_new(path); return; }
			else throw std::runtime_error{ path + " not found." };
		}

		_stream = std::fstream{ path, std::ios::app | std::ios::out | std::ios::in };
	}

	explicit sconfig(const std::filesystem::path& path, bool create_if_not_found = false)
		: _path{ path.string() }, _stream{}
	{
		if (!std::filesystem::exists({ path })) {
			if (create_if_not_found) { create_new(path.string()); return; }
			else throw std::runtime_error{ path.string() + " not found." };
		}

		_stream = std::fstream{ path, std::ios::app | std::ios::out | std::ios::in };
	}

	template <typename... Args>
		requires (sconfig_name_and_val_pair<std::remove_cvref_t<Args>> && ...)
	sconfig(const std::string& path, Args&&... args)
		: _path{ path }, _stream{}
	{
		if (std::filesystem::exists(path)) return;
		create_new(path);
		(add_field(args.first, args.second), ...);
	}

	static void create_config(const std::string& path) {
		if (std::filesystem::exists(path)) return;

		std::ofstream stream = std::ofstream{ path };
		stream << "{\n}";
	}

	template <typename... Args>
		requires (sconfig_name_and_val_pair<std::remove_cvref_t<Args>> && ...)
	static void create_config(const std::string& path, Args&&... args) {
		sconfig tmp{ path, std::forward<Args>(args)... };
	}

	static void delete_config(const std::string& path) {
		std::filesystem::remove({ path });
	}

	static void delete_config(sconfig& config) {
		config._stream = {};
		std::filesystem::remove({ config._path });
	}

	[[nodiscard]] static bool config_exists(const std::string& path) {
		return std::filesystem::exists({ path });
	}

	[[nodiscard]] static bool config_exists(const sconfig& config) {
		return std::filesystem::exists({ config._path });
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

	[[nodiscard]] bool field_contains_value(const std::string& field_name) {
		_stream.clear();
		_stream.seekp(0);

		std::string line{};
		while (std::getline(_stream, line)) {
			if (line.find(field_name + ": ") != std::string::npos) {
				try {
					char tmp_c{ line.at(line.find(field_name) + field_name.length() + 2) };
					if (tmp_c == ' ') return {};
					else return true;
				}
				catch (...) {
					return {};
				}
			}
		}

		throw sconfig_field_not_found{};

		return {};
	}

	template <typename _Ty>
		requires sconfig_type_is_valid_get<_Ty>
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
				else if constexpr (std::is_same_v<std::decay_t<_Ty>, std::string>)
					return value_str;
			}
		}

		throw sconfig_field_not_found{};
	}

	template <typename _Ty>
		requires sconfig_type_is_valid<_Ty>
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
			throw sconfig_field_not_found{};

		std::ofstream out{ _path, std::ios::trunc };
		for (const std::string& line : lines)
			out << line;
	}

	template <typename _Ty>
		requires sconfig_type_is_valid_get<_Ty>
	[[nodiscard]] bool field_check_value(const std::string& field_name, const _Ty& val) {
		_stream.clear();
		_stream.seekp(0);

		_Ty field_value{ field_get_value(field_name) };

		return val == field_value;
	}

	template <typename _Ty>
		requires sconfig_type_is_valid<_Ty>
	void add_field(const std::string& field_name, const _Ty& val) {
		_stream.clear();
		_stream.seekp(0);

		std::vector<std::string> lines{};
		std::string line{};

		while (std::getline(_stream, line)) {
			if (line == "}") {
				std::string full_line{ field_name + ": " };

				if constexpr (std::is_same_v<std::decay_t<_Ty>, bool>)
					full_line += (val ? "true" : "false");
				else if constexpr (std::is_arithmetic_v<std::decay_t<_Ty>>)
					full_line += std::to_string(val);
				else if constexpr (std::is_same_v<std::decay_t<_Ty>, const char*>)
					full_line += val;
				else if constexpr (std::is_same_v<std::decay_t<_Ty>, std::string>)
					full_line += val;

				lines.emplace_back(full_line);
			}

			lines.emplace_back(line);
		}

		std::ofstream out{ _path };

		for (const std::string& line : lines)
			out << line << '\n';
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

	void delete_config() {
		_stream = {};
		std::filesystem::remove({ _path });
	}

	template <typename _Ty>
		requires sconfig_type_is_valid_get<_Ty>
	[[nodiscard]] sfield<_Ty> get_field_instance(const std::string& field_name) {
		return sfield<_Ty>{ *this, field_name, field_get_value<_Ty>(field_name) };
	}

	[[nodiscard]] std::filesystem::path get_path() const noexcept {
		return { _path };
	}

	[[nodiscard]] explicit operator std::string_view() const noexcept {
		return _path.c_str();
	}

	[[nodiscard]] explicit operator std::string() const {
		return _path;
	}

	[[nodiscard]] explicit operator const char* () const noexcept {
		return _path.c_str();
	}

	[[nodiscard]] explicit operator std::filesystem::path() const {
		return { _path };
	}

	[[nodiscard]] bool operator==(const sconfig& other) const noexcept {
		return _path == other._path;
	}

	[[nodiscard]] bool operator!=(const sconfig& other) const noexcept {
		return _path != other._path;
	}
};
#endif
