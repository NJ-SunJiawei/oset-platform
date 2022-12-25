#include "grep.hpp"
#include <glib.h>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  const gchar *end;
  if(!g_utf8_validate(reinterpret_cast<const char *>(data), size, &end))
    return 0;

  Grep grep({}, {}, false, false);
  grep.get_location(std::string(reinterpret_cast<const char *>(data), size), true, true);
  return 0;
}
