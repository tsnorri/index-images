/**
 * Copyright (c) Tuukka Norri 2019
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <boost/iostreams/filtering_stream.hpp>
#include <boost/range/iterator_range.hpp>
#include <filesystem>
#include <libbio/assert.hh>
#include <libbio/dispatch.hh>
#include <list>
#include <regex>
#include <sqlite_modern_cpp.h>
#include "cmdline.h"
#include "raw_processor.hh"

namespace fs	= std::filesystem;
namespace lb	= libbio;
namespace pi	= index_images;
namespace bios	= boost::iostreams;


namespace {
	
	class index_images_context;
	
	
	// Maintain the directory tree iteration state.
	class process_directory_state
	{
	protected:
		fs::recursive_directory_iterator		m_directory_iterator;
		
	public:
		process_directory_state() = default;
		
		process_directory_state(std::string const &path):
			m_directory_iterator(fs::recursive_directory_iterator(path))
		{
		}
		
		bool advance_and_call(index_images_context &ctx);
	};
	
	
	enum processing_state : std::uint8_t
	{
		PROCESSING,
		WAITING_FOR_WORKER,
		DRAINING
	};
	
	
	class index_images_context
	{
	protected:
		typedef std::unique_ptr <pi::raw_processor>	processor_ptr;
		typedef std::list <processor_ptr>			processor_list_type;
		
	protected:
		sqlite::database							m_db;
		process_directory_state						m_dir_state;
		std::regex									m_name_regex;
		std::string									m_image_root;
		processor_list_type							m_processors;
		processor_list_type							m_pending_processors;
		processing_state							m_state{PROCESSING};
		std::uint16_t								m_project_name_from_parent{};
		
	protected:
		static constexpr std::size_t processor_count() { return 16; }
		
	public:
		index_images_context(std::string const &image_root, std::string const &database_path, std::uint16_t project_name_from_parent):
			m_db(database_path),
			m_name_regex("\\.ORF$", std::regex_constants::icase),
			m_image_root(image_root),
			m_processors(processor_count()),
			m_project_name_from_parent(project_name_from_parent)
		{
			for (auto &ptr : m_processors)
				ptr.reset(pi::raw_processor::instantiate());
			
			// Try to save time.
			m_db << u8"PRAGMA journal_mode = OFF;";
		}
		
		std::regex const &get_name_regex() const { return m_name_regex; }
		
		void start_processing();
		void process_next();
		
		inline processor_ptr pick_processor();
		inline void return_processor(processor_ptr &ptr);
		
		void process_path(std::string const &path);
		void finish();
		
	protected:
		void cleanup() { delete this; }
		inline void queue_process_next();
		std::string_view project_name(std::string const &path) const;
	};
	
	
	// Find the next image file and call the processing function.
	bool process_directory_state::advance_and_call(index_images_context &ctx)
	{
		while (true)
		{
			if (fs::recursive_directory_iterator() == m_directory_iterator)
				return false;
			
			auto const &entry(*m_directory_iterator);
			auto const &path(entry.path());
			auto const &path_str(path.u8string());
			
			if (std::regex_search(path_str, ctx.get_name_regex()))
			{
				ctx.process_path(path_str);
				++m_directory_iterator;
				return true;
			}
			
			++m_directory_iterator;
		}
		
		// Should not be reached.
		libbio_fail("One of the return statements in the while loop should be reached.");
		return false;
	}
	
	
	// Start the processing loop.
	void index_images_context::start_processing()
	{
		// Processing entry point.
		m_dir_state = process_directory_state(m_image_root);
		if (!m_dir_state.advance_and_call(*this))
		{
			std::cerr << "Did not find any suitable files.\n";
			finish();
		}
	}
	
	
	// Attempt to advance the directory tree iteration.
	void index_images_context::process_next()
	{
		// If there are no more images to process, drain the queue.
		if (!m_dir_state.advance_and_call(*this))
			m_state = processing_state::DRAINING;
	}
	
	
	// Queue the advancing function.
	void index_images_context::queue_process_next()
	{
		lb::dispatch(this).async <&index_images_context::process_next>(dispatch_get_main_queue());
	}
	
	
	// Clean up.
	void index_images_context::finish()
	{
		cleanup();
		// this no longer valid.
		std::exit(EXIT_SUCCESS);
	}
	
	
	// Process the image at the given path.
	void index_images_context::process_path(std::string const &path)
	{
		// Callback (from process_directory_state::advance_and_call).
		std::cerr << path << std::endl;
		
		// Get an image processor and move it to the pending list.
		auto current_proc(pick_processor());
		current_proc->prepare_file(path);
		lb::dispatch_async_fn(dispatch_get_global_queue(QOS_CLASS_UTILITY, 0), [this, path, current_proc{std::move(current_proc)}]() mutable {
			current_proc->process_image();
			lb::dispatch_async_fn(dispatch_get_main_queue(), [this, path, current_proc{std::move(current_proc)}]() mutable {
				
				auto const &exif_data(current_proc->get_exif_properties());
				auto const &dop_data(current_proc->get_dop_properties());
				auto const &project(project_name(path));
				
				// Handle the processed data.
				try
				{
					m_db
						<< u8"INSERT INTO image ("
						"filename, project, timestamp, artist, copyright, make, model, lens_model, aperture, "
						"focal_length, exposure_time_n, exposure_time_d, iso, exposure_program, flash, rank, preview"
						") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);"
						<< path
						<< std::string(project)
						<< exif_data.timestamp
						<< exif_data.artist
						<< exif_data.copyright
						<< exif_data.make
						<< exif_data.model
						<< exif_data.lens_model
						<< exif_data.aperture
						<< exif_data.focal_length
						<< exif_data.exposure_time.first
						<< exif_data.exposure_time.second
						<< exif_data.iso_speed
						<< exif_data.exposure_program
						<< exif_data.flash
						<< dop_data.rank
						<< current_proc->get_buffer();
				}
				catch (sqlite::sqlite_exception const &exc)
				{
					std::cerr << "Caught an SQLite exception: " << exc.get_code() << ' ' << exc.get_extended_code() << ' ' << exc.what() << '\n';
				}
				
				// Return the processor.
				return_processor(current_proc);
				
				switch (m_state)
				{
					case processing_state::PROCESSING:
						return;
					
					case processing_state::WAITING_FOR_WORKER:
					{
						m_state = processing_state::PROCESSING;
						queue_process_next();
						return;
					}
					
					case processing_state::DRAINING:
					{
						// Check if the last task just finished.
						if (m_pending_processors.empty())
							finish();
						return;
					}
				}
			});
		});
		
		switch (m_state)
		{
			case processing_state::PROCESSING:
			{
				// If there are no more processors available, wait.
				// Otherwise, get the next path.
				if (m_processors.empty())
					m_state = processing_state::WAITING_FOR_WORKER;
				else
					queue_process_next();
				return;
			}
			
			case processing_state::WAITING_FOR_WORKER:
			case processing_state::DRAINING:
				return;
		}
	}
	
	
	// Get an image processor from the processor list.
	auto index_images_context::pick_processor() -> processor_ptr
	{
		processor_ptr retval;
		
		libbio_assert(!m_processors.empty());
		auto proc_it(m_processors.begin());
		
		using std::swap;
		swap(*proc_it, retval);
		libbio_assert(!*proc_it);
		
		m_pending_processors.splice(m_pending_processors.end(), m_processors, proc_it);
		return retval;
	}
	
	
	// Return an image processor to the list.
	void index_images_context::return_processor(processor_ptr &ptr)
	{
		auto proc_it(m_pending_processors.begin());
		
		using std::swap;
		swap(*proc_it, ptr);
		libbio_assert(!ptr);
		
		m_processors.splice(m_processors.begin(), m_pending_processors, proc_it);
	}
	
	
	// Determine the project name by inspecting the image path.
	std::string_view index_images_context::project_name(std::string const &path) const
	{
		if (0 == m_project_name_from_parent)
			goto fail;
		
		{
			auto last_idx(path.size());
			auto prev_idx(last_idx);
			for (decltype(m_project_name_from_parent) i(0); i <= m_project_name_from_parent; ++i)
			{
				if (!last_idx)
					goto fail;
				
				prev_idx = last_idx;
				last_idx = path.find_last_of('/', last_idx - 1);
				if (std::string::npos == last_idx)
				{
					// Not found. If this is the last iteration, we can continue, though.
					if (i < m_project_name_from_parent)
						goto fail;
					else
					{
						last_idx = 0;
						goto ret;
					}
				}
			}
			++last_idx;
			
		ret:
			return std::string_view(path).substr(last_idx, prev_idx - last_idx);
		}
		
	fail:
		return std::string_view();
	}
}


int main(int argc, char **argv)
{
	gengetopt_args_info args_info;
	if (0 != cmdline_parser(argc, argv, &args_info))
		exit(EXIT_FAILURE);

	std::ios_base::sync_with_stdio(false);	// Don't use C style IO after calling cmdline_parser.

#ifndef NDEBUG
	std::cerr << "Assertions have been enabled." << std::endl;
#endif
	
	// Guard for exceptions while starting by using a unique_ptr.
	std::unique_ptr <index_images_context> ctx(new index_images_context(args_info.image_root_arg, args_info.database_arg, args_info.project_name_from_parent_arg));
	lb::dispatch_async_fn(dispatch_get_main_queue(), [ctx{std::move(ctx)}]() mutable {
		ctx->start_processing();
		ctx.release(); // Don’t deallocate.
	});
	
	// Everything in args_info should have been copied by now, so it is no longer needed.
	cmdline_parser_free(&args_info);
	
	dispatch_main();
	// Not reached b.c. pthread_exit() is eventually called in dispatch_main().
	return EXIT_SUCCESS;
}
