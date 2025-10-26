#define X_IMPL_TEST
#include <stdx_test.h>
#define X_IMPL_TML
#include <stdx_tml.h>

#include <stdio.h>
#include <string.h>
#include <stdint.h>

static const char *g_tml_src =
"level:\n"
"  tutorial:\n"
"    enabled: true\n"
"    seed: 934784\n"
"    description: \"\"\"\n"
"    Multi line strings are\n"
"    also supported!\n"
"\"\"\"\n"
"    objects:\n"
"      - position: 1.0, 2.0, 0.0\n"
"        scale: 1.0, 1.0, 1.0\n"
"        weights: 10, 20, 40, 103,\n"
"                 99, 71, 44, -1,\n"
"                 18, 5, 7, 91\n"
"      - position: 7.0, 0.0, 0.0\n"
"        scale: 1.0, 1.0, 1.0\n"
"        weights: 10, 20, 40, 103,\n"
"                 18, 3, 0, -91\n"
"  green_hills:\n"
"    enabled: true\n"
"    seed: 934784\n"
"    description: \"\"\"\n"
"    Multi line strings are\n"
"    also supported!\n"
"\"\"\"\n"
"    objects:\n"
"      - position: 1.0, 2.0, 0.0\n"
"        scale: 1.0, 1.0, 1.0\n"
"        weights: 10, 20, 40, 103,\n"
"                 99, 71, 44, -1,\n"
"                 18, 5, 7, 91\n"
"      - position: 7.0, 0.0, 0.0\n"
"        scale: 1.0, 1.0, 1.0\n"
"        weights: 10, 20, 40, 103,\n"
"                 18, 3, 0, -91\n";

/* Open and root children */
int test_tml_open_and_root_children(void)
{
  XTml *doc = NULL;
  int ok = x_tml_load(g_tml_src, (uint32_t)strlen(g_tml_src), 0, &doc);
  ASSERT_TRUE(ok == 1);

#if 0
  x_tml_dump(doc, NULL);
#endif

  XTmlCursor root = x_tml_root(doc);
  uint32_t root_children = 0;
  ok = x_tml_child_count(doc, root, &root_children);
  ASSERT_TRUE(ok == 1);
  ASSERT_TRUE(root_children == 1);

  XTmlCursor level;
  ok = x_tml_get_section(doc, root, "level", &level);
  ASSERT_TRUE(ok == 1);

  uint32_t level_children = 0;
  ok = x_tml_child_count(doc, level, &level_children);
  ASSERT_TRUE(ok == 1);
  ASSERT_TRUE(level_children == 2);

  x_tml_unload(doc);
  return 0;
}

/* Get section by dot path */
int test_tml_get_section_by_path(void)
{
  XTml *doc = NULL;
  int ok = x_tml_load(g_tml_src, (uint32_t)strlen(g_tml_src), 0, &doc);
  ASSERT_TRUE(ok == 1);

  XTmlCursor root = x_tml_root(doc);

  XTmlCursor tut;
  ok = x_tml_get_section(doc, root, "level.tutorial", &tut);
  ASSERT_TRUE(ok == 1);

  XTmlCursor gh;
  ok = x_tml_get_section(doc, root, "level.green_hills", &gh);
  ASSERT_TRUE(ok == 1);

  x_tml_unload(doc);
  return 0;
}

/* Discovery iteration */
int test_tml_discovery_iteration(void)
{
  XTml *doc = NULL;
  int ok = x_tml_load(g_tml_src, (uint32_t)strlen(g_tml_src), 0, &doc);
  ASSERT_TRUE(ok == 1);

  XTmlCursor root = x_tml_root(doc);

  XTmlCursor level;
  ok = x_tml_get_section(doc, root, "level", &level);
  ASSERT_TRUE(ok == 1);

  uint32_t n_levels = 0;
  ok = x_tml_child_count(doc, level, &n_levels);
  ASSERT_TRUE(ok == 1);
  ASSERT_TRUE(n_levels == 2);

  XTmlCursor first_level;
  ok = x_tml_child_at(doc, level, 0, &first_level);
  ASSERT_TRUE(ok == 1);

  XTmlCursor objs;
  ok = x_tml_get_section(doc, first_level, "objects", &objs);
  ASSERT_TRUE(ok == 1);

  uint32_t n_objs = 0;
  ok = x_tml_child_count(doc, objs, &n_objs);
  ASSERT_TRUE(ok == 1);
  ASSERT_TRUE(n_objs == 2);

  XTmlCursor obj0;
  ok = x_tml_child_at(doc, objs, 0, &obj0);
  ASSERT_TRUE(ok == 1);

  XTmlCursor obj1;
  ok = x_tml_child_at(doc, objs, 1, &obj1);
  ASSERT_TRUE(ok == 1);

  x_tml_unload(doc);
  return 0;
}

