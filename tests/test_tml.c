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

int test_entry_get_scalar_api(void)
{
  char const* source =
    "config:\n"
    "  enabled: true\n"
    "  count: 42\n"
    "  ratio: 0.5\n"
    "  name: \"demo\"\n";

  TMLParseResult result = tml_parse(source);
  ASSERT_TRUE(result.ok);

  TMLNode const* node = tml_root_node_at(result.document, 0);
  ASSERT_TRUE(node != NULL);

  TMLEntry const* enabled_entry = tml_node_find_entry(result.document, node, "enabled");
  TMLEntry const* count_entry = tml_node_find_entry(result.document, node, "count");
  TMLEntry const* ratio_entry = tml_node_find_entry(result.document, node, "ratio");
  TMLEntry const* name_entry = tml_node_find_entry(result.document, node, "name");

  u8 enabled = 0;
  i64 count = 0;
  f64 ratio = 0.0;
  TMLString name = {0};

  ASSERT_TRUE(tml_entry_get_bool(enabled_entry, &enabled));
  ASSERT_TRUE(tml_entry_get_i64(count_entry, &count));
  ASSERT_TRUE(tml_entry_get_f64(ratio_entry, &ratio));
  ASSERT_TRUE(tml_entry_get_string(name_entry, &name));

  ASSERT_TRUE(enabled == 1);
  ASSERT_TRUE(count == 42);
  ASSERT_TRUE(ratio == 0.5);
  ASSERT_TRUE(name.size == 4);
  ASSERT_TRUE(memcmp(name.data, "demo", 4) == 0);
  ASSERT_TRUE(name.data[4] == '\0');

  ASSERT_TRUE(!tml_entry_get_i64(enabled_entry, &count));
  ASSERT_TRUE(!tml_entry_get_bool(count_entry, &enabled));

  tml_document_free(result.document);

  return 0;
}

int test_node_get_scalar_api(void)
{
  char const* source =
    "player:\n"
    "  alive: true\n"
    "  hp: 100\n"
    "  speed: 3.5\n"
    "  title: \"captain\"\n";

  TMLParseResult result = tml_parse(source);
  ASSERT_TRUE(result.ok);

  TMLNode const* player = tml_root_node_at(result.document, 0);
  ASSERT_TRUE(player != NULL);

  u8 alive = 0;
  i64 hp = 0;
  f64 speed = 0.0;
  TMLString title = {0};

  ASSERT_TRUE(tml_node_get_bool(result.document, player, "alive", &alive));
  ASSERT_TRUE(tml_node_get_i64(result.document, player, "hp", &hp));
  ASSERT_TRUE(tml_node_get_f64(result.document, player, "speed", &speed));
  ASSERT_TRUE(tml_node_get_string(result.document, player, "title", &title));

  ASSERT_TRUE(alive == 1);
  ASSERT_TRUE(hp == 100);
  ASSERT_TRUE(speed == 3.5);
  ASSERT_TRUE(title.size == 7);
  ASSERT_TRUE(memcmp(title.data, "captain", 7) == 0);

  ASSERT_TRUE(!tml_node_get_i64(result.document, player, "missing", &hp));
  ASSERT_TRUE(!tml_node_get_bool(result.document, player, "hp", &alive));

  tml_document_free(result.document);

  return 0;
}

int test_entry_get_array_api(void)
{
  char const* source =
    "data:\n"
    "  integers: 1, 2, 3\n"
    "  floats: 1.0, 2.0, 3.5\n"
    "  strings: \"a\", \"b\", \"c\"\n";

  TMLParseResult result = tml_parse(source);
  ASSERT_TRUE(result.ok);

  TMLNode const* data = tml_root_node_at(result.document, 0);
  ASSERT_TRUE(data != NULL);

  TMLEntry const* integers_entry = tml_node_find_entry(result.document, data, "integers");
  TMLEntry const* floats_entry = tml_node_find_entry(result.document, data, "floats");
  TMLEntry const* strings_entry = tml_node_find_entry(result.document, data, "strings");

  TMLI64Slice integers = {0};
  TMLF64Slice floats = {0};
  TMLStringSlice strings = {0};

  ASSERT_TRUE(tml_entry_get_i64_array(result.document, integers_entry, &integers));
  ASSERT_TRUE(tml_entry_get_f64_array(result.document, floats_entry, &floats));
  ASSERT_TRUE(tml_entry_get_string_array(result.document, strings_entry, &strings));

  ASSERT_TRUE(integers.count == 3);
  ASSERT_TRUE(integers.data[0] == 1);
  ASSERT_TRUE(integers.data[1] == 2);
  ASSERT_TRUE(integers.data[2] == 3);

  ASSERT_TRUE(floats.count == 3);
  ASSERT_TRUE(floats.data[0] == 1.0);
  ASSERT_TRUE(floats.data[1] == 2.0);
  ASSERT_TRUE(floats.data[2] == 3.5);

  ASSERT_TRUE(strings.count == 3);
  ASSERT_TRUE(strings.data[0].size == 1);
  ASSERT_TRUE(strings.data[1].size == 1);
  ASSERT_TRUE(strings.data[2].size == 1);
  ASSERT_TRUE(memcmp(strings.data[0].data, "a", 1) == 0);
  ASSERT_TRUE(memcmp(strings.data[1].data, "b", 1) == 0);
  ASSERT_TRUE(memcmp(strings.data[2].data, "c", 1) == 0);

  ASSERT_TRUE(!tml_entry_get_f64_array(result.document, integers_entry, &floats));

  tml_document_free(result.document);

  return 0;
}

