include local.mk
include common.mk

DEPENDENCIES =	lib/libbio/src/libbio.a \
				lib/LibRaw/lib/.libs/libraw.a

ifeq ($(shell uname -s),Linux)
	DEPENDENCIES += lib/swift-corelibs-libdispatch/build/src/libdispatch.a
endif


.PHONY: all clean-all clean clean-dependencies dependencies dist

all: dependencies
	$(MAKE) -C src all

clean-all: clean clean-dependencies clean-dist

clean:
	$(MAKE) -C src clean

clean-dependencies: lib/libbio/local.mk
	$(MAKE) -C lib/libbio clean-all
	-$(MAKE) -C lib/libRaw distclean
	$(RM) -rf lib/swift-corelibs-libdispatch/build

dependencies: $(DEPENDENCIES)

lib/LibRaw/lib/.libs/libraw.a: local.mk
	cd lib/LibRaw && \
	autoreconf --install && \
	CC="$(CC)" CXX="$(CXX)" CFLAGS="$(SYSTEM_CFLAGS)" CXXFLAGS="$(SYSTEM_CXXFLAGS)" LDFLAGS="$(SYSTEM_LDFLAGS) -lc++" ./configure --disable-shared --enable-static && \
	$(MAKE)

lib/libbio/local.mk: local.mk
	$(CP) local.mk lib/libbio

lib/libbio/src/libbio.a: lib/libbio/local.mk
	$(MAKE) -C lib/libbio

lib/swift-corelibs-libdispatch/build/src/libdispatch.a:
	$(RM) -rf lib/swift-corelibs-libdispatch/build && \
	cd lib/swift-corelibs-libdispatch && \
	$(MKDIR) build && \
	cd build && \
	$(CMAKE) \
		-G Ninja \
		-DCMAKE_C_COMPILER="$(CC)" \
		-DCMAKE_CXX_COMPILER="$(CXX)" \
		-DCMAKE_C_FLAGS="$(LIBDISPATCH_CFLAGS)" \
		-DCMAKE_CXX_FLAGS="$(LIBDISPATCH_CXXFLAGS)" \
		-DBUILD_SHARED_LIBS=OFF \
		.. && \
	$(NINJA) -v