/* Scalar entries, arrays and value types */
int test_tml_scalar_entries_and_types(void)
{
  XTml *doc = NULL;
  int ok = x_tml_load(g_tml_src, (uint32_t)strlen(g_tml_src), 0, &doc);
  ASSERT_TRUE(ok == 1);

  XTmlCursor root = x_tml_root(doc);

  XTmlCursor tut;
  ok = x_tml_get_section(doc, root, "level.tutorial", &tut);
  ASSERT_TRUE(ok == 1);

  int enabled = 0;
  ok = x_tml_get_bool(doc, tut, "enabled", &enabled);
  ASSERT_TRUE(ok == 1);
  ASSERT_TRUE(enabled == 1);

  int64_t seed = 0;
  ok = x_tml_get_i64(doc, tut, "seed", &seed);
  ASSERT_TRUE(ok == 1);
  ASSERT_TRUE(seed == 934784);

  const char *desc = NULL;
  uint32_t desc_len = 0;
  ok = x_tml_get_str(doc, tut, "description", &desc, &desc_len);
  ASSERT_TRUE(ok == 1);
  ASSERT_TRUE(desc_len > 0);

  /* Verifica que contém o início do texto multilinha */
  int found = 0;

  for (uint32_t i = 0; i + 22 <= desc_len; i++)
  {
    if (memcmp(desc + i, "Multi line strings are", 22) == 0)
    {
      found = 1;
      break;
    }
  }
  ASSERT_TRUE(found == 1);

  XTmlCursor objs;
  ok = x_tml_get_section(doc, tut, "objects", &objs);
  ASSERT_TRUE(ok == 1);

  XTmlCursor obj0;
  ok = x_tml_child_at(doc, objs, 0, &obj0);
  ASSERT_TRUE(ok == 1);

  const double *pos = NULL;
  uint32_t pos_n = 0;
  ok = x_tml_get_array_f64(doc, obj0, "position", &pos, &pos_n);
  ASSERT_TRUE(ok == 1);
  ASSERT_TRUE(pos_n == 3);
  ASSERT_TRUE(pos[0] == 1.0);
  ASSERT_TRUE(pos[1] == 2.0);
  ASSERT_TRUE(pos[2] == 0.0);

  const double *scale = NULL;
  uint32_t scale_n = 0;
  ok = x_tml_get_array_f64(doc, obj0, "scale", &scale, &scale_n);
  ASSERT_TRUE(ok == 1);
  ASSERT_TRUE(scale_n == 3);

  const int64_t *weights = NULL;
  uint32_t w_n = 0;
  ok = x_tml_get_array_i64(doc, obj0, "weights", &weights, &w_n);
  ASSERT_TRUE(ok == 1);
  ASSERT_TRUE(w_n == 12);
  ASSERT_TRUE(weights[0] == 10);
  ASSERT_TRUE(weights[3] == 103);
  ASSERT_TRUE(weights[7] == -1);

  x_tml_unload(doc);
  return 0;
}

int test_tml_arrays_numeric_single_and_multiline(void)
{
  XTml *doc = NULL;
  int ok = x_tml_load(g_tml_src, (uint32_t)strlen(g_tml_src), 0, &doc);
  ASSERT_TRUE(ok == 1);

  XTmlCursor root = x_tml_root(doc);

  XTmlCursor gh;
  ok = x_tml_get_section(doc, root, "level.green_hills", &gh);
  ASSERT_TRUE(ok == 1);

  XTmlCursor objs;
  ok = x_tml_get_section(doc, gh, "objects", &objs);
  ASSERT_TRUE(ok == 1);

  XTmlCursor obj1;
  ok = x_tml_child_at(doc, objs, 1, &obj1);
  ASSERT_TRUE(ok == 1);

  const int64_t *weights = NULL;
  uint32_t w_n = 0;
  ok = x_tml_get_array_i64(doc, obj1, "weights", &weights, &w_n);
  ASSERT_TRUE(ok == 1);
  ASSERT_TRUE(w_n == 8);
  ASSERT_TRUE(weights[0] == 10);
  ASSERT_TRUE(weights[3] == 103);
  ASSERT_TRUE(weights[7] == -91);

  x_tml_unload(doc);
  return 0;
}