int test_node_get_array_api(void)
{
  char const* source =
    "mesh:\n"
    "  indices: 0, 1, 2, 2, 3, 0\n"
    "  vertices: 0.0, 1.0, 2.0,\n"
    "    3.0, 4.0, 5.0\n"
    "  names: \"p0\", \"p1\"\n";

  TMLParseResult result = tml_parse(source);
  ASSERT_TRUE(result.ok);

  TMLNode const* mesh = tml_root_node_at(result.document, 0);
  ASSERT_TRUE(mesh != NULL);

  TMLI64Slice indices = {0};
  TMLF64Slice vertices = {0};
  TMLStringSlice names = {0};

  ASSERT_TRUE(tml_node_get_i64_array(result.document, mesh, "indices", &indices));
  ASSERT_TRUE(tml_node_get_f64_array(result.document, mesh, "vertices", &vertices));
  ASSERT_TRUE(tml_node_get_string_array(result.document, mesh, "names", &names));

  ASSERT_TRUE(indices.count == 6);
  ASSERT_TRUE(indices.data[0] == 0);
  ASSERT_TRUE(indices.data[5] == 0);

  ASSERT_TRUE(vertices.count == 6);
  ASSERT_TRUE(vertices.data[0] == 0.0);
  ASSERT_TRUE(vertices.data[5] == 5.0);

  ASSERT_TRUE(names.count == 2);
  ASSERT_TRUE(names.data[0].size == 2);
  ASSERT_TRUE(names.data[1].size == 2);
  ASSERT_TRUE(memcmp(names.data[0].data, "p0", 2) == 0);
  ASSERT_TRUE(memcmp(names.data[1].data, "p1", 2) == 0);

  ASSERT_TRUE(!tml_node_get_i64_array(result.document, mesh, "vertices", &indices));

  tml_document_free(result.document);

  return 0;
}

int test_path_find_node_by_name_and_index(void)
{
  char const* source =
    "scene:\n"
    "  objects:\n"
    "    - name: \"first\"\n"
    "    - name: \"second\"\n"
    "  camera:\n"
    "    fov: 70.0\n";

  TMLParseResult result = tml_parse(source);
  ASSERT_TRUE(result.ok);

  TMLNode const* scene_by_name = tml_path_find_node(result.document, "scene");
  TMLNode const* scene_by_index = tml_path_find_node(result.document, "0");
  TMLNode const* objects_by_name = tml_path_find_node(result.document, "scene.objects");
  TMLNode const* objects_by_index = tml_path_find_node(result.document, "0.0");
  TMLNode const* object1_by_index = tml_path_find_node(result.document, "scene.objects.1");
  TMLNode const* camera_by_name = tml_path_find_node(result.document, "scene.camera");
  TMLNode const* camera_by_index = tml_path_find_node(result.document, "scene.1");

  ASSERT_TRUE(scene_by_name != NULL);
  ASSERT_TRUE(scene_by_index != NULL);
  ASSERT_TRUE(scene_by_name == scene_by_index);

  ASSERT_TRUE(objects_by_name != NULL);
  ASSERT_TRUE(objects_by_index != NULL);
  ASSERT_TRUE(objects_by_name == objects_by_index);

  ASSERT_TRUE(object1_by_index != NULL);
  ASSERT_TRUE(object1_by_index->name.size == 0);

  ASSERT_TRUE(camera_by_name != NULL);
  ASSERT_TRUE(camera_by_index != NULL);
  ASSERT_TRUE(camera_by_name == camera_by_index);

  ASSERT_TRUE(tml_path_find_node(result.document, "scene.missing") == NULL);
  ASSERT_TRUE(tml_path_find_node(result.document, "scene.objects.3") == NULL);

  tml_document_free(result.document);

  return 0;
}

