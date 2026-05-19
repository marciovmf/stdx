#define X_IMPL_TEST
#include <stdx_test.h>
#define X_IMPL_TML
#include <stdx_tml.h>

#include <stdio.h>
#include <string.h>
#include <stdint.h>

int test_parse_empty_document(void)
{
  TMLParseResult result;

  result = tml_parse("");

  ASSERT_TRUE(result.ok);
  ASSERT_TRUE(result.document != NULL);
  ASSERT_TRUE(result.document->node_count == 0);
  ASSERT_TRUE(result.document->entry_count == 0);

  tml_document_free(result.document);

  return 0;
}

int test_parse_single_node(void)
{
  char const* source = "root:\n";

  TMLParseResult result = tml_parse(source);
  ASSERT_TRUE(result.ok);
  ASSERT_TRUE(result.document->node_count == 1);

  TMLNode const* node = tml_root_node_at(result.document, 0);

  ASSERT_TRUE(node != NULL);
  ASSERT_TRUE(node->name.size == 4);
  ASSERT_TRUE(memcmp(node->name.data, "root", 4) == 0);

  ASSERT_TRUE(node->child_count == 0);
  ASSERT_TRUE(node->entry_count == 0);

  tml_document_free(result.document);

  return 0;
}

int test_parse_node_with_entries(void)
{

  char const* source = 
    "player:\n"
    "  enabled: true\n"
    "  health: 100\n";

  TMLParseResult result = tml_parse(source);
  ASSERT_TRUE(result.ok);

  TMLNode const* node = tml_root_node_at(result.document, 0);
  ASSERT_TRUE(node != NULL);
  ASSERT_TRUE(node->entry_count == 2);

  TMLEntry const* enabled = tml_node_find_entry(result.document, node, "enabled");
  ASSERT_TRUE(enabled != NULL);
  ASSERT_TRUE(enabled->type == TML_VALUE_BOOL);
  ASSERT_TRUE(enabled->boolean == 1);

  TMLEntry const* health = tml_node_find_entry(result.document, node, "health");
  ASSERT_TRUE(health != NULL);
  ASSERT_TRUE(health->type == TML_VALUE_I64);
  ASSERT_TRUE(health->integer == 100);

  tml_document_free(result.document);

  return 0;
}

int test_parse_nested_nodes(void)
{
  char const* source =
    "entity:\n"
    "  transform:\n"
    "    enabled: true\n";

  TMLParseResult result = tml_parse(source);
  ASSERT_TRUE(result.ok);
  ASSERT_TRUE(result.document->node_count == 2);

  TMLNode const* root = tml_root_node_at(result.document, 0);
  ASSERT_TRUE(root != NULL);
  ASSERT_TRUE(root->child_count == 1);

  TMLNode const* transform = tml_node_child_at(result.document, root, 0);
  ASSERT_TRUE(transform != NULL);
  ASSERT_TRUE(transform->name.size == 9);
  ASSERT_TRUE(memcmp(transform->name.data, "transform", 9) == 0);
  ASSERT_TRUE(transform->entry_count == 1);

  tml_document_free(result.document);

  return 0;
}

int test_parse_anonymous_nodes(void)
{
  char const* source =
    "scene:\n"
    "  objects:\n"
    "    - name: \"a\"\n"
    "    - name: \"b\"\n";

  TMLParseResult result= tml_parse(source);
  ASSERT_TRUE(result.ok);

  TMLNode const* objects = tml_node_find_child(result.document,
      tml_root_node_at(result.document, 0), "objects");
  ASSERT_TRUE(objects != NULL);
  ASSERT_TRUE(objects->child_count == 2);

  TMLNode const* child0 = tml_node_child_at(result.document, objects, 0);
  TMLNode const* child1 = tml_node_child_at(result.document, objects, 1);
  ASSERT_TRUE(child0 != NULL);
  ASSERT_TRUE(child1 != NULL);

  TMLEntry const* name0 = tml_node_find_entry(result.document, child0, "name");
  ASSERT_TRUE(child0->name.size == 0);
  ASSERT_TRUE(name0 != NULL);
  ASSERT_TRUE(name0->type == TML_VALUE_STRING);
  ASSERT_TRUE(strcmp(name0->string.data, "a") == 0);

  TMLEntry const* name1 = tml_node_find_entry(result.document, child1, "name");
  ASSERT_TRUE(child1->name.size == 0);
  ASSERT_TRUE(name1 != NULL);
  ASSERT_TRUE(name1->type == TML_VALUE_STRING);
  ASSERT_TRUE(strcmp(name1->string.data, "b") == 0);

  tml_document_free(result.document);
  return 0;
}

int test_parse_i64_array(void)
{
  char const* source =
    "data:\n"
    "  values: 1, 2,\n 3, 4\n";

  TMLParseResult result = tml_parse(source);
  ASSERT_TRUE(result.ok);

  TMLNode const* node = tml_root_node_at(result.document, 0);
  TMLEntry const* values = tml_node_find_entry(result.document, node, "values");

  ASSERT_TRUE(values != NULL);
  ASSERT_TRUE(values->type == TML_VALUE_ARRAY_I64);
  ASSERT_TRUE(values->array.count == 4);

  ASSERT_TRUE(result.document->array_i64[values->array.first + 0] == 1);
  ASSERT_TRUE(result.document->array_i64[values->array.first + 1] == 2);
  ASSERT_TRUE(result.document->array_i64[values->array.first + 2] == 3);
  ASSERT_TRUE(result.document->array_i64[values->array.first + 3] == 4);

  tml_document_free(result.document);

  return 0;
}

