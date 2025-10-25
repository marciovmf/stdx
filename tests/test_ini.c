
/* test_ini.c */
#include <stdx_common.h>
#define X_IMPL_TEST
#include <stdx_test.h>
#define X_IMPL_IO
#include <stdx_io.h>
#define X_IMPL_INI
#include <stdx_ini.h>

#include <stdint.h>
#include <string.h>

/* ---------- Utilidades de apoio aos testes ---------- */

/* C99 substring search: retorna 1 se needle aparece em haystack (com tamanhos explícitos). */
static int s_x_contains(const char *haystack, uint32_t hlen,
                        const char *needle,   uint32_t nlen)
{
  if (needle == NULL || nlen == 0u)
  {
    return 1;
  }
  if (haystack == NULL || hlen < nlen)
  {
    return 0;
  }
  for (uint32_t i = 0u; i + nlen <= hlen; i = i + 1u)
  {
    if (memcmp(haystack + i, needle, nlen) == 0)
    {
      return 1;
    }
  }
  return 0;
}

static const char *s_x_sample_ini(void)
{
  return
    "level/tutorial:\n"
    "  enabled     = true\n"
    "  seed        = 934784\n"
    "  description = \"\"\"\n"
    "    Multi line strings are\n"
    "    also supported!\n"
    "    \"\"\"\n"
    "\n"
    "level/tutorial/objects/sphere:\n"
    "  position    = 1.0, 2.0, 0.0\n"
    "  scale       = 1.0, 1.0, 1.0\n"
    "  weights     = 10, 20, 40, 103,\n"
    "                99, 71, 44, -1,\n"
    "                18,  5,  7, 91\n"
    "\n"
    "level/tutorial/objects/cylinder:\n"
    "  position    = 7.0, 0.0, 0.0\n"
    "  scale       = 1.0, 1.0, 1.0\n"
    "  weights     = 10, 20, 40, 103,\n"
    "                18,  3,  0, -91\n"
    "\n"
    "level/green_hills:\n"
    "  enabled     = true\n"
    "  seed        = 934784\n"
    "  description = \"\"\"\n"
    "    Multi line strings are\n"
    "    also supported!\n"
    "    \"\"\"\n"
    "\n"
    "level/green_hills/objects/teapot:\n"
    "  position    = 1.0, 2.0, 0.0\n"
    "  scale       = 1.0, 1.0, 1.0\n"
    "  weights     = 10, 20, 40, 103,\n"
    "                99, 71, 44, -1,\n"
    "                18,  5,  7, 91\n"
    "\n"
    "level/green_hills/objects/tree:\n"
    "  position    = 7.0, 0.0, 0.0\n"
    "  scale       = 1.0, 1.0, 1.0\n"
    "  weights     = 10, 20, 40, 103,\n"
    "                18,  3,  0, -91\n";
}

static int s_x_open_sample(XIni **out_ini)
{
  const char *txt = s_x_sample_ini();
  uint32_t len = (uint32_t)strlen(txt);
  int ok = x_ini_open_from_memory(txt, len, XINI_OPEN_DEFAULT, out_ini);
  return ok != 0 ? 1 : 0;
}

/* ---------- Testes ---------- */

int test_ini_open_and_root_children(void)
{
  XIni *ini = NULL;
  int ok = s_x_open_sample(&ini);
  ASSERT_TRUE(ok == 1);

  XIniCursor root = x_ini_root(ini);

  uint32_t n = 0u;
  ok = x_ini_child_count(ini, root, &n);
  ASSERT_TRUE(ok == 1);
  ASSERT_TRUE(n > 0u);

  /* We expect "level" to be at root level. */
  XIniCursor child = { -1 };
  int found = x_ini_find_child(ini, root, "level", 5u, &child);
  ASSERT_TRUE(found == 1);
  ASSERT_TRUE(child.node >= 0);

  x_ini_close(ini);
  return 0;
}

