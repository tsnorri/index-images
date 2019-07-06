/**
 * Copyright (c) Tuukka Norri 2019
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <boost/endian/conversion.hpp>

// Does not use namespaces.
#include <libraw/libraw.h>

#include "libraw_exif_reader.hh"


namespace {
	
	namespace le = index_images::libraw_exif;
	
	
	template <bool t_swap_bytes, typename t_dst>
	inline bool copy_single_value(LibRaw_abstract_datastream &ds, t_dst &dst, std::uint16_t const ord)
	{
		// loc not necessarily t_dst-aligned. (Did not check the TIFF or EXIF specs thoroughly, though.)
		auto const read_len(ds.read(&dst, sizeof(t_dst), 1));
		if (1 != read_len)
			return false;
		
		if constexpr (!t_swap_bytes)
			return true;
		else
		{
			switch (ord)
			{
				case 0x4949: // II
					boost::endian::little_to_native_inplace(dst);
					return true;
			
				case 0x4d4d: // MM
					boost::endian::big_to_native_inplace(dst);
					return true;
			
				default:
					return false;
			}
		}
	}
	
	template <bool t_swap_bytes, typename t_dst>
	inline bool copy_multiple_values(LibRaw_abstract_datastream &ds, std::size_t const len, std::vector <t_dst> &dst, std::uint16_t const ord)
	{
		namespace end = boost::endian;
		
		if (len != ds.read(dst.data(), sizeof(t_dst), len))
			return false;
		
		if constexpr (!t_swap_bytes)
			return true;
		else
		{
			switch (ord)
			{
				case 0x4949: // II
				{
					if (end::order::little == end::order::native)
						return true;
				
					for (auto &val : dst)
						end::little_to_native_inplace(val);
					return true;
				}
			
				case 0x4d4d: // MM
				{
					if (end::order::big == end::order::native)
						return true;
				
					for (auto &val : dst)
						end::big_to_native_inplace(val);
					return true;
				}
			
				default:
					return false;
			}
		}
	}
	
	template <le::tiff_data_type t_data_type, bool t_swap_bytes, typename t_dst>
	bool read_single_value(LibRaw_abstract_datastream &ds, int const dt, std::size_t const len, std::uint16_t const ord, t_dst &dst)
	{
		if (1 != len)
			return false;
		if (t_data_type != dt)
			return false;
		return copy_single_value <t_swap_bytes>(ds, dst, ord);
	}
	
	template <le::tiff_data_type t_data_type, bool t_swap_bytes, typename t_dst>
	bool read_single_rational_value(LibRaw_abstract_datastream &ds, int const dt, std::size_t const len, std::uint16_t const ord, std::pair <t_dst, t_dst> &dst)
	{
		if (1 != len)
			return false;
		if (t_data_type != dt)
			return false;
		
		if (!copy_single_value <t_swap_bytes>(ds, dst.first, ord))
			return false;
		return (copy_single_value <t_swap_bytes>(ds, dst.second, ord));
	}
	
	template <le::tiff_data_type t_data_type, bool t_swap_bytes, typename t_dst>
	bool read_multiple_values(LibRaw_abstract_datastream &ds, int const dt, std::size_t const len, std::uint16_t const ord, std::vector <t_dst> &dst)
	{
		dst.clear();
		
		if (t_data_type != dt)
			return false;
		
		dst.resize(len, t_dst());
		
		if (!copy_multiple_values <t_swap_bytes>(ds, len, dst, ord))
			return false;
		
		return true;
	}
}

namespace index_images { namespace libraw_exif {
	
	bool read_single_ascii(LibRaw_abstract_datastream &ds, int const dt, std::size_t const len, std::uint16_t const ord, std::string &dst)
	{
		dst.clear();
		
		if (le::tiff_data_type::ASCII != dt)
			return false;
		
		dst.resize(len, 0);
		if (len != ds.read(dst.data(), 1, len))
			return false;
		
		// Check for the nul character.
		if (auto const pos(dst.find_first_of('\0')); std::string::npos != pos)
			dst.resize(pos);
		
		return true;
	}
	
	bool read_single_byte			(LibRaw_abstract_datastream &ds, int const dt, std::size_t const len, std::uint16_t const ord, std::byte &dst)									{ return read_single_value <tiff_data_type::BYTE, false>				(ds, dt, len, ord, dst); }
	bool read_single_short			(LibRaw_abstract_datastream &ds, int const dt, std::size_t const len, std::uint16_t const ord, std::uint16_t &dst)								{ return read_single_value <tiff_data_type::SHORT, true>				(ds, dt, len, ord, dst); }
	bool read_single_long			(LibRaw_abstract_datastream &ds, int const dt, std::size_t const len, std::uint16_t const ord, std::uint32_t &dst)								{ return read_single_value <tiff_data_type::LONG, true>					(ds, dt, len, ord, dst); }
	bool read_single_sbyte			(LibRaw_abstract_datastream &ds, int const dt, std::size_t const len, std::uint16_t const ord, signed char &dst)								{ return read_single_value <tiff_data_type::SBYTE, false>				(ds, dt, len, ord, dst); }
	bool read_single_undefined		(LibRaw_abstract_datastream &ds, int const dt, std::size_t const len, std::uint16_t const ord, std::byte &dst)									{ return read_single_value <tiff_data_type::UNDEFINED, false>			(ds, dt, len, ord, dst); }
	bool read_single_sshort			(LibRaw_abstract_datastream &ds, int const dt, std::size_t const len, std::uint16_t const ord, std::int16_t &dst)								{ return read_single_value <tiff_data_type::SSHORT, true>				(ds, dt, len, ord, dst); }
	bool read_single_slong			(LibRaw_abstract_datastream &ds, int const dt, std::size_t const len, std::uint16_t const ord, std::int32_t &dst)								{ return read_single_value <tiff_data_type::SLONG, true>				(ds, dt, len, ord, dst); }
	bool read_single_float			(LibRaw_abstract_datastream &ds, int const dt, std::size_t const len, std::uint16_t const ord, float &dst)										{ return read_single_value <tiff_data_type::FLOAT, false>				(ds, dt, len, ord, dst); }
	bool read_single_double			(LibRaw_abstract_datastream &ds, int const dt, std::size_t const len, std::uint16_t const ord, double &dst)										{ return read_single_value <tiff_data_type::DOUBLE, false>				(ds, dt, len, ord, dst); }
	bool read_single_rational		(LibRaw_abstract_datastream &ds, int const dt, std::size_t const len, std::uint16_t const ord, std::pair <std::uint32_t, std::uint32_t> &dst)	{ return read_single_rational_value <tiff_data_type::RATIONAL, true>	(ds, dt, len, ord, dst); }
	bool read_single_srational		(LibRaw_abstract_datastream &ds, int const dt, std::size_t const len, std::uint16_t const ord, std::pair <std::int32_t, std::int32_t> &dst)		{ return read_single_rational_value <tiff_data_type::SRATIONAL, true>	(ds, dt, len, ord, dst); }
	
	bool read_multiple_byte			(LibRaw_abstract_datastream &ds, int const dt, std::size_t const len, std::uint16_t const ord, std::vector <std::byte> &dst)					{ return read_multiple_values <tiff_data_type::BYTE, false>				(ds, dt, len, ord, dst); }
	bool read_multiple_short		(LibRaw_abstract_datastream &ds, int const dt, std::size_t const len, std::uint16_t const ord, std::vector <std::uint16_t> &dst)				{ return read_multiple_values <tiff_data_type::SHORT, true>				(ds, dt, len, ord, dst); }
	bool read_multiple_long			(LibRaw_abstract_datastream &ds, int const dt, std::size_t const len, std::uint16_t const ord, std::vector <std::uint32_t> &dst)				{ return read_multiple_values <tiff_data_type::LONG, true>				(ds, dt, len, ord, dst); }
	bool read_multiple_sbyte		(LibRaw_abstract_datastream &ds, int const dt, std::size_t const len, std::uint16_t const ord, std::vector <signed char> &dst)					{ return read_multiple_values <tiff_data_type::SBYTE, false>			(ds, dt, len, ord, dst); }
	bool read_multiple_undefined	(LibRaw_abstract_datastream &ds, int const dt, std::size_t const len, std::uint16_t const ord, std::vector <std::byte> &dst)					{ return read_multiple_values <tiff_data_type::UNDEFINED, false>		(ds, dt, len, ord, dst); }
	bool read_multiple_sshort		(LibRaw_abstract_datastream &ds, int const dt, std::size_t const len, std::uint16_t const ord, std::vector <std::int16_t> &dst)					{ return read_multiple_values <tiff_data_type::SSHORT, true>			(ds, dt, len, ord, dst); }
	bool read_multiple_slong		(LibRaw_abstract_datastream &ds, int const dt, std::size_t const len, std::uint16_t const ord, std::vector <std::int32_t> &dst)					{ return read_multiple_values <tiff_data_type::SLONG, true>				(ds, dt, len, ord, dst); }
	bool read_multiple_float		(LibRaw_abstract_datastream &ds, int const dt, std::size_t const len, std::uint16_t const ord, std::vector <float> &dst)						{ return read_multiple_values <tiff_data_type::FLOAT, false>			(ds, dt, len, ord, dst); }
	bool read_multiple_double		(LibRaw_abstract_datastream &ds, int const dt, std::size_t const len, std::uint16_t const ord, std::vector <double> &dst)						{ return read_multiple_values <tiff_data_type::DOUBLE, false>			(ds, dt, len, ord, dst); }
}}