int test_path_find_entry_by_name_and_index(void)
{
  char const* source =
    "scene:\n"
    "  meta:\n"
    "    type: \"scene\"\n"
    "    magic: 0xCAFEBABE\n"
    "    visible: true\n";

  TMLParseResult result = tml_parse(source);
  ASSERT_TRUE(result.ok);

  TMLEntry const* type_by_name = tml_path_find_entry(result.document, "scene.meta.type");
  TMLEntry const* type_by_index = tml_path_find_entry(result.document, "scene.meta.0");
  TMLEntry const* magic_by_name = tml_path_find_entry(result.document, "scene.meta.magic");
  TMLEntry const* magic_by_index = tml_path_find_entry(result.document, "scene.meta.1");
  TMLEntry const* visible_by_name = tml_path_find_entry(result.document, "scene.meta.visible");
  TMLEntry const* visible_by_index = tml_path_find_entry(result.document, "scene.meta.2");

  ASSERT_TRUE(type_by_name != NULL);
  ASSERT_TRUE(type_by_index != NULL);
  ASSERT_TRUE(type_by_name == type_by_index);

  ASSERT_TRUE(magic_by_name != NULL);
  ASSERT_TRUE(magic_by_index != NULL);
  ASSERT_TRUE(magic_by_name == magic_by_index);

  ASSERT_TRUE(visible_by_name != NULL);
  ASSERT_TRUE(visible_by_index != NULL);
  ASSERT_TRUE(visible_by_name == visible_by_index);

  ASSERT_TRUE(tml_path_find_entry(result.document, "scene.meta.3") == NULL);
  ASSERT_TRUE(tml_path_find_entry(result.document, "scene.meta.missing") == NULL);
  ASSERT_TRUE(tml_path_find_entry(result.document, "scene.meta.magic.extra") == NULL);

  tml_document_free(result.document);

  return 0;
}

int test_path_get_scalar_api(void)
{
  char const* source =
    "scene:\n"
    "  meta:\n"
    "    enabled: true\n"
    "    magic: 0xCAFEBABE\n"
    "    ratio: 1.25\n"
    "    title: \"Island\"\n";

  TMLParseResult result = tml_parse(source);
  ASSERT_TRUE(result.ok);

  u8 enabled = 0;
  i64 magic = 0;
  f64 ratio = 0.0;
  TMLString title = {0};

  ASSERT_TRUE(tml_path_get_bool(result.document, "scene.meta.enabled", &enabled));
  ASSERT_TRUE(tml_path_get_i64(result.document, "scene.meta.magic", &magic));
  ASSERT_TRUE(tml_path_get_f64(result.document, "scene.meta.ratio", &ratio));
  ASSERT_TRUE(tml_path_get_string(result.document, "scene.meta.title", &title));

  ASSERT_TRUE(enabled == 1);
  ASSERT_TRUE(magic == 0xCAFEBABE);
  ASSERT_TRUE(ratio == 1.25);
  ASSERT_TRUE(title.size == 6);
  ASSERT_TRUE(memcmp(title.data, "Island", 6) == 0);

  ASSERT_TRUE(tml_path_get_i64(result.document, "0.0.1", &magic));
  ASSERT_TRUE(magic == 0xCAFEBABE);

  ASSERT_TRUE(!tml_path_get_i64(result.document, "scene.meta.title", &magic));
  ASSERT_TRUE(!tml_path_get_bool(result.document, "scene.meta.missing", &enabled));

  tml_document_free(result.document);

  return 0;
}

int test_path_get_array_api(void)
{
  char const* source =
    "scene:\n"
    "  mesh:\n"
    "    indices: 0, 1, 2\n"
    "    vertices: 1.0, 2.0, 3.0\n"
    "    names: \"a\", \"b\"\n";

  TMLParseResult result = tml_parse(source);
  ASSERT_TRUE(result.ok);

  TMLI64Slice indices = {0};
  TMLF64Slice vertices = {0};
  TMLStringSlice names = {0};

  ASSERT_TRUE(tml_path_get_i64_array(result.document, "scene.mesh.indices", &indices));
  ASSERT_TRUE(tml_path_get_f64_array(result.document, "scene.mesh.vertices", &vertices));
  ASSERT_TRUE(tml_path_get_string_array(result.document, "scene.mesh.names", &names));

  ASSERT_TRUE(indices.count == 3);
  ASSERT_TRUE(indices.data[0] == 0);
  ASSERT_TRUE(indices.data[1] == 1);
  ASSERT_TRUE(indices.data[2] == 2);

  ASSERT_TRUE(vertices.count == 3);
  ASSERT_TRUE(vertices.data[0] == 1.0);
  ASSERT_TRUE(vertices.data[1] == 2.0);
  ASSERT_TRUE(vertices.data[2] == 3.0);

  ASSERT_TRUE(names.count == 2);
  ASSERT_TRUE(names.data[0].size == 1);
  ASSERT_TRUE(names.data[1].size == 1);
  ASSERT_TRUE(memcmp(names.data[0].data, "a", 1) == 0);
  ASSERT_TRUE(memcmp(names.data[1].data, "b", 1) == 0);

  ASSERT_TRUE(!tml_path_get_f64_array(result.document, "scene.mesh.indices", &vertices));

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
    X_TEST(test_entry_get_scalar_api),
    X_TEST(test_node_get_scalar_api),
    X_TEST(test_entry_get_array_api),
    X_TEST(test_node_get_array_api),
    X_TEST(test_path_find_node_by_name_and_index),
    X_TEST(test_path_find_entry_by_name_and_index),
    X_TEST(test_path_get_scalar_api),
    X_TEST(test_path_get_array_api),
  };

  return x_tests_run(tests, sizeof(tests)/sizeof(tests[0]), NULL);
}

