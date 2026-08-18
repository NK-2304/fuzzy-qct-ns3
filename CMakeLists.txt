build_lib(
  LIBNAME fuzzy-qct
  SOURCE_FILES
    model/qct-ared-queue-disc.cc
  HEADER_FILES
    model/qct-ared-queue-disc.h
  LIBRARIES_TO_LINK
    ${libtraffic-control}
)