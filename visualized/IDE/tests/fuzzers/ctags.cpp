#include "ctags.hpp"
#include <glib.h>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  const gchar *end;
  if(!g_utf8_validate(reinterpret_cast<const char *>(data), size, &end))
    return 0;

  std::map<std::string, std::list<std::string>> variables;
  Ctags ctags({}, true, true);
  ctags.get_location(std::string(reinterpret_cast<const char *>(data), size), true, false);
  return 0;
}