int test_ini_get_section_by_path(void)
{
  XIni *ini = NULL;
  int ok = s_x_open_sample(&ini);
  ASSERT_TRUE(ok == 1);

  XIniCursor cur = { -1 };
  ok = x_ini_get_section(ini, x_ini_root(ini), "level/tutorial", &cur);
  ASSERT_TRUE(ok == 1);
  ASSERT_TRUE(cur.node >= 0);

  XIniCursor obj = { -1 };
  ok = x_ini_get_section(ini, x_ini_root(ini), "level/tutorial/objects/sphere", &obj);
  ASSERT_TRUE(ok == 1);
  ASSERT_TRUE(obj.node >= 0);

  x_ini_close(ini);
  return 0;
}

int test_ini_discovery_iteration(void)
{
  XIni *ini = NULL;
  int ok = s_x_open_sample(&ini);
  ASSERT_TRUE(ok == 1);

  XIniCursor level = { -1 };
  ok = x_ini_get_section(ini, x_ini_root(ini), "level", &level);
  ASSERT_TRUE(ok == 1);

  /* Filhos de level: "tutorial", "green_hills" */
  uint32_t count = 0u;
  ok = x_ini_child_count(ini, level, &count);
  ASSERT_TRUE(ok == 1);
  ASSERT_TRUE(count == 2u);

  XIniCursor a = { -1 };
  XIniCursor b = { -1 };

  ok = x_ini_child_at(ini, level, 0u, &a);
  ASSERT_TRUE(ok == 1);
  ok = x_ini_child_at(ini, level, 1u, &b);
  ASSERT_TRUE(ok == 1);

  const char *name_a = NULL;
  uint32_t len_a = 0u;
  ok = x_ini_section_name(ini, a, &name_a, &len_a);
  ASSERT_TRUE(ok == 1);

  const char *name_b = NULL;
  uint32_t len_b = 0u;
  ok = x_ini_section_name(ini, b, &name_b, &len_b);
  ASSERT_TRUE(ok == 1);

  ASSERT_TRUE(len_a > 0u);
  ASSERT_TRUE(len_b > 0u);

  x_ini_close(ini);
  return 0;
}

int test_ini_scalar_entries_and_types(void)
{
  XIni *ini = NULL;
  int ok = s_x_open_sample(&ini);
  ASSERT_TRUE(ok == 1);

  XIniCursor tut = { -1 };
  ok = x_ini_get_section(ini, x_ini_root(ini), "level/tutorial", &tut);
  ASSERT_TRUE(ok == 1);

  int enabled = 0;
  ok = x_ini_get_bool(ini, tut, "enabled", &enabled);
  ASSERT_TRUE(ok == 1);
  ASSERT_TRUE(enabled == 1);

  int64_t seed_i = 0;
  ok = x_ini_get_i64(ini, tut, "seed", &seed_i);
  ASSERT_TRUE(ok == 1);
  ASSERT_TRUE(seed_i == 934784);

  const char *desc = NULL;
  uint32_t desc_len = 0u;
  ok = x_ini_get_str(ini, tut, "description", &desc, &desc_len);
  ASSERT_TRUE(ok == 1);
  ASSERT_TRUE(desc_len > 0u);
  ASSERT_TRUE(s_x_contains(desc, desc_len, "Multi line strings are", 19) == 1);

  x_ini_close(ini);
  return 0;
}

