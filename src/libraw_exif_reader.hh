/**
 * Copyright (c) Tuukka Norri 2019
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef INDEX_IMAGES_LIBRAW_EXIF_READER_HH
#define INDEX_IMAGES_LIBRAW_EXIF_READER_HH

#include <libbio/utility.hh>


// Handle EXIF data as part of Libraw’s callback.
namespace index_images { namespace libraw_exif {
	
	enum tiff_data_type : int
	{
		BYTE = 1,
		ASCII = 2,
		SHORT = 3,
		LONG = 4,
		RATIONAL = 5,
		SBYTE = 6,
		UNDEFINED = 7,
		SSHORT = 8,
		SLONG = 9,
		SRATIONAL = 10,
		FLOAT = 11,
		DOUBLE = 12
	};
	
	inline bool operator==(tiff_data_type const lhs, std::underlying_type_t <tiff_data_type> const rhs)
	{
		return libbio::to_underlying(lhs) == rhs;
	}

	inline bool operator!=(tiff_data_type const lhs, std::underlying_type_t <tiff_data_type> const rhs)
	{
		return !(lhs == rhs);
	}
	
	bool read_single_ascii			(LibRaw_abstract_datastream &ds, int const dt, std::size_t const len, std::uint16_t const ord, std::string &dst);
	bool read_single_byte			(LibRaw_abstract_datastream &ds, int const dt, std::size_t const len, std::uint16_t const ord, std::byte &dst);
	bool read_single_short			(LibRaw_abstract_datastream &ds, int const dt, std::size_t const len, std::uint16_t const ord, std::uint16_t &dst);
	bool read_single_long			(LibRaw_abstract_datastream &ds, int const dt, std::size_t const len, std::uint16_t const ord, std::uint32_t &dst);
	bool read_single_sbyte			(LibRaw_abstract_datastream &ds, int const dt, std::size_t const len, std::uint16_t const ord, signed char &dst);
	bool read_single_undefined		(LibRaw_abstract_datastream &ds, int const dt, std::size_t const len, std::uint16_t const ord, std::byte &dst);
	bool read_single_sshort			(LibRaw_abstract_datastream &ds, int const dt, std::size_t const len, std::uint16_t const ord, std::int16_t &dst);
	bool read_single_slong			(LibRaw_abstract_datastream &ds, int const dt, std::size_t const len, std::uint16_t const ord, std::int32_t &dst);
	bool read_single_float			(LibRaw_abstract_datastream &ds, int const dt, std::size_t const len, std::uint16_t const ord, float &dst);
	bool read_single_double			(LibRaw_abstract_datastream &ds, int const dt, std::size_t const len, std::uint16_t const ord, double &dst);
	bool read_single_rational		(LibRaw_abstract_datastream &ds, int const dt, std::size_t const len, std::uint16_t const ord, std::pair <std::uint32_t, std::uint32_t> &dst);
	bool read_single_srational		(LibRaw_abstract_datastream &ds, int const dt, std::size_t const len, std::uint16_t const ord, std::pair <std::int32_t, std::int32_t> &dst);
	
	bool read_multiple_byte			(LibRaw_abstract_datastream &ds, int const dt, std::size_t const len, std::uint16_t const ord, std::vector <std::byte> &dst);
	bool read_multiple_short		(LibRaw_abstract_datastream &ds, int const dt, std::size_t const len, std::uint16_t const ord, std::vector <std::uint16_t> &dst);
	bool read_multiple_long			(LibRaw_abstract_datastream &ds, int const dt, std::size_t const len, std::uint16_t const ord, std::vector <std::uint32_t> &dst);
	bool read_multiple_sbyte		(LibRaw_abstract_datastream &ds, int const dt, std::size_t const len, std::uint16_t const ord, std::vector <signed char> &dst);
	bool read_multiple_undefined	(LibRaw_abstract_datastream &ds, int const dt, std::size_t const len, std::uint16_t const ord, std::vector <std::byte> &dst);
	bool read_multiple_sshort		(LibRaw_abstract_datastream &ds, int const dt, std::size_t const len, std::uint16_t const ord, std::vector <std::int16_t> &dst);
	bool read_multiple_slong		(LibRaw_abstract_datastream &ds, int const dt, std::size_t const len, std::uint16_t const ord, std::vector <std::int32_t> &dst);
	bool read_multiple_float		(LibRaw_abstract_datastream &ds, int const dt, std::size_t const len, std::uint16_t const ord, std::vector <float> &dst);
	bool read_multiple_double		(LibRaw_abstract_datastream &ds, int const dt, std::size_t const len, std::uint16_t const ord, std::vector <double> &dst);
}}

#endif