int test_parse_f64_array(void)
{
  char const*  source =
    "data:\n"
    "  values: 1.0, 2.5, 3.75\n";

  TMLParseResult result = tml_parse(source);

  ASSERT_TRUE(result.ok);

  TMLNode const* node = tml_root_node_at(result.document, 0);
  TMLEntry const* values = tml_node_find_entry(result.document, node, "values");
  ASSERT_TRUE(values != NULL);
  ASSERT_TRUE(values->type == TML_VALUE_ARRAY_F64);
  ASSERT_TRUE(values->array.count == 3);

  ASSERT_TRUE(result.document->array_f64[values->array.first + 0] == 1.0);
  ASSERT_TRUE(result.document->array_f64[values->array.first + 1] == 2.5);
  ASSERT_TRUE(result.document->array_f64[values->array.first + 2] == 3.75);

  tml_document_free(result.document);

  return 0;
}

int test_parse_multiline_string(void)
{
  char const* source =
    "meta:\n"
    "  text: \"hello\n"
    "world\"\n";

  TMLParseResult result = tml_parse(source);
  ASSERT_TRUE(result.ok);

  TMLNode const* node = tml_root_node_at(result.document, 0);
  TMLEntry const* text = tml_node_find_entry(result.document, node, "text");
  ASSERT_TRUE(text != NULL);
  ASSERT_TRUE(text->type == TML_VALUE_STRING);
  ASSERT_TRUE(strcmp(text->string.data, "hello\nworld") == 0);

  tml_document_free(result.document);

  return 0;
}

int test_parse_string_escape_sequences(void)
{
  char const* source =
    "meta:\n"
    "  text: \"hello\\nworld\\t!\\\"\"\n";

  TMLParseResult result = tml_parse(source);
  ASSERT_TRUE(result.ok);

  TMLNode const* node = tml_root_node_at(result.document, 0);
  TMLEntry const* text = tml_node_find_entry(result.document, node, "text");
  ASSERT_TRUE(text != NULL);
  ASSERT_TRUE(strcmp(text->string.data, "hello\nworld\t!\"") == 0);

  tml_document_free(result.document);

  return 0;
}

int test_parse_hex_integer(void)
{
  char const* source =
    "meta:\n"
    "  magic: 0xCAFEBABE\n";

  TMLParseResult result = tml_parse(source);
  ASSERT_TRUE(result.ok);

  TMLNode const* node = tml_root_node_at(result.document, 0);
  TMLEntry const* magic = tml_node_find_entry(result.document, node, "magic");
  ASSERT_TRUE(magic != NULL);
  ASSERT_TRUE(magic->type == TML_VALUE_I64);
  ASSERT_TRUE(magic->integer == 0xCAFEBABE);

  tml_document_free(result.document);

  return 0;
}

int test_reject_duplicate_names(void)
{
  char const* source =
    "root:\n"
    "  value: 1\n"
    "  value: 2\n";

  TMLParseResult result = tml_parse(source);
  ASSERT_TRUE(!result.ok);

  return 0;
}

int test_reject_tabs_for_indent(void)
{
  char const* source =
    "root:\n"
    "\tvalue: 1\n";

  TMLParseResult result = tml_parse(source);
  ASSERT_TRUE(!result.ok);

  return 0;
}

int test_reject_mixed_array_types(void)
{
  char const* source =
    "root:\n"
    "  values: 1, 2.0, 3\n";

  TMLParseResult result = tml_parse(source);
  ASSERT_TRUE(!result.ok);

  return 0;
}

int test_reject_top_level_entry(void)
{
  char const* source = "value: 10\n";
  TMLParseResult result = tml_parse(source);
  ASSERT_TRUE(!result.ok);

  return 0;
}

int test_child_indices_are_stable(void)
{
  char const* source =
    "root:\n"
    "  foo:\n"
    "  bar:\n"
    "  - enabled: true\n";

  TMLParseResult result = tml_parse(source);
  ASSERT_TRUE(result.ok);

  TMLNode const* root = tml_root_node_at(result.document, 0);
  ASSERT_TRUE(root->child_count == 3);
  TMLNode const* child0 = tml_node_child_at(result.document, root, 0);
  TMLNode const* child1 = tml_node_child_at(result.document, root, 1);
  TMLNode const* child2 = tml_node_child_at(result.document, root, 2);
  ASSERT_TRUE(child0 != NULL);
  ASSERT_TRUE(child1 != NULL);
  ASSERT_TRUE(child2 != NULL);
  ASSERT_TRUE(strcmp(child0->name.data, "foo") == 0);
  ASSERT_TRUE(strcmp(child1->name.data, "bar") == 0);
  ASSERT_TRUE(child2->name.size == 0);

  tml_document_free(result.document);

  return 0;
}

int main()
{
  STDXTestCase tests[] =
  {
    X_TEST(test_parse_empty_document),
    X_TEST(test_parse_single_node),
    X_TEST(test_parse_node_with_entries),
    X_TEST(test_parse_nested_nodes),
    X_TEST(test_parse_anonymous_nodes),
    X_TEST(test_parse_i64_array),
    X_TEST(test_parse_f64_array),
    X_TEST(test_parse_multiline_string),
    X_TEST(test_parse_string_escape_sequences),
    X_TEST(test_parse_hex_integer),
    X_TEST(test_reject_duplicate_names),
    X_TEST(test_reject_tabs_for_indent),
    X_TEST(test_reject_mixed_array_types),
    X_TEST(test_reject_top_level_entry),
    X_TEST(test_child_indices_are_stable),
  };

  return x_tests_run(tests, sizeof(tests)/sizeof(tests[0]), NULL);
}

