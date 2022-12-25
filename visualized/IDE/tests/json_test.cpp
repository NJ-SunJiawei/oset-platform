#include "config.hpp"
#include "json.hpp"
#include <glib.h>
#include <iomanip>
#include <iostream>
#include <sstream>

int main() {
  std::string json = R"({
  "integer": 3,
  "integer_as_string": "3",
  "string": "some\ntext",
  "string2": "1test",
  "boolean": true,
  "boolean_as_integer": 1,
  "boolean_as_string1": "true",
  "boolean_as_string2": "1",
  "pi": 3.14,
  "pi_as_string": "3.14",
  "array": [
    1,
    3,
    3.14
  ],
  "array_with_strings": [
    "a",
    "b",
    "c"
  ],
  "object": {
    "integer": 3,
    "string": "some\ntext",
    "array": [
      1,
      3,
      3.14
    ]
  }
})";

  std::string json_no_indent;
  for(auto &chr : json) {
    if(chr != ' ' && chr != '\n')
      json_no_indent += chr;
  }

  {
    JSON j(Config::get().default_config());
    g_assert(!j.string("version").empty());
  }

  {
    JSON j(json);
    g_assert(j.to_string(2) == json);
    g_assert(j.to_string() == json_no_indent);

    {
      std::ostringstream ss;
      ss << std::setw(2) << j;
      g_assert(ss.str() == json);
    }
    {
      std::ostringstream ss;
      ss << j;
      g_assert(ss.str() == json_no_indent);
    }

    g_assert(j.integer("integer_as_string", JSON::ParseOptions::accept_string) == 3);
    g_assert(j.boolean("boolean_as_string1", JSON::ParseOptions::accept_string) == true);
    g_assert(j.boolean("boolean_as_string2", JSON::ParseOptions::accept_string) == true);
    g_assert(j.floating_point("pi_as_string", JSON::ParseOptions::accept_string) > 3.1 && j.floating_point("pi_as_string", JSON::ParseOptions::accept_string) < 3.2);

    g_assert(j.string("integer_as_string") == "3");
    try {
      j.integer("integer_as_string");
      g_assert(false);
    }
    catch(...) {
    }
    g_assert(j.string("boolean_as_string1") == "true");
    g_assert(j.string("boolean_as_string2") == "1");
    try {
      j.boolean("boolean_as_string1");
      j.boolean("boolean_as_string2");
      g_assert(false);
    }
    catch(...) {
    }
    g_assert(j.string("pi_as_string") == "3.14");
    try {
      j.boolean("pi_as_string");
      g_assert(false);
    }
    catch(...) {
    }

    g_assert(j.boolean("boolean_as_integer") == true);
    g_assert(j.floating_point("pi") >= 3.13 && j.floating_point("pi") <= 3.15);
    g_assert(j.floating_point("integer") > 2.9 && j.floating_point("integer") < 3.1);
    g_assert(j.integer("pi") == 3);

    j.object();
    g_assert(!j.children().empty());
    g_assert(!j.array("array").empty());
    j.child("array");

    try {
      j.object("array");
      g_assert(false);
    }
    catch(...) {
    }

    j.object("object");
    j.child("object");
    try {
      j.array("object");
      g_assert(false);
    }
    catch(...) {
    }
  }

  {
    JSON j(json);
    j.set("test", 2);
    g_assert(j.integer("test") == 2);
    g_assert(j.array("array").size() == 3);
    j.child("array").emplace_back(JSON());
    g_assert(j.array("array").size() == 4);

    try {
      j.child("object").emplace_back(JSON());
      g_assert(false);
    }
    catch(...) {
    }
  }

  {
    JSON j(json);
    g_assert(j.owner);
    g_assert(j.object("object").children().size() == 3);
    g_assert(!j.object("object").owner);
    auto owner = JSON::make_owner(j.object("object"));
    g_assert(owner.owner);
    g_assert(owner.children().size() == 3);
    for(auto &child : owner.children()) {
      g_assert(!child.first.empty());
      g_assert(!child.second.owner);
    }
    g_assert(owner.array("array").size() == 3);
    size_t count = 0;
    for(auto &e : owner.array("array")) {
      ++count;
      g_assert(e.integer() >= 1);
      g_assert(e.floating_point() > 0.5);
      g_assert(!e.owner);
    }
    g_assert(count == 3);

    g_assert(j.owner);
    g_assert(j.child("object").to_string() == "null");
    g_assert(!j.child("object").owner);
    g_assert(!j.child("array").owner);
    for(auto &child : j.children()) {
      g_assert(!child.first.empty());
      g_assert(!child.second.owner);
    }
    for(auto &e : j.array("array"))
      g_assert(!e.owner);
  }

  {
    JSON j;
    JSON child;
    g_assert(j.owner);
    g_assert(child.owner);
    child.set("a_string", "test");
    child.set("a_bool", true);
    g_assert(child.string("a_string") == "test");
    g_assert(child.child("a_string").string() == "test");
    g_assert(child.boolean("a_bool") == true);
    g_assert(child.child("a_bool").boolean() == true);

    j.set("an_object", std::move(child));
    assert(child.to_string() == "null");
    assert(!child.owner);
    assert(j.owner);
    assert(!j.object("an_object").owner);
    assert(!j.object("an_object").child("a_string").owner);
    assert(!j.object("an_object").child("a_bool").owner);
    g_assert(j.object("an_object").string("a_string") == "test");
    g_assert(j.object("an_object").child("a_string").string() == "test");
    g_assert(j.object("an_object").boolean("a_bool") == true);
    g_assert(j.object("an_object").child("a_bool").boolean() == true);
  }

  {
    JSON j(json);
    g_assert(!j.string_optional());
    g_assert(!j.integer_optional());
    g_assert(!j.boolean_optional());
    g_assert(!j.floating_point_optional());
    g_assert(!j.array_optional());
    g_assert(j.object_optional());

    g_assert(!j.string_optional("integer"));
    g_assert(j.integer_optional("integer"));
    g_assert(!j.boolean_optional("integer"));
    g_assert(j.floating_point_optional("integer"));
    g_assert(!j.array_optional("integer"));
    g_assert(!j.object_optional("integer"));

    g_assert(j.string_optional("string"));
    g_assert(!j.integer_optional("string"));
    g_assert(!j.boolean_optional("string"));
    g_assert(!j.floating_point_optional("string"));
    g_assert(!j.array_optional("string"));
    g_assert(!j.object_optional("string"));

    g_assert(!j.string_optional("array"));
    g_assert(!j.integer_optional("array"));
    g_assert(!j.boolean_optional("array"));
    g_assert(!j.floating_point_optional("array"));
    g_assert(j.array_optional("array"));
    g_assert(!j.object_optional("array"));

    g_assert(!j.string_optional("boolean"));
    g_assert(!j.integer_optional("boolean"));
    g_assert(j.boolean_optional("boolean"));
    g_assert(!j.floating_point_optional("boolean"));
    g_assert(!j.array_optional("boolean"));
    g_assert(!j.object_optional("boolean"));

    g_assert(!j.string_optional("pi"));
    g_assert(j.integer_optional("pi"));
    g_assert(!j.boolean_optional("pi"));
    g_assert(j.floating_point_optional("pi"));
    g_assert(!j.array_optional("pi"));
    g_assert(!j.object_optional("pi"));
  }

  {
    JSON j(json);
    g_assert(j.string_or("fail") == "fail");
    g_assert(j.integer_or(-1) == -1);
    g_assert(j.boolean_or(false) == false);
    g_assert(j.boolean_or(true) == true);
    g_assert(j.floating_point_or(-1.5) >= -1.6 && j.floating_point_or(-1.5) < -1.4);
    g_assert(j.array_or_empty().empty());
    g_assert(!j.children_or_empty().empty());

    g_assert(j.string_or("integer", "fail") == "fail");
    g_assert(j.integer_or("integer", -1) == 3);
    g_assert(j.boolean_or("integer", false) == false);
    g_assert(j.boolean_or("integer", true) == true);
    g_assert(j.floating_point_or("integer", -1.5) >= 2.9 && j.floating_point_or("integer", -1.5) < 3.1);
    g_assert(j.array_or_empty("integer").empty());
    g_assert(j.children_or_empty("integer").empty());

    g_assert(j.string_or("string", "fail") == "some\ntext");
    g_assert(j.integer_or("string", -1) == -1);
    g_assert(j.boolean_or("string", false) == false);
    g_assert(j.boolean_or("string", true) == true);
    g_assert(j.floating_point_or("string", -1.5) >= -1.6 && j.floating_point_or("string", -1.5) < -1.4);
    g_assert(j.array_or_empty("string").empty());
    g_assert(j.children_or_empty("string").empty());

    g_assert(j.string_or("array", "fail") == "fail");
    g_assert(j.integer_or("array", -1) == -1);
    g_assert(j.boolean_or("array", false) == false);
    g_assert(j.boolean_or("array", true) == true);
    g_assert(j.floating_point_or("array", -1.5) >= -1.6 && j.floating_point_or("array", -1.5) < -1.4);
    g_assert(!j.array_or_empty("array").empty());
    g_assert(j.children_or_empty("array").empty());

    g_assert(j.string_or("boolean", "fail") == "fail");
    g_assert(j.integer_or("boolean", -1) == -1);
    g_assert(j.boolean_or("boolean", false) == true);
    g_assert(j.floating_point_or("boolean", -1.5) >= -1.6 && j.floating_point_or("boolean", -1.5) < -1.4);
    g_assert(j.array_or_empty("boolean").empty());
    g_assert(j.children_or_empty("boolean").empty());

    g_assert(j.string_or("pi", "fail") == "fail");
    g_assert(j.integer_or("pi", -1) == 3);
    g_assert(j.boolean_or("pi", false) == false);
    g_assert(j.boolean_or("pi", true) == true);
    g_assert(j.floating_point_or("pi", -1.5) >= 3.1 && j.floating_point_or("pi", -1.5) < 3.2);
    g_assert(j.array_or_empty("pi").empty());
    g_assert(j.children_or_empty("pi").empty());
  }

  {
    JSON j(json);
    j.child("integer");
    j.child("array");
    j.child("object");
    j.remove("integer");
    j.remove("array");
    j.remove("object");
    try {
      j.child("integer");
      g_assert(false);
    }
    catch(...) {
    }
    try {
      j.child("array");
      g_assert(false);
    }
    catch(...) {
    }
    try {
      j.child("object");
      g_assert(false);
    }
    catch(...) {
    }
  }
}
