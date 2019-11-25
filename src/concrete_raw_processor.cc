/**
 * Copyright (c) Tuukka Norri 2019
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <algorithm>
#include <boost/gil.hpp>
#include <boost/gil/extension/io/jpeg.hpp>
#include <boost/gil/extension/io/tiff.hpp>
#include <boost/gil/extension/numeric/sampler.hpp>
#include <boost/gil/extension/numeric/resample.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/stream.hpp>
#include <libbio/assert.hh>
#include <libbio/file_handling.hh>
#include "concrete_raw_processor.hh"
#include "dop_parser.hh"
#include "libraw_exif_reader.hh"

namespace bios = boost::iostreams;
namespace gil = boost::gil;
namespace lb = libbio;


namespace {
	
	namespace ii = index_images;
	
	
	void trim(std::string &str)
	{
		auto const pos(str.find_last_not_of(" \n\r\t"));
		if (std::string::npos != pos)
			str.resize(1 + pos);
	}
	
	
	void exif_callback(void *context, int tag, int type, int len, unsigned int ord, void *ifp, INT64 base)
	{
		auto &ds(*reinterpret_cast <LibRaw_abstract_datastream *>(ifp));
		auto &processor(*reinterpret_cast <ii::concrete_raw_processor *>(context));
		auto &exif_properties(processor.get_exif_properties());
		
		switch (tag)
		{
			case 0x8298: 	// Copyright
			case 0x108298:
			{
				if (ii::libraw_exif::read_single_ascii(ds, type, len, ord, exif_properties.copyright))
					trim(exif_properties.copyright);
				else
					std::cerr << "Unexpected value for copyright.\n";
				break;
			}
			
			case 0x8822: // Exposure program
			{
				if (!ii::libraw_exif::read_single_short(ds, type, len, ord, exif_properties.exposure_program))
					std::cerr << "Unexpected value for exposure program.\n";
				break;
			}
			
			//case 0x9201: // Shutter
			case 0x829a: // Exposure
			{
				if (!ii::libraw_exif::read_single_rational(ds, type, len, ord, exif_properties.exposure_time))
					std::cerr << "Unexpected value for shutter speed.\n";
				break;
			}
			
			case 0x9209: // Flash
			{
				if (!ii::libraw_exif::read_single_short(ds, type, len, ord, exif_properties.flash))
					std::cerr << "Unexpected value for flash.\n";
				break;
			}
			
			case 0xa434: // Lens model
			{
				std::string dst;
				if (!ii::libraw_exif::read_single_ascii(ds, type, len, ord, exif_properties.lens_model))
					std::cerr << "Unexpected value for lens model.\n";
				break;
			}
			
			default:
				break;
		}
	}
	
	
	// Custom deleter from libraw memory images.
	struct free_dcraw_mem_image
	{
		void operator()(libraw_processed_image_t *ptr)
		{
			LibRaw::dcraw_clear_mem(ptr);
		}
	};
	
	
	// Write the source image to the given view and save it as JPEG.
	template <typename t_src, typename t_dst>
	void resize_image(t_src const &src_view, t_dst &dst_image, ii::raw_processor::buffer_type &buffer)
	{
		auto dst_view(gil::view(dst_image));
		gil::resize_view(src_view, dst_view, gil::bilinear_sampler());

		// Write the JPEG data to the given vector.
		typedef bios::back_insert_device <ii::raw_processor::buffer_type> sink_type;
		buffer.clear();
		sink_type sink(buffer);
		bios::stream <sink_type> os(sink);
		gil::write_view(os, dst_view, gil::image_write_info <gil::jpeg_tag>(85));
	}
	
	
	void assign_first_string(std::string const &src, std::string &dst)
	{
		if (auto const pos(src.find_first_of('\0')); pos == std::string::npos)
			dst = src;
		else
			dst = src.substr(0, pos);
	}
}


namespace index_images {
	
	// Read the relevant EXIF data.
	void concrete_raw_processor::read_additional_exif_data()
	{
		auto const &make(m_processor.imgdata.idata.make);
		auto const &model(m_processor.imgdata.idata.model);
		auto const &artist(m_processor.imgdata.other.artist);
		
		assign_first_string(make, m_exif_properties.make);		// FIXME: make has multiple strings at least in the case of Olympus. Consider reading all of them.
		assign_first_string(model, m_exif_properties.model);
		assign_first_string(artist, m_exif_properties.artist);
		
		trim(m_exif_properties.artist);
		
		// FIXME: check the types.
		this->m_exif_properties.iso_speed = m_processor.imgdata.other.iso_speed;
		this->m_exif_properties.aperture = m_processor.imgdata.other.aperture;
		this->m_exif_properties.focal_length = m_processor.imgdata.other.focal_len;
		this->m_exif_properties.timestamp = m_processor.imgdata.other.timestamp;
	}
	
	
	// Read the relevant sidecar data.
	void concrete_raw_processor::read_dop_data(std::string const &path)
	{
		m_dop_properties = dop_properties();
		
		auto const dop_path(path + ".dop");
		lb::file_istream stream;
		if (lb::try_open_file_for_reading(dop_path, stream))
		{
			dop::key_value_pair root;
			if (!dop::parse_stream(stream, root))
			{
				std::cerr << "Unable to parse the DOP file: " << dop_path << '\n';
				return;
			}
			
			// Find the correct Item.
			try
			{
				auto const &list(dop::get_map_value <dop::value_list>(root.second, "Source", "Items"));
				
				// Determine the base name.
				auto const pos(path.find_last_of("/"));
				std::string_view const path_view(path);
				std::string_view basename(path_view.substr(std::string::npos == pos || 1 + pos == path_view.size() ? 0 : 1 + pos));
				
				for (auto const &item_val : list)
				{
					try
					{
						auto const &item(dop::get <dop::key_value_map>(item_val));
						auto const &name(dop::get_map_value <std::string>(item, "Name"));
						if (name == basename)
						{
							try
							{
								m_dop_properties.rank = dop::get_map_value <std::int64_t>(item, "Rank");
							}
							catch (boost::bad_get const &)
							{
								// Return.
							}
							
							return;
						}
					}
					catch (boost::bad_get const &)
					{
						// Continue.
					}
				}
				
			}
			catch (boost::bad_get const &)
			{
				// Return.
			}
		}
	}
	
	
	// Prepare m_processor.
	void concrete_raw_processor::prepare_file(std::string const &path)
	{
		read_dop_data(path);
		
		m_processor.set_exifparser_handler(&exif_callback, this);
		
		m_processor.open_file(path.c_str()); // Checked from the source that the file is opened read-only.
		m_processor.unpack();
		
		read_additional_exif_data();
		
		m_processor.recycle_datastream();
	}
	
	
	// Helper function for determining the scaled image size.
	auto concrete_raw_processor::scaled_image_size(std::size_t const width, std::size_t const height) -> std::pair <std::uint16_t, std::uint16_t>
	{
		auto const max_dim(std::max(width, height));
		double const max_size(1024.0);
		auto const factor(max_size / max_dim);
		return std::pair <std::uint16_t, std::uint16_t>(factor * width, factor * height);
	}
	
	
	// Process the prepared image.
	// Use a parallel queue at the call site.
	void concrete_raw_processor::process_image()
	{
		typedef bios::stream <bios::array_sink> buffer_ostream;
	
		// Convert the RAW to RGB.
		{
			auto const st(m_processor.dcraw_process());
			if (LIBRAW_SUCCESS != st)
			{
				std::cerr << "*** Got a libraw error: " << libraw_strerror(st) << '\n';
				goto end;
			}
		}
		
		{
			// Get the RGB output.
			int status(0);
			std::unique_ptr <libraw_processed_image_t, free_dcraw_mem_image> processed_image(m_processor.dcraw_make_mem_image(&status));
			if (!processed_image)
			{
				std::cerr << "*** Got a libraw error: " << libraw_strerror(status) << '\n';
				goto end;
			}

			libbio_always_assert_msg(LIBRAW_IMAGE_BITMAP == processed_image->type, "Expected to get a bitmap.");
			auto const scaled_size(scaled_image_size(processed_image->width, processed_image->height));
			
			// Get an image view and resize.
			switch (processed_image->colors)
			{
				case 1:
				{
					gil::gray8_image_t dst_image(scaled_size.first, scaled_size.second);
					switch (processed_image->bits)
					{
						case 8:
						{
							auto const *pixel_ptr(reinterpret_cast <gil::gray8_pixel_t const *>(processed_image->data));
							auto const src_view(gil::interleaved_view(processed_image->width, processed_image->height, pixel_ptr, processed_image->width * sizeof(char)));
							resize_image(src_view, dst_image, m_buffer);
							break;
						}
						case 16:
						{
							// May not work if LibRaw does not allocate with the correct alignment.
							auto const *pixel_ptr(reinterpret_cast <gil::gray16_pixel_t const *>(processed_image->data));
							auto const src_view(gil::interleaved_view(processed_image->width, processed_image->height, pixel_ptr, processed_image->width * 2 * sizeof(char)));
							resize_image(src_view, dst_image, m_buffer);
							break;
						}
						default:
							libbio_fail("Unexpected number of bits.");
					}
					break;
				}
					
				case 3:
				{
					gil::rgb8_image_t dst_image(scaled_size.first, scaled_size.second);
					switch (processed_image->bits)
					{
						case 8:
						{
							auto const *pixel_ptr(reinterpret_cast <gil::rgb8_pixel_t const *>(processed_image->data));
							auto const src_view(gil::interleaved_view(processed_image->width, processed_image->height, pixel_ptr, 3 * processed_image->width * sizeof(char)));
							resize_image(src_view, dst_image, m_buffer);
							break;
						}
						case 16:
						{
							// May not work if LibRaw does not allocate with the correct alignment.
							auto const *pixel_ptr(reinterpret_cast <gil::rgb16_pixel_t const *>(processed_image->data));
							auto const src_view(gil::interleaved_view(processed_image->width, processed_image->height, pixel_ptr, 3 * processed_image->width * 2 * sizeof(char)));
							resize_image(src_view, dst_image, m_buffer);
							break;
						}
						default:
							libbio_fail("Unexpected number of bits.");
					}
					break;
				}
					
				default:
					libbio_fail("Unexpected number of colour components.");
			}
		}

	end:
		// Free memory.
		m_processor.recycle();
	}
}
