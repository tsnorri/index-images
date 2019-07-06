/**
 * Copyright (c) Tuukka Norri 2019
 * This code is licensed under MIT license (see LICENSE for details).
 */

//#define BOOST_SPIRIT_X3_DEBUG
#include <boost/fusion/adapted.hpp>
#include <boost/spirit/home/support/multi_pass.hpp>

#include "dop_parser.hh"

namespace spirit = boost::spirit;


namespace index_images { namespace dop {
	
	// Print the variant-based value.
	class value_printer : boost::static_visitor <std::ostream &>
	{
	protected:
		std::ostream *m_os{};
		
	public:
		explicit value_printer(std::ostream &os): m_os(&os) {}
		
		template <typename t_type>
		std::ostream &operator()(t_type const &val) const
		{
			return *m_os << val;
		}
	};
	
	std::ostream &operator<<(std::ostream &os, list const &l)
	{
		os << '[';
		bool first(true);
		for (auto const &val : l.entries)
		{
			if (!first)
				os << ", ";
			os << val;
			first = false;
		}
		return os << ']';
	}
	
	std::ostream &operator<<(std::ostream &os, group const &g)
	{
		os << '{';
		bool first(true);
		for (auto const &kv : g.entries)
		{
			if (!first)
				os << ", ";
			os << kv.first << ": " << kv.second;
			first = false;
		}
		return os << '}';
	}
	
	std::ostream &operator<<(std::ostream &os, value const &val)
	{
		value_printer printer(os);
		return boost::apply_visitor(printer, val);
	}
}}


namespace index_images { namespace dop { namespace parser {
	
	namespace x3 = boost::spirit::x3;
	
	x3::rule <class value_class, value> value_ = "value";
	x3::rule <class key_value_pair_class, key_value_pair> key_value_pair_ = "key_value_pair";
	x3::rule <class list_class, list> list_ = "list";
	x3::rule <class group_class, group> group_ = "group";
	
	x3::real_parser <double, boost::spirit::x3::strict_real_policies <double>> double_;
	
	auto const space_(x3::char_(' ') | x3::char_('\t') | x3::char_('\n'));
	auto const key_(x3::lexeme[+(x3::char_ - space_)]);
	auto const quoted_string_(x3::lexeme['"' >> *(x3::char_ - '"') >> '"']);
	auto const list_wrap_('{' >> list_ >> -x3::lit(',') >> '}');				// Trailing comma not present in empty lists. '
	auto const group_wrap_('{' >> group_ >> -x3::lit(',') >> '}');				// ' <- Fix syntax highlighting.
	
	auto const value__def
		= x3::bool_
		| double_
		| x3::int64
		| quoted_string_
		| list_wrap_
		| group_wrap_
		;
	
	auto const key_value_pair__def(key_ >> '=' >> value_);
	auto const list__def(value_ % ',' | "");									// Allow an empty list here.
	auto const group__def(key_value_pair_ % ',');
	auto const start(x3::skip(space_) [ key_value_pair_ ]);
	
	BOOST_SPIRIT_DEFINE(value_, list_, group_, key_value_pair_);
}}}


BOOST_FUSION_ADAPT_STRUCT(index_images::dop::list, entries);
BOOST_FUSION_ADAPT_STRUCT(index_images::dop::group, entries);


namespace index_images { namespace dop {
	
	bool parse_stream(std::istream &stream, key_value_pair &out_pair)
	{
		typedef std::istreambuf_iterator <char> base_iterator_type;
		spirit::multi_pass <base_iterator_type> it(
			spirit::make_default_multi_pass(base_iterator_type(stream))
		);
		auto const end(spirit::make_default_multi_pass(base_iterator_type()));
		
		key_value_pair p;
		if (x3::parse(it, end, parser::start, p))
		{
			using std::swap;
			swap(p, out_pair);
			return true;
		}
		
		return false;
	}
}}