int test_ini_arrays_numeric_single_and_multiline(void)
{
  XIni *ini = NULL;
  int ok = s_x_open_sample(&ini);
  ASSERT_TRUE(ok == 1);

  XIniCursor sphere = { -1 };
  ok = x_ini_get_section(ini, x_ini_root(ini), "level/tutorial/objects/sphere", &sphere);
  ASSERT_TRUE(ok == 1);

  /* position = 1.0, 2.0, 0.0 */
  const double *pos = NULL;
  uint32_t npos = 0u;
  ok = x_ini_get_array_f64(ini, sphere, "position", &pos, &npos);
  ASSERT_TRUE(ok == 1);
  ASSERT_TRUE(npos == 3u);
  ASSERT_TRUE(pos[0] == 1.0);
  ASSERT_TRUE(pos[1] == 2.0);
  ASSERT_TRUE(pos[2] == 0.0);

  /* weights multiline, total 12 elementos */
  uint32_t wlen = 0u;
  ok = x_ini_get_array_len(ini, sphere, "weights", &wlen);
  ASSERT_TRUE(ok == 1);
  ASSERT_TRUE(wlen == 12u);

  /* Os inteiros devem ser acessíveis por i64 */
  const int64_t *wv = NULL;
  uint32_t nw = 0u;
  ok = x_ini_get_array_i64(ini, sphere, "weights", &wv, &nw);
  ASSERT_TRUE(ok == 1);
  ASSERT_TRUE(nw == 12u);
  ASSERT_TRUE(wv[0] == 10);
  ASSERT_TRUE(wv[3] == 103);
  ASSERT_TRUE(wv[7] == -1);

  x_ini_close(ini);
  return 0;
}

int test_ini_missing_and_error_paths(void)
{
  XIni *ini = NULL;
  int ok = s_x_open_sample(&ini);
  ASSERT_TRUE(ok == 1);

  XIniCursor cur = { -1 };
  ok = x_ini_get_section(ini, x_ini_root(ini), "does/not/exist", &cur);
  ASSERT_TRUE(ok == 0);

  XIniCursor tut = { -1 };
  ok = x_ini_get_section(ini, x_ini_root(ini), "level/tutorial", &tut);
  ASSERT_TRUE(ok == 1);

  int has = x_ini_has_key(ini, tut, "nope");
  ASSERT_TRUE(has == 0);

  double f = 0.0;
  ok = x_ini_get_f64(ini, tut, "enabled", &f);
  ASSERT_TRUE(ok == 0);

  x_ini_close(ini);
  return 0;
}

int test_ini_iteration_over_objects(void)
{
  XIni *ini = NULL;
  int ok = s_x_open_sample(&ini);
  ASSERT_TRUE(ok == 1);

  XIniCursor objects = { -1 };
  ok = x_ini_get_section(ini, x_ini_root(ini), "level/green_hills/objects", &objects);
  ASSERT_TRUE(ok == 1);

  uint32_t n = 0u;
  ok = x_ini_child_count(ini, objects, &n);
  ASSERT_TRUE(ok == 1);
  ASSERT_TRUE(n == 2u);

  for (uint32_t i = 0u; i < n; i = i + 1u)
  {
    XIniCursor ent = { -1 };
    ok = x_ini_child_at(ini, objects, i, &ent);
    ASSERT_TRUE(ok == 1);

    const char *nm = NULL;
    uint32_t nl = 0u;
    ok = x_ini_section_name(ini, ent, &nm, &nl);
    ASSERT_TRUE(ok == 1);
    ASSERT_TRUE(nl > 0u);

    const double *pos = NULL;
    uint32_t npos = 0u;
    ok = x_ini_get_array_f64(ini, ent, "position", &pos, &npos);
    ASSERT_TRUE(ok == 1);
    ASSERT_TRUE(npos == 3u);

    const double *scl = NULL;
    uint32_t nscl = 0u;
    ok = x_ini_get_array_f64(ini, ent, "scale", &scl, &nscl);
    ASSERT_TRUE(ok == 1);
    ASSERT_TRUE(nscl == 3u);
  }

  x_ini_close(ini);
  return 0;
}

/* ---------- Runner ---------- */

int main(void)
{
  STDXTestCase cases[] =
  {
    X_TEST(test_ini_open_and_root_children),
    X_TEST(test_ini_get_section_by_path),
    X_TEST(test_ini_discovery_iteration),
    X_TEST(test_ini_scalar_entries_and_types),
    X_TEST(test_ini_arrays_numeric_single_and_multiline),
    X_TEST(test_ini_missing_and_error_paths),
    X_TEST(test_ini_iteration_over_objects)
  };

  int rc = stdx_run_tests(cases, sizeof(cases) / sizeof(cases[0]));
  return rc;
}
