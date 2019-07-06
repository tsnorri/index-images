CLANG_ROOT			= /usr/local/homebrew/opt/llvm
GCC_ROOT			= /usr/local/homebrew

CC					= $(CLANG_ROOT)/bin/clang
CXX					= $(CLANG_ROOT)/bin/clang++
OPT_FLAGS			= -O2 -g
DOT					= /usr/local/homebrew/bin/dot
WGET				= /usr/local/homebrew/bin/wget

SYSTEM_CFLAGS		= -mmacosx-version-min=10.11
SYSTEM_CXXFLAGS		= -mmacosx-version-min=10.11 -faligned-allocation -stdlib=libc++ -nostdinc++ -I$(CLANG_ROOT)/include/c++/v1
SYSTEM_LDFLAGS		= -mmacosx-version-min=10.11 -faligned-allocation -stdlib=libc++ -L$(CLANG_ROOT)/lib -Wl,-rpath,$(CLANG_ROOT)/lib

CPPFLAGS			= -D_LIBCPP_DISABLE_AVAILABILITY -I/usr/local/homebrew/include
LDFLAGS				= -L/usr/local/homebrew/lib -L/usr/local/homebrew/opt/sqlite3/lib

BOOST_ROOT			= /usr/local/homebrew/opt/boost
BOOST_INCLUDE		= -I$(BOOST_ROOT)/include
BOOST_LIBS			= -L$(BOOST_ROOT)/lib -lboost_iostreams-mt -lboost_serialization-mt
