--
-- Copyright (c) 2019 Tuukka Norri
-- This code is licensed under MIT license (see LICENSE for details).
--

CREATE TABLE image (
	id					INTEGER PRIMARY KEY,
	project				TEXT,
	filename			TEXT,
	timestamp			INTEGER,
	artist				TEXT,
	copyright			TEXT,
	make				TEXT,
	model				TEXT,
	lens_model			TEXT,
	aperture			REAL,
	focal_length		REAL,
	iso					REAL,
	exposure_time_n		INTEGER,
	exposure_time_d		INTEGER,
	exposure_program	INTEGER,
	flash				INTEGER,
	rank				INTEGER,
	preview				BLOB
);