/* Missing paths and bad lookups */
int test_tml_missing_and_error_paths(void)
{
  XTml *doc = NULL;
  int ok = x_tml_load(g_tml_src, (uint32_t)strlen(g_tml_src), 0, &doc);
  ASSERT_TRUE(ok == 1);

  XTmlCursor root = x_tml_root(doc);

  XTmlCursor missing;
  ok = x_tml_get_section(doc, root, "level.not_here", &missing);
  ASSERT_TRUE(ok == 0);

  XTmlCursor tut;
  ok = x_tml_get_section(doc, root, "level.tutorial", &tut);
  ASSERT_TRUE(ok == 1);

  int has = x_tml_has_key(doc, tut, "nope");
  ASSERT_TRUE(has == 0);

  int b = 0;
  ok = x_tml_get_bool(doc, tut, "seed", &b);
  ASSERT_TRUE(ok == 0);

  x_tml_unload(doc);
  return 0;
}

/* Iteration over objects (list semantics) */
int test_tml_iteration_over_objects(void)
{
  XTml *doc = NULL;
  int ok = x_tml_load(g_tml_src, (uint32_t)strlen(g_tml_src), 0, &doc);
  ASSERT_TRUE(ok == 1);

  XTmlCursor root = x_tml_root(doc);

  XTmlCursor tut;
  ok = x_tml_get_section(doc, root, "level.tutorial", &tut);
  ASSERT_TRUE(ok == 1);

  XTmlCursor objs;
  ok = x_tml_get_section(doc, tut, "objects", &objs);
  ASSERT_TRUE(ok == 1);

  uint32_t n = 0;
  ok = x_tml_child_count(doc, objs, &n);
  ASSERT_TRUE(ok == 1);
  ASSERT_TRUE(n == 2);

  XTmlCursor c0;
  ok = x_tml_child_at(doc, objs, 0, &c0);
  ASSERT_TRUE(ok == 1);

  XTmlCursor c1;
  ok = x_tml_child_at(doc, objs, 1, &c1);
  ASSERT_TRUE(ok == 1);

  x_tml_unload(doc);
  return 0;
}

/* BTML round-trip: encode, load, navigate */
int test_btml_roundtrip_and_nav(void)
{
  XTml *doc = NULL;
  int ok = x_tml_load(g_tml_src, (uint32_t)strlen(g_tml_src), 0, &doc);
  ASSERT_TRUE(ok == 1);

  const char *tmp_path = "tmp_scene.btml";

  ok = x_btml_encode_to_file(doc, tmp_path);
  ASSERT_TRUE(ok == 1);

  XBtml bin;
  memset(&bin, 0, sizeof(bin));

  ok = x_btml_load_from_file(tmp_path, &bin);
  ASSERT_TRUE(ok == 1);

  int node_level = -1;
  ok = x_btml_get_section_by_dotpath(&bin, -1, "level", &node_level);
  ASSERT_TRUE(ok == 1);
  ASSERT_TRUE(node_level >= 0);

  uint32_t n_lv = 0;
  ok = x_btml_child_count(&bin, node_level, &n_lv);
  ASSERT_TRUE(ok == 1);
  ASSERT_TRUE(n_lv == 2);

  int node_tut = -1;
  ok = x_btml_get_section_by_dotpath(&bin, node_level, "tutorial", &node_tut);
  ASSERT_TRUE(ok == 1);

  int node_objs = -1;
  ok = x_btml_get_section_by_dotpath(&bin, node_tut, "objects", &node_objs);
  ASSERT_TRUE(ok == 1);

  uint32_t n_objs = 0;
  ok = x_btml_child_count(&bin, node_objs, &n_objs);
  ASSERT_TRUE(ok == 1);
  ASSERT_TRUE(n_objs == 2);

  int node_obj1 = -1;
  ok = x_btml_child_at(&bin, node_objs, 1, &node_obj1);
  ASSERT_TRUE(ok == 1);
  ASSERT_TRUE(node_obj1 >= 0);

  x_btml_unload(&bin);
  x_tml_unload(doc);
  return 0;
}

int main()
{
  STDXTestCase tests[] =
  {
    X_TEST(test_tml_get_section_by_path),
    X_TEST(test_tml_open_and_root_children),
    X_TEST(test_tml_get_section_by_path),
    X_TEST(test_tml_discovery_iteration),
    X_TEST(test_tml_scalar_entries_and_types),
    X_TEST(test_tml_arrays_numeric_single_and_multiline),
    X_TEST(test_tml_missing_and_error_paths),
    X_TEST(test_tml_iteration_over_objects),
    X_TEST(test_btml_roundtrip_and_nav)

  };

  return stdx_run_tests(tests, sizeof(tests)/sizeof(tests[0]));
}

