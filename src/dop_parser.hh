/**
 * Copyright (c) Tuukka Norri 2019
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef INDEX_IMAGES_DOP_PARSER_HH
#define INDEX_IMAGES_DOP_PARSER_HH

#include <boost/variant/get.hpp>
#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/home/x3/support/ast/variant.hpp>
#include <cstdint>
#include <map>
#include <ostream>
#include <string>
#include <vector>
#include <utility>


// Parser for DxO Optics Pro .dop sidecars.
namespace index_images { namespace dop {
	
	namespace x3 = boost::spirit::x3;
	
	struct list;
	struct group;
	
	typedef x3::forward_ast <list> list_forward_ast;
	typedef x3::forward_ast <group> group_forward_ast;
	
	// Apparently the parser requires x3::variant instead of std::variant.
	typedef x3::variant <
		bool,
		std::int64_t,
		double,
		std::string,
		list_forward_ast,
		group_forward_ast
	> value_base;
	
	struct value;
	typedef std::vector <value> value_list;
	typedef std::map <std::string, value> key_value_map;
	typedef std::pair <std::string, value> key_value_pair;
	
	struct value : value_base
	{
		using value_base::value_base;
		using value_base::operator=;
	};
	
	struct list
	{
		value_list entries;
	};
	
	struct group
	{
		key_value_map entries;
	};
	
	bool parse_stream(std::istream &stream, key_value_pair &out_pair);
	
	// Forward declarations, needed (also) b.c. of mutual recursion.
	std::ostream &operator<<(std::ostream &os, list_forward_ast const &ast);
	std::ostream &operator<<(std::ostream &os, group_forward_ast const &ast);
	std::ostream &operator<<(std::ostream &os, list const &l);
	std::ostream &operator<<(std::ostream &os, group const &g);
	std::ostream &operator<<(std::ostream &os, value const &val);
	
	// Implementations.
	inline std::ostream &operator<<(std::ostream &os, list_forward_ast const &ast) { return os << ast.get(); }
	inline std::ostream &operator<<(std::ostream &os, group_forward_ast const &ast) { return os << ast.get(); }
	
	// Helpers for accessing value.
	template <typename t_value>
	t_value const &get(value const &val)
	{
		return boost::get <t_value>(val);
	}
	
	template <>
	inline value_list const &get <value_list>(value const &val)
	{
		return boost::get <list_forward_ast>(val).get().entries;
	}
	
	template <>
	inline key_value_map const &get <key_value_map>(value const &val)
	{
		return boost::get <group_forward_ast>(val).get().entries;
	}
	
	
	// Helpers for accessing key_value_map.
	template <typename t_value>
	t_value const &get_map_value(key_value_map const &map, std::string const &key)
	{
		auto const it(map.find(key));
		if (map.end() == it)
			throw boost::bad_get();
		
		return get <t_value>(it->second);
	}
	
	template <typename t_value, typename ... t_args>
	t_value const &get_map_value(key_value_map const &map, std::string const &key, t_args ... args)
	{
		auto const it(map.find(key));
		if (map.end() == it)
			throw boost::bad_get();
		
		return get_map_value <t_value>(get <key_value_map>(it->second), args...);
	}
	
	template <typename t_value, typename ... t_args>
	t_value const &get_map_value(value const &val, std::string const &key, t_args ... args)
	{
		return get_map_value <t_value>(get <key_value_map>(val), key, args...);
	}
}}

#endif
