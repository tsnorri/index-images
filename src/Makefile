include ../local.mk
include ../common.mk

OBJECTS		=	cmdline.o \
				concrete_raw_processor.o \
				dop_parser.o \
				libraw_exif_reader.o \
				main.o \
				raw_processor.o

all: index_images

clean:
	$(RM) $(OBJECTS) index_images cmdline.c cmdline.h

index_images: $(OBJECTS)
	$(CXX) -fopenmp -o $@ $(OBJECTS) $(LDFLAGS) ../lib/libbio/src/libbio.a ../lib/LibRaw/lib/.libs/libraw.a -lc++fs -llcms2 -lexpat -liconv -ljpeg -lsqlite3 -lz

main.cc : cmdline.c


include ../config.mk
