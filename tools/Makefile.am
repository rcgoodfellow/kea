AM_CPPFLAGS = $(BOOST_INCLUDES)

AM_CXXFLAGS = $(KEA_CXXFLAGS)

if USE_STATIC_LINK
AM_LDFLAGS = -static
endif

CLEANFILES = *.gcno *.gcda
DISTCLEANFILES = path_replacer.sh

if GENERATE_DOCS

noinst_PROGRAMS = system_messages
system_messages_SOURCES = system_messages.cc

# For bare distcheck
EXTRA_DIST = system_messages

# Scripts for Cassandra support. Ultimately those scripts will have to be
# incorporated in DataStax cpp-driver source tree, but until that happens,
# let's keep them in Kea rpo
EXTRA_DIST += cql_config cql_config_defines.sh

endif
