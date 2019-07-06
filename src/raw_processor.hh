/**
 * Copyright (c) Tuukka Norri 2019
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef INDEX_IMAGES_RAW_PROCESSOR_HH
#define INDEX_IMAGES_RAW_PROCESSOR_HH

#include <cstdint>
#include <string>
#include <vector>
#include <utility>


namespace index_images {
	
	typedef std::pair <std::uint32_t, std::uint32_t> rational_type;
	
	
	class raw_processor;
	
	
	struct exif_properties
	{
		std::string		artist;
		std::string		copyright;
		std::string		make;
		std::string		model;
		std::string		lens_model;
		rational_type	exposure_time{};
		std::uint64_t	timestamp{};
		float			aperture{};
		float			focal_length{};
		float			iso_speed{};
		std::uint16_t	exposure_program{};
		std::uint16_t	flash{};
	};
	
	struct dop_properties
	{
		std::int32_t	rank{};
	};
	
	std::ostream &operator<<(std::ostream &os, exif_properties const &properties);
	
	
	// RAW processor base class.
	class raw_processor
	{
	public:
		typedef std::vector <char>	buffer_type;
		
	protected:
		buffer_type				m_buffer;
		exif_properties			m_exif_properties;
		dop_properties			m_dop_properties;
		
	public:
		static raw_processor *instantiate();
		virtual ~raw_processor() {}
		virtual void prepare_file(std::string const &path) = 0;
		virtual void process_image() = 0;
		buffer_type const &get_buffer() const { return m_buffer; }
		exif_properties const &get_exif_properties() const { return m_exif_properties; }
		exif_properties &get_exif_properties() { return m_exif_properties; }
		dop_properties const &get_dop_properties() const { return m_dop_properties; }
		
	protected:
		raw_processor() = default;
	};
}

#endif
