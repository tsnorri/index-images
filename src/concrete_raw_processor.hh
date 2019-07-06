/**
 * Copyright (c) Tuukka Norri 2019
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef INDEX_IMAGES_CONCRETE_RAW_PROCESSOR_HH
#define INDEX_IMAGES_CONCRETE_RAW_PROCESSOR_HH

#include "raw_processor.hh"

// Does not use namespaces.
#include <libraw/libraw.h>


namespace index_images {
	
	class concrete_raw_processor : public raw_processor
	{
	protected:
		LibRaw		m_processor;
		
	public:
		using raw_processor::raw_processor;
		
		void prepare_file(std::string const &path) override;
		void process_image() override;
		
	protected:
		void read_additional_exif_data();
		void read_dop_data(std::string const &path);
		std::pair <std::uint16_t, std::uint16_t> scaled_image_size(std::size_t const width, std::size_t const height);
	};
}

#endif
