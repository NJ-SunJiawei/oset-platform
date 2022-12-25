#include "tooltips.hpp"
#include <glib.h>
#include <gtkmm.h>
#include <iostream>

int main() {
  auto app = Gtk::Application::create();
  Gsv::init();

  auto get_markdown_tooltip = [](const std::string &input) {
    auto tooltip = std::make_unique<Tooltip>([&](Tooltip &tooltip) {
      tooltip.insert_markdown(input);
    });
    tooltip->show();
    return tooltip;
  };

  auto get_doxygen_tooltip = [](const std::string &input, bool remove_delimiters) {
    auto tooltip = std::make_unique<Tooltip>([&](Tooltip &tooltip) {
      tooltip.insert_doxygen(input, remove_delimiters);
    });
    tooltip->show();
    return tooltip;
  };

  auto get_docstring_tooltip = [](const std::string &input) {
    auto tooltip = std::make_unique<Tooltip>([&](Tooltip &tooltip) {
      tooltip.insert_docstring(input);
    });
    tooltip->show();
    return tooltip;
  };

  // Testing insert_markdown():
  {
    auto tooltip = get_markdown_tooltip("");
    g_assert(tooltip->buffer->get_text() == "");
  }
  {
    auto tooltip = get_markdown_tooltip("test");
    g_assert(tooltip->buffer->get_text() == "test");
  }
  {
    auto tooltip = get_markdown_tooltip("test\ntest");
    g_assert(tooltip->buffer->get_text() == "test test");
  }
  {
    auto tooltip = get_markdown_tooltip("test\n\ntest");
    g_assert(tooltip->buffer->get_text() == "test\n\ntest");
  }
  {
    auto tooltip = get_markdown_tooltip("test\n\ntest\n\n");
    g_assert(tooltip->buffer->get_text() == "test\n\ntest");
  }
  {
    auto tooltip = get_markdown_tooltip("\\# test");
    auto buffer = tooltip->buffer;
    g_assert(buffer->get_text() == "# test");
  }
  {
    auto tooltip = get_markdown_tooltip("test\\\ntest");
    g_assert(tooltip->buffer->get_text() == "test\ntest");
  }
  {
    auto tooltip = get_markdown_tooltip("test&nbsp;&nbsp;test");
    g_assert(tooltip->buffer->get_text() == "test  test");
  }
  {
    auto tooltip = get_markdown_tooltip("test\n\n---\ntest");
    g_assert(tooltip->buffer->get_text() == "test\n\n---\ntest");
  }
  {
    auto tooltip = get_markdown_tooltip("test\n\n-----\ntest");
    g_assert(tooltip->buffer->get_text() == "test\n\n---\ntest");
  }
  {
    auto tooltip = get_markdown_tooltip("test\n\n***\n\ntest");
    g_assert(tooltip->buffer->get_text() == "test\n\n---\n\ntest");
  }
  {
    auto tooltip = get_markdown_tooltip("test\n\n___\n\ntest");
    g_assert(tooltip->buffer->get_text() == "test\n\n---\n\ntest");
  }
  {
    auto tooltip = get_markdown_tooltip("test\n\n---\n\ntest");
    g_assert(tooltip->buffer->get_text() == "test\n\n---\n\ntest");
  }
  {
    auto tooltip = get_markdown_tooltip("test\n\n---test---\n\ntest");
    g_assert(tooltip->buffer->get_text() == "test\n\n---test---\n\ntest");
  }
  {
    auto tooltip = get_markdown_tooltip("# test");
    auto buffer = tooltip->buffer;
    g_assert(buffer->get_text() == "test");
    g_assert(buffer->begin().starts_tag(tooltip->h1_tag));
    g_assert(buffer->get_iter_at_offset(4).ends_tag(tooltip->h1_tag));
  }
  {
    auto tooltip = get_markdown_tooltip("# test\ntest");
    auto buffer = tooltip->buffer;
    g_assert(buffer->get_text() == "test\n\ntest");
    g_assert(buffer->begin().starts_tag(tooltip->h1_tag));
    g_assert(buffer->get_iter_at_offset(4).ends_tag(tooltip->h1_tag));
  }
  {
    auto tooltip = get_markdown_tooltip("# test\n\ntest");
    auto buffer = tooltip->buffer;
    g_assert(buffer->get_text() == "test\n\ntest");
    g_assert(buffer->begin().starts_tag(tooltip->h1_tag));
    g_assert(buffer->get_iter_at_offset(4).ends_tag(tooltip->h1_tag));
  }
  {
    auto tooltip = get_markdown_tooltip("test\n# test\ntest");
    auto buffer = tooltip->buffer;
    g_assert(buffer->get_text() == "test\n\ntest\n\ntest");
    g_assert(buffer->get_iter_at_offset(6).starts_tag(tooltip->h1_tag));
    g_assert(buffer->get_iter_at_offset(10).ends_tag(tooltip->h1_tag));
  }
  {
    auto tooltip = get_markdown_tooltip("## test");
    auto buffer = tooltip->buffer;
    g_assert(buffer->get_text() == "test");
    g_assert(buffer->begin().starts_tag(tooltip->h2_tag));
    g_assert(buffer->get_iter_at_offset(4).ends_tag(tooltip->h2_tag));
  }
  {
    auto tooltip = get_markdown_tooltip("test\n====");
    auto buffer = tooltip->buffer;
    g_assert(buffer->get_text() == "test");
    g_assert(buffer->begin().starts_tag(tooltip->h1_tag));
    g_assert(buffer->get_iter_at_offset(4).ends_tag(tooltip->h1_tag));
  }
  {
    auto tooltip = get_markdown_tooltip("test\n----");
    auto buffer = tooltip->buffer;
    g_assert(buffer->get_text() == "test");
    g_assert(buffer->begin().starts_tag(tooltip->h2_tag));
    g_assert(buffer->get_iter_at_offset(4).ends_tag(tooltip->h2_tag));
  }
  {
    auto tooltip = get_markdown_tooltip("### test");
    auto buffer = tooltip->buffer;
    g_assert(buffer->get_text() == "test");
    g_assert(buffer->begin().starts_tag(tooltip->h3_tag));
    g_assert(buffer->get_iter_at_offset(4).ends_tag(tooltip->h3_tag));
  }
  {
    auto tooltip = get_markdown_tooltip("_test_");
    auto buffer = tooltip->buffer;
    g_assert(buffer->get_text() == "test");
    g_assert(buffer->begin().starts_tag(tooltip->italic_tag));
    g_assert(buffer->get_iter_at_offset(4).ends_tag(tooltip->italic_tag));
  }
  {
    auto tooltip = get_markdown_tooltip("*test*");
    auto buffer = tooltip->buffer;
    g_assert(buffer->get_text() == "test");
    g_assert(buffer->begin().starts_tag(tooltip->italic_tag));
    g_assert(buffer->get_iter_at_offset(4).ends_tag(tooltip->italic_tag));
    g_assert(!buffer->begin().starts_tag(tooltip->bold_tag));
    g_assert(!buffer->get_iter_at_offset(4).ends_tag(tooltip->bold_tag));
  }
  {
    auto tooltip = get_markdown_tooltip("*test* test");
    auto buffer = tooltip->buffer;
    g_assert(buffer->get_text() == "test test");
    g_assert(buffer->begin().starts_tag(tooltip->italic_tag));
    g_assert(buffer->get_iter_at_offset(4).ends_tag(tooltip->italic_tag));
    g_assert(!buffer->begin().starts_tag(tooltip->bold_tag));
    g_assert(!buffer->get_iter_at_offset(4).ends_tag(tooltip->bold_tag));
  }
  {
    auto tooltip = get_markdown_tooltip("test*test*test");
    auto buffer = tooltip->buffer;
    g_assert(buffer->get_text() == "testtesttest");
    g_assert(buffer->get_iter_at_offset(4).starts_tag(tooltip->italic_tag));
    g_assert(buffer->get_iter_at_offset(8).ends_tag(tooltip->italic_tag));
  }
  {
    auto tooltip = get_markdown_tooltip("test**test**test");
    auto buffer = tooltip->buffer;
    g_assert(buffer->get_text() == "testtesttest");
    g_assert(buffer->get_iter_at_offset(4).starts_tag(tooltip->bold_tag));
    g_assert(buffer->get_iter_at_offset(8).ends_tag(tooltip->bold_tag));
  }
  {
    auto tooltip = get_markdown_tooltip("**test**");
    auto buffer = tooltip->buffer;
    g_assert(buffer->get_text() == "test");
    g_assert(buffer->begin().starts_tag(tooltip->bold_tag));
    g_assert(buffer->get_iter_at_offset(4).ends_tag(tooltip->bold_tag));
    g_assert(!buffer->begin().starts_tag(tooltip->italic_tag));
    g_assert(!buffer->get_iter_at_offset(4).ends_tag(tooltip->italic_tag));
  }
  {
    auto tooltip = get_markdown_tooltip("__test__");
    auto buffer = tooltip->buffer;
    g_assert(buffer->get_text() == "test");
    g_assert(buffer->begin().starts_tag(tooltip->bold_tag));
    g_assert(buffer->get_iter_at_offset(4).ends_tag(tooltip->bold_tag));
    g_assert(!buffer->begin().starts_tag(tooltip->italic_tag));
    g_assert(!buffer->get_iter_at_offset(4).ends_tag(tooltip->italic_tag));
  }
  {
    auto tooltip = get_markdown_tooltip("_*test*_");
    auto buffer = tooltip->buffer;
    g_assert(buffer->get_text() == "test");
    g_assert(!buffer->begin().starts_tag(tooltip->bold_tag));
    g_assert(!buffer->get_iter_at_offset(4).ends_tag(tooltip->bold_tag));
    g_assert(buffer->begin().starts_tag(tooltip->italic_tag));
    g_assert(buffer->get_iter_at_offset(4).ends_tag(tooltip->italic_tag));
  }
  {
    auto tooltip = get_markdown_tooltip("***test***");
    auto buffer = tooltip->buffer;
    g_assert(buffer->get_text() == "test");
    g_assert(buffer->begin().starts_tag(tooltip->bold_tag));
    g_assert(buffer->begin().starts_tag(tooltip->italic_tag));
    g_assert(buffer->get_iter_at_offset(4).ends_tag(tooltip->bold_tag));
    g_assert(buffer->get_iter_at_offset(4).ends_tag(tooltip->italic_tag));
  }
  {
    auto tooltip = get_markdown_tooltip("**_test_**");
    auto buffer = tooltip->buffer;
    g_assert(buffer->get_text() == "test");
    g_assert(buffer->begin().starts_tag(tooltip->bold_tag));
    g_assert(buffer->begin().starts_tag(tooltip->italic_tag));
    g_assert(buffer->get_iter_at_offset(4).ends_tag(tooltip->bold_tag));
    g_assert(buffer->get_iter_at_offset(4).ends_tag(tooltip->italic_tag));
  }
  {
    auto tooltip = get_markdown_tooltip("~~test~~");
    auto buffer = tooltip->buffer;
    g_assert(buffer->get_text() == "test");
    g_assert(buffer->begin().starts_tag(tooltip->strikethrough_tag));
    g_assert(buffer->get_iter_at_offset(4).ends_tag(tooltip->strikethrough_tag));
  }
  {
    auto tooltip = get_markdown_tooltip("~~test");
    auto buffer = tooltip->buffer;
    g_assert(buffer->get_text() == "~~test");
  }
  {
    auto tooltip = get_markdown_tooltip("~~test~~ test");
    auto buffer = tooltip->buffer;
    g_assert(buffer->get_text() == "test test");
    g_assert(buffer->begin().starts_tag(tooltip->strikethrough_tag));
    g_assert(buffer->get_iter_at_offset(4).ends_tag(tooltip->strikethrough_tag));
  }
  {
    auto tooltip = get_markdown_tooltip("~~*test*~~");
    auto buffer = tooltip->buffer;
    g_assert(buffer->get_text() == "test");
    g_assert(buffer->begin().starts_tag(tooltip->strikethrough_tag));
    g_assert(buffer->begin().starts_tag(tooltip->italic_tag));
    g_assert(buffer->get_iter_at_offset(4).ends_tag(tooltip->strikethrough_tag));
    g_assert(buffer->get_iter_at_offset(4).ends_tag(tooltip->italic_tag));
  }
  {
    auto tooltip = get_markdown_tooltip("test_test");
    auto buffer = tooltip->buffer;
    g_assert(buffer->get_text() == "test_test");
  }
  {
    auto tooltip = get_markdown_tooltip("_test_test_");
    auto buffer = tooltip->buffer;
    g_assert(buffer->get_text() == "test_test");
    g_assert(buffer->begin().starts_tag(tooltip->italic_tag));
    g_assert(buffer->get_iter_at_offset(9).ends_tag(tooltip->italic_tag));
  }
  {
    auto tooltip = get_markdown_tooltip("2 * 2 * 2");
    auto buffer = tooltip->buffer;
    g_assert(buffer->get_text() == "2 * 2 * 2");
  }
  {
    auto tooltip = get_markdown_tooltip("* *2*");
    auto buffer = tooltip->buffer;
    g_assert(buffer->get_text() == "* 2");
  }
  {
    auto tooltip = get_markdown_tooltip("* 2*");
    auto buffer = tooltip->buffer;
    g_assert(buffer->get_text() == "* 2*");
  }
  {
    auto tooltip = get_markdown_tooltip("*");
    auto buffer = tooltip->buffer;
    g_assert(buffer->get_text() == "*");
  }
  {
    auto tooltip = get_markdown_tooltip("\\*test\\*");
    auto buffer = tooltip->buffer;
    g_assert(buffer->get_text() == "*test*");
  }
  {
    auto tooltip = get_markdown_tooltip("*\\*test\\**");
    auto buffer = tooltip->buffer;
    g_assert(buffer->get_text() == "*test*");
    g_assert(buffer->get_iter_at_offset(0).starts_tag(tooltip->italic_tag));
    g_assert(buffer->get_iter_at_offset(6).ends_tag(tooltip->italic_tag));
  }
  {
    auto tooltip = get_markdown_tooltip("**test _test_**");
    auto buffer = tooltip->buffer;
    g_assert(buffer->get_text() == "test test");
    g_assert(buffer->get_iter_at_offset(0).starts_tag(tooltip->bold_tag));
    g_assert(buffer->get_iter_at_offset(9).ends_tag(tooltip->bold_tag));
    g_assert(buffer->get_iter_at_offset(5).starts_tag(tooltip->italic_tag));
    g_assert(buffer->get_iter_at_offset(9).ends_tag(tooltip->italic_tag));
  }
  {
    auto tooltip = get_markdown_tooltip("_test _test_");
    auto buffer = tooltip->buffer;
    g_assert(buffer->get_text() == "_test test");
  }
  {
    auto tooltip = get_markdown_tooltip("_test _test __test __test");
    auto buffer = tooltip->buffer;
    g_assert(buffer->get_text() == "_test _test __test __test");
  }
  {
    auto tooltip = get_markdown_tooltip("`test`");
    auto buffer = tooltip->buffer;
    g_assert(buffer->get_text() == "test");
    g_assert(buffer->begin().starts_tag(tooltip->code_tag));
    g_assert(buffer->get_iter_at_offset(4).ends_tag(tooltip->code_tag));
  }
  {
    auto tooltip = get_markdown_tooltip("test `test` test");
    auto buffer = tooltip->buffer;
    g_assert(buffer->get_text() == "test test test");
    g_assert(buffer->get_iter_at_offset(5).starts_tag(tooltip->code_tag));
    g_assert(buffer->get_iter_at_offset(9).ends_tag(tooltip->code_tag));
  }
  {
    auto tooltip = get_markdown_tooltip("``te`st``");
    auto buffer = tooltip->buffer;
    g_assert(buffer->get_text() == "te`st");
    g_assert(buffer->begin().starts_tag(tooltip->code_tag));
    g_assert(buffer->get_iter_at_offset(5).ends_tag(tooltip->code_tag));
  }
  {
    auto tooltip = get_markdown_tooltip("# Test");
    auto buffer = tooltip->buffer;
    g_assert(buffer->get_text() == "Test");
  }
  {
    auto tooltip = get_markdown_tooltip("# Test\ntest");
    auto buffer = tooltip->buffer;
    g_assert(buffer->get_text() == "Test\n\ntest");
  }
  {
    auto tooltip = get_markdown_tooltip("test\n\n# Test\ntest");
    auto buffer = tooltip->buffer;
    g_assert(buffer->get_text() == "test\n\nTest\n\ntest");
  }
  {
    auto tooltip = get_markdown_tooltip("```\ntest\n```");
    auto buffer = tooltip->buffer;
    g_assert(buffer->get_text() == "test");
    g_assert(buffer->begin().starts_tag(tooltip->code_block_tag));
  }
  {
    auto tooltip = get_markdown_tooltip("```\ntest\n```\ntest");
    auto buffer = tooltip->buffer;
    g_assert(buffer->get_text() == "test\ntest");
    g_assert(buffer->begin().starts_tag(tooltip->code_block_tag));
    g_assert(buffer->get_iter_at_offset(5).ends_tag(tooltip->code_block_tag));
  }
  {
    auto tooltip = get_markdown_tooltip("test\n\n```c++\ntest\n```\n\ntest");
    auto buffer = tooltip->buffer;
    g_assert(buffer->get_text() == "test\n\ntest\n\ntest");
    g_assert(buffer->get_iter_at_offset(6).starts_tag(tooltip->code_block_tag));
    g_assert(buffer->get_iter_at_offset(11).ends_tag(tooltip->code_block_tag));
  }
  {
    auto tooltip = get_markdown_tooltip("test\n```c++\ntest\n```\ntest");
    auto buffer = tooltip->buffer;
    g_assert(buffer->get_text() == "test\ntest\ntest");
    g_assert(buffer->get_iter_at_offset(5).starts_tag(tooltip->code_block_tag));
    g_assert(buffer->get_iter_at_offset(10).ends_tag(tooltip->code_block_tag));
  }
  {
    auto tooltip = get_markdown_tooltip("http://test.com");
    g_assert(tooltip->buffer->get_text() == "http://test.com");
    auto buffer = tooltip->buffer;
    g_assert(buffer->begin().starts_tag(tooltip->link_tag));
    g_assert(buffer->get_iter_at_offset(15).ends_tag(tooltip->link_tag));
  }
  {
    auto tooltip = get_markdown_tooltip("http://test.com.");
    g_assert(tooltip->buffer->get_text() == "http://test.com.");
    auto buffer = tooltip->buffer;
    g_assert(buffer->begin().starts_tag(tooltip->link_tag));
    g_assert(buffer->get_iter_at_offset(15).ends_tag(tooltip->link_tag));
  }
  {
    auto tooltip = get_markdown_tooltip("http://test.com#test.");
    g_assert(tooltip->buffer->get_text() == "http://test.com#test.");
    auto buffer = tooltip->buffer;
    g_assert(buffer->begin().starts_tag(tooltip->link_tag));
    g_assert(buffer->get_iter_at_offset(20).ends_tag(tooltip->link_tag));
  }
  {
    auto tooltip = get_markdown_tooltip("[test](http://test.com)");
    g_assert(tooltip->buffer->get_text() == "test");
    auto buffer = tooltip->buffer;
    g_assert(buffer->begin().starts_tag(tooltip->link_tag));
    g_assert(buffer->get_iter_at_offset(4).ends_tag(tooltip->link_tag));
  }
  {
    auto tooltip = get_markdown_tooltip("[`test`](http://test.com)");
    g_assert(tooltip->buffer->get_text() == "test");
    auto buffer = tooltip->buffer;
    g_assert(buffer->begin().starts_tag(tooltip->link_tag));
    g_assert(buffer->get_iter_at_offset(4).ends_tag(tooltip->link_tag));
    g_assert(buffer->begin().starts_tag(tooltip->code_tag));
    g_assert(buffer->get_iter_at_offset(4).ends_tag(tooltip->code_tag));
  }
  {
    auto tooltip = get_markdown_tooltip("[1]: http://test.com");
    g_assert(tooltip->buffer->get_text() == "");
  }
  {
    auto tooltip = get_markdown_tooltip("[`test`]\n\n[`test`]: http://test.com");
    g_assert(tooltip->buffer->get_text() == "test");
    auto buffer = tooltip->buffer;
    g_assert(buffer->begin().starts_tag(tooltip->link_tag));
    g_assert(buffer->get_iter_at_offset(4).ends_tag(tooltip->link_tag));
    g_assert(buffer->begin().starts_tag(tooltip->code_tag));
    g_assert(buffer->get_iter_at_offset(4).ends_tag(tooltip->code_tag));

    g_assert(tooltip->reference_links.size() == 1);
    g_assert(tooltip->reference_links.begin()->second == "test");
    g_assert(tooltip->references.size() == 1);
    g_assert(tooltip->references.begin()->second == "http://test.com");
  }
  {
    auto tooltip = get_markdown_tooltip("[]");
    g_assert(tooltip->buffer->get_text() == "[]");
    g_assert(!tooltip->buffer->get_iter_at_offset(1).has_tag(tooltip->link_tag));
  }
  {
    auto tooltip = get_markdown_tooltip("[`test`]");
    g_assert(tooltip->buffer->get_text() == "[test]");
    g_assert(!tooltip->buffer->get_iter_at_offset(3).has_tag(tooltip->link_tag));
    g_assert(tooltip->buffer->get_iter_at_offset(3).has_tag(tooltip->code_tag));
  }
  {
    auto tooltip = get_markdown_tooltip("[`text`][test]\n\n[test]: http://test.com");
    g_assert(tooltip->buffer->get_text() == "text");
    auto buffer = tooltip->buffer;
    g_assert(buffer->begin().starts_tag(tooltip->link_tag));
    g_assert(buffer->get_iter_at_offset(4).ends_tag(tooltip->link_tag));
    g_assert(buffer->begin().starts_tag(tooltip->code_tag));
    g_assert(buffer->get_iter_at_offset(4).ends_tag(tooltip->code_tag));

    g_assert(tooltip->reference_links.size() == 1);
    g_assert(tooltip->reference_links.begin()->second == "test");
    g_assert(tooltip->references.size() == 1);
    g_assert(tooltip->references.begin()->second == "http://test.com");
  }
  {
    auto tooltip = get_markdown_tooltip("- test");
    g_assert(tooltip->buffer->get_text() == "- test");
  }
  {
    auto tooltip = get_markdown_tooltip("test\n- test");
    g_assert(tooltip->buffer->get_text() == "test\n- test");
  }
  {
    auto tooltip = get_markdown_tooltip("test\n\n- test");
    g_assert(tooltip->buffer->get_text() == "test\n\n- test");
  }
  {
    auto tooltip = get_markdown_tooltip("- test\ntest");
    g_assert(tooltip->buffer->get_text() == "- test test");
  }
  {
    auto tooltip = get_markdown_tooltip("- test\n\ntest");
    g_assert(tooltip->buffer->get_text() == "- test\n\ntest");
  }
  {
    auto tooltip = get_markdown_tooltip("- test\n  - test");
    g_assert(tooltip->buffer->get_text() == "- test\n  - test");
  }
  {
    auto tooltip = get_markdown_tooltip("1. test\n2. test\n  30. test\n42. test");
    g_assert(tooltip->buffer->get_text() == "1. test\n2. test\n  30. test\n42. test");
  }

  // Testing wrap_lines():
  {
    auto tooltip = get_markdown_tooltip("");
    tooltip->wrap_lines();
    g_assert(tooltip->buffer->get_text() == "");
  }
  {
    auto tooltip = get_markdown_tooltip("test\ntest");
    tooltip->wrap_lines();
    g_assert(tooltip->buffer->get_text() == "test test");
  }
  {
    auto tooltip = get_markdown_tooltip("test test test test test test test test test test test test test test test test test test test test test");
    tooltip->wrap_lines();
    g_assert(tooltip->buffer->get_text() == "test test test test test test test test test test test test test test test test\ntest test test test test");
  }
  {
    auto tooltip = get_markdown_tooltip("test test test test test test test test test test test test test test test testt test test test test test");
    tooltip->wrap_lines();
    g_assert(tooltip->buffer->get_text() == "test test test test test test test test test test test test test test test testt\ntest test test test test");
  }
  {
    auto tooltip = get_markdown_tooltip("test test test test test test test test test test test test test test test testtt test test test test test");
    tooltip->wrap_lines();
    g_assert(tooltip->buffer->get_text() == "test test test test test test test test test test test test test test test\ntesttt test test test test test");
  }
  {
    auto tooltip = get_markdown_tooltip("testtesttesttesttesttesttesttesttesttesttesttesttesttesttesttesttesttesttesttesttesttesttesttest\ntest test");
    tooltip->wrap_lines();
    g_assert(tooltip->buffer->get_text() == "testtesttesttesttesttesttesttesttesttesttesttesttesttesttesttesttesttesttesttesttesttesttesttest\ntest test");
  }
  {
    auto tooltip = get_markdown_tooltip("testtesttesttesttesttesttesttesttesttesttesttesttesttesttesttesttesttesttesttesttesttesttesttest test test");
    tooltip->wrap_lines();
    g_assert(tooltip->buffer->get_text() == "testtesttesttesttesttesttesttesttesttesttesttesttesttesttesttesttesttesttesttesttesttesttesttest\ntest test");
  }

  // Testing insert_doxygen
  {
    auto tooltip = get_doxygen_tooltip("", false);
    g_assert(tooltip->buffer->get_text() == "");
  }
  {
    auto tooltip = get_doxygen_tooltip("", true);
    g_assert(tooltip->buffer->get_text() == "");
  }
  {
    auto tooltip = get_doxygen_tooltip("test", false);
    g_assert(tooltip->buffer->get_text() == "test");
  }
  {
    auto tooltip = get_doxygen_tooltip("`test`", false);
    g_assert(tooltip->buffer->get_text() == "test");
  }
  {
    auto tooltip = get_doxygen_tooltip("*test*", false);
    g_assert(tooltip->buffer->get_text() == "test");
  }
  {
    auto tooltip = get_doxygen_tooltip("**test**", false);
    g_assert(tooltip->buffer->get_text() == "test");
  }
  {
    auto tooltip = get_doxygen_tooltip("\\ test", false);
    g_assert(tooltip->buffer->get_text() == "test");
  }
  {
    auto tooltip = get_doxygen_tooltip("\\%test", false);
    g_assert(tooltip->buffer->get_text() == "%test");
  }
  {
    auto tooltip = get_doxygen_tooltip("\\\\test", false);
    g_assert(tooltip->buffer->get_text() == "\\test");
  }
  {
    auto tooltip = get_doxygen_tooltip("\\@test", false);
    g_assert(tooltip->buffer->get_text() == "@test");
  }
  {
    auto tooltip = get_doxygen_tooltip("@test", false);
    g_assert(tooltip->buffer->get_text() == "@test");
  }
  {
    auto tooltip = get_doxygen_tooltip("%test", false);
    g_assert(tooltip->buffer->get_text() == "test");
  }
  {
    auto tooltip = get_doxygen_tooltip("\\a test", false);
    g_assert(tooltip->buffer->get_text() == "test");
  }
  {
    auto tooltip = get_doxygen_tooltip("\\a __test", false);
    g_assert(tooltip->buffer->get_text() == "__test");
  }
  {
    auto tooltip = get_doxygen_tooltip("\\a &str[0]", false);
    g_assert(tooltip->buffer->get_text() == "&str[0]");
  }
  {
    auto tooltip = get_doxygen_tooltip("\\a str.data()", false);
    g_assert(tooltip->buffer->get_text() == "str.data()");
  }
  {
    auto tooltip = get_doxygen_tooltip("@a str@data()", false);
    g_assert(tooltip->buffer->get_text() == "str@data()");
  }
  {
    auto tooltip = get_doxygen_tooltip("\"@a str@data()\"", false);
    g_assert(tooltip->buffer->get_text() == "\"@a str@data()\"");
  }
  {
    auto tooltip = get_doxygen_tooltip("\\a str.data().", false);
    g_assert(tooltip->buffer->begin().starts_tag(tooltip->italic_tag));
    auto it = tooltip->buffer->end();
    it.backward_char();
    g_assert(it.ends_tag(tooltip->italic_tag));
    g_assert(tooltip->buffer->get_text() == "str.data().");
  }
  {
    auto tooltip = get_doxygen_tooltip("<tt>_test_</tt>", false);
    g_assert(tooltip->buffer->get_text() == "_test_");
  }
  {
    auto tooltip = get_doxygen_tooltip("<em>_test_</em>", false);
    g_assert(tooltip->buffer->get_text() == "_test_");
  }
  {
    auto tooltip = get_doxygen_tooltip("<b>_test_</b>", false);
    g_assert(tooltip->buffer->get_text() == "_test_");
  }
  {
    auto tooltip = get_doxygen_tooltip(R"(/**
* Constructor that sets the time to a given value.
*
* @param timemillis is a number of milliseconds
*        passed since Jan 1, 1970.
*/)",
                                       true);
    g_assert(tooltip->buffer->get_text() == R"(Constructor that sets the time to a given value.

Parameters:
- timemillis is a number of milliseconds passed since Jan 1, 1970.)");
  }
  {
    auto tooltip = get_doxygen_tooltip(R"(/** Returns <tt>true</tt> if @a test points to the start.
* Test pointing to the \\n of a \\r\\n pair will not.
*
*
* @return Whether @a test is at the end of a line.
*/)",
                                       true);
    g_assert(tooltip->buffer->get_text() == R"(Returns true if test points to the start. Test pointing to the \n of a \r\n pair
will not.

Returns Whether test is at the end of a line.)");
  }
  {
    auto tooltip = get_doxygen_tooltip(R"(/**
* test
* \a test
* test
*/)",
                                       true);
    g_assert(tooltip->buffer->get_text() == "test test test");
  }
  {
    auto tooltip = get_doxygen_tooltip(R"(/**
* Testing
* - t
*   - t2
* - asd
* 
* More testing
* end
*/)",
                                       true);
    g_assert(tooltip->buffer->get_text() == R"(Testing
- t
  - t2
- asd

More testing end)");
  }
  {
    auto tooltip = get_doxygen_tooltip(R"(/**
  * Testing
  * 
  * - t
  *   - t2
  * - asd
  * 
  * More testing
  * end
  */)",
                                       true);
    g_assert(tooltip->buffer->get_text() == R"(Testing

- t
  - t2
- asd

More testing end)");
  }
  {
    auto tooltip = get_doxygen_tooltip(R"(    //! A normal member taking two arguments and returning an integer value.
    /*!
      \param a an integer argument.
      \param s a constant character pointer.
      \return The test results
      \sa Test(), ~Test(), testMeToo() and publicVar()
      \test testing
      @test testing
    */)",
                                       true);
    g_assert(tooltip->buffer->get_text() == R"(A normal member taking two arguments and returning an integer value.

Parameters:
- a an integer argument.
- s a constant character pointer.

Returns The test results

See also Test(), ~Test(), testMeToo() and publicVar()

\test testing

@test testing)");
  }
  {
    auto tooltip = get_doxygen_tooltip(R"(/*! \brief Brief description.
  *          Brief description continued.
  *          Brief description continued.
  *          Brief description continued.
  *
  *     Detailed description starts here.
  */)",
                                       true);
    g_assert(tooltip->buffer->get_text() == R"(Brief description. Brief description continued. Brief description continued.
Brief description continued.

Detailed description starts here.)");
  }
  {
    auto tooltip = get_doxygen_tooltip(R"(/**
* \code
* int a = 2;
* \endcode
*/)",
                                       true);
    g_assert(tooltip->buffer->get_text() == "int a = 2;");
  }
  {
    auto tooltip = get_doxygen_tooltip(R"(/**
* @code
* int a = 2;
* @endcode
*/)",
                                       true);
    g_assert(tooltip->buffer->get_text() == "int a = 2;");
  }
  {
    auto tooltip = get_doxygen_tooltip(R"(/**
* test
* \code
* int a = 2;
* \endcode
* test
*/)",
                                       true);
    g_assert(tooltip->buffer->get_text() == "test\nint a = 2;\ntest");
  }
  {
    auto tooltip = get_doxygen_tooltip(R"(/**
* test
*
* \code
* int a = 2;
* \endcode
*
* test
*/)",
                                       true);
    g_assert(tooltip->buffer->get_text() == "test\n\nint a = 2;\n\ntest");
  }
  {
    auto tooltip = get_doxygen_tooltip("<code>*this == \"\"</code>", false);
    g_assert(tooltip->buffer->get_text() == "*this == \"\"");
  }
  {
    auto tooltip = get_doxygen_tooltip("In some cases (`@{<-n>}` or `<branchname>@{upstream}`), the expression", false);
    g_assert(tooltip->buffer->get_text() == "In some cases (@{<-n>} or <branchname>@{upstream}), the expression");
  }
  {
    auto tooltip = get_doxygen_tooltip("/** The intent of test*/", true);
    g_assert(tooltip->buffer->get_text() == "The intent of test");
  }
  {
    auto tooltip = get_doxygen_tooltip("/** The intent of test  */", true);
    g_assert(tooltip->buffer->get_text() == "The intent of test");
  }
  {
    auto tooltip = get_doxygen_tooltip("/** Don't insert \"[PATCH]\" in testing*/", true);
    g_assert(tooltip->buffer->get_text() == "Don't insert \"[PATCH]\" in testing");
  }
  {
    auto tooltip = get_doxygen_tooltip("/** Don't insert \"[PAT\\\\CH]\" in testing*/", true);
    g_assert(tooltip->buffer->get_text() == "Don't insert \"[PAT\\CH]\" in testing");
  }
  {
    auto tooltip = get_doxygen_tooltip("/** Don't insert \"[PAT\\\"CH]\" in testing*/", true);
    g_assert(tooltip->buffer->get_text() == "Don't insert \"[PAT\"CH]\" in testing");
  }
  {
    auto tooltip = get_doxygen_tooltip("/** Don't insert \" *test* \" in testing*/", true);
    g_assert(tooltip->buffer->get_text() == "Don't insert \" *test* \" in testing");
  }
  {
    auto tooltip = get_doxygen_tooltip("/**         testing*/", true);
    g_assert(tooltip->buffer->get_text() == "testing");
  }
  {
    auto tooltip = get_doxygen_tooltip("/**   # testing*/", true);
    g_assert(tooltip->buffer->get_text() == "testing");
  }

  // Testing insert_docstring
  {
    auto tooltip = get_docstring_tooltip(R"(**Some** *Python* \` ``testing`` `routine`.
Section
====
>>> test
test

Example
  >>> test
  test

  test

Example::

  test

  test

Subsection
----

A `link <https://test.test>`_)");
    g_assert(tooltip->buffer->get_text() == R"(Some Python ` testing routine.
Section
>>> test
test

Example
  >>> test
  test

  test

Example:

  test

  test

Subsection

A link)");
    auto it = tooltip->buffer->begin();
    g_assert(it.starts_tag(tooltip->bold_tag));
    it.forward_chars(4);
    g_assert(it.ends_tag(tooltip->bold_tag));
    it.forward_chars(1);
    g_assert(it.starts_tag(tooltip->italic_tag));
    it.forward_chars(6);
    g_assert(it.ends_tag(tooltip->italic_tag));
    it.forward_chars(3);
    g_assert(it.starts_tag(tooltip->code_tag));
    it.forward_chars(7);
    g_assert(it.ends_tag(tooltip->code_tag));
    it.forward_chars(1);
    g_assert(it.starts_tag(tooltip->code_tag));
    it.forward_chars(7);
    g_assert(it.ends_tag(tooltip->code_tag));
    it.forward_chars(2);
    g_assert(it.starts_tag(tooltip->h1_tag));
    it.forward_chars(7);
    g_assert(it.ends_tag(tooltip->h1_tag));
    it.forward_chars(1);
    g_assert(it.starts_tag(tooltip->code_block_tag));
    it.forward_chars(13);
    g_assert(it.ends_tag(tooltip->code_block_tag));
    it.forward_chars(10);
    g_assert(it.starts_tag(tooltip->code_block_tag));
    it.forward_chars(25);
    g_assert(it.ends_tag(tooltip->code_block_tag));
    it.forward_chars(12);
    g_assert(it.starts_tag(tooltip->code_block_tag));
    it.forward_chars(14);
    g_assert(it.ends_tag(tooltip->code_block_tag));
    it.forward_chars(2);
    g_assert(it.starts_tag(tooltip->h2_tag));
    it.forward_chars(10);
    g_assert(it.ends_tag(tooltip->h2_tag));
    it.forward_chars(4);
    g_assert(it.starts_tag(tooltip->link_tag));
    it.forward_chars(4);
    g_assert(it.ends_tag(tooltip->link_tag));
    g_assert(tooltip->links.begin()->second == "https://test.test");
  }
}
