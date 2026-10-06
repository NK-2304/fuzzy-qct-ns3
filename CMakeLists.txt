build_lib(
  LIBNAME fuzzy-qct
  SOURCE_FILES
    model/qct-ared-queue-disc.cc
    model/fuzzy-engine.cc
    model/fuzzy-qct-queue-disc.cc
  HEADER_FILES
    model/qct-ared-queue-disc.h
    model/fuzzy-engine.h
    model/fuzzy-qct-queue-disc.h
  LIBRARIES_TO_LINK
    ${libcore}
    ${libnetwork}
    ${libtraffic-control}
    fuzzylite
)
