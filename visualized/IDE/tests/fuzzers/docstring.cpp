#include "tooltips.hpp"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  const gchar *end;
  if(!g_utf8_validate(reinterpret_cast<const char *>(data), size, &end))
    return 0;

  class Init {
    Glib::RefPtr<Gtk::Application> app;

  public:
    Init() {
      app = Gtk::Application::create();
      Gsv::init();
    }
  };
  static Init init;
  static auto get_docstring_tooltip = [](const std::string &input) {
    auto tooltip = std::make_unique<Tooltip>([&](Tooltip &tooltip) {
      tooltip.insert_docstring(input);
    });
    tooltip->show();
    return tooltip;
  };

  get_docstring_tooltip(std::string(reinterpret_cast<const char *>(data), size));
  return 0;
}
