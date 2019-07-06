/**
 * Copyright (c) Tuukka Norri 2019
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <iostream>
#include "concrete_raw_processor.hh"


namespace index_images {
	
	raw_processor *raw_processor::instantiate()
	{
		// Return a subclass instance b.c. libraw exposes its data structures to the global namespace
		// and including the headers is therefore not desired.
		return new concrete_raw_processor();
	}

	std::ostream &operator<<(std::ostream &os, exif_properties const &properties)
	{
		std::cerr << "timestamp\t\t\t"		<< properties.timestamp			<< '\n';
		std::cerr << "artist\t\t\t\t"		<< properties.artist			<< '\n';
		std::cerr << "copyright\t\t\t"		<< properties.copyright			<< '\n';
		std::cerr << "make\t\t\t\t"			<< properties.make				<< '\n';
		std::cerr << "model\t\t\t\t"		<< properties.model				<< '\n';
		std::cerr << "lens_model\t\t\t"		<< properties.lens_model		<< '\n';
		
		std::cerr
			<< "exposure_time\t\t"
			<< properties.exposure_time.first
			<< '/'
			<< properties.exposure_time.second
			<< '\n';
		
		std::cerr << "f_number\t\t\t"		<< properties.aperture			<< '\n';
		std::cerr << "exposure_program\t"	<< properties.exposure_program	<< '\n';
		std::cerr << "iso_speed\t\t\t"		<< properties.iso_speed			<< '\n';
		std::cerr << "flash\t\t\t\t"		<< properties.flash				<< '\n';
		std::cerr << "focal_length\t\t"		<< properties.focal_length		<< '\n';
		
		return os;
	}
}
