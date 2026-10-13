#define X_IMPL_TEST
#include <stdx_test.h>
#define X_IMPL_FILESYSTEM
#include <stdx_filesystem.h>
#include <stdx_string.h>


// ---------------------------------------------------------------------------
// Test path manipulation functions
// ---------------------------------------------------------------------------

int test_x_fs_path_executable_path(void)
{
  XFSPath p = {0};
  ASSERT_TRUE(x_fs_path_from_executable(&p) > 0);
  return 0;
}

int test_x_fs_path_init_and_set(void)
{
  XFSPath p;
  x_fs_path_init(&p);
  ASSERT_TRUE(x_fs_path_set(&p, "/usr"));
  ASSERT_TRUE(x_fs_path_compare_cstr(&p, "/usr") == 0);
  return 0;
}

int test_x_fs_path_append(void)
{
  XFSPath p;
  // Append
  ASSERT_TRUE(x_fs_path_append(&p, "bin") == 0);
  printf("%s\n", x_fs_path_cstr(&p));
#if _WIN32
  ASSERT_TRUE(x_fs_path_compare_cstr(&p, "/usr\\bin") == 0);
#else
  assert(strcmp(x_fs_path_cstr(&p), "/usr/bin") == 0);
#endif
  return 0;
}

int test_x_fs_path_join(void)
{
  XFSPath joined;
  ASSERT_TRUE(x_fs_path_join(&joined, "/usr", "local", "share", NULL) == 0);
  ASSERT_TRUE(x_fs_path_compare_cstr(&joined, "/usr/local/share") == 0);
  return 0;
}

int test_x_fs_path_normalize(void)
{
  XFSPath p;
  x_fs_path_init(&p);
  x_fs_path_set(&p, "//usr/../usr//bin//");
  ASSERT_TRUE(x_fs_path_compare_cstr(x_fs_path_normalize(&p), "/usr/bin/") == 0);
  return 0;
}

int test_x_fs_path_basename(void)
{
  XSlice base = x_fs_path_basename_cstr("/usr/local/bin/gcc");
  ASSERT_TRUE(x_slice_eq(base, x_slice("gcc")));
  return 0;
}

int test_x_fs_path_dirname(void)
{
  XSlice dirname = x_fs_path_dirname_cstr("/usr/local/bin/gcc");
  ASSERT_TRUE(x_slice_eq(dirname, x_slice("/usr/local/bin")));
  return 0;
}

int test_x_fs_path_extension(void)
{
  XSlice ext = x_fs_path_extension_cstr("/foo/bar/biz/bald/foo.tar.gz");
  ASSERT_FALSE(x_slice_is_empty(ext));
  ASSERT_TRUE(x_slice_eq_cstr(ext, "gz"));
  return 0;
}

int test_x_fs_path_extension_change(void)
{
  XFSPath p;
  x_fs_path_set(&p, "/bar/foo.txt");
  ASSERT_TRUE(x_fs_path_change_extension(&p, "md"));
  ASSERT_TRUE(x_fs_path_compare_cstr(&p, "/bar/foo.md") == 0);
  return 0;
}

int test_x_fs_path_is_absolute(void)
{
  // Is absolute
#ifdef _WIN32
  ASSERT_TRUE(x_fs_path_is_absolute_cstr("C:\\Windows") == 1);
#else
  ASSERT_TRUE(x_fs_path_is_absolute_cstr("/usr") == 1);
#endif
  return 0;
}

int test_x_fs_path_relative(void)
{
  // Relative
  XFSPath rel;
  ASSERT_TRUE(x_fs_path_common_prefix("/usr/local/", "/usr/local/bin/gcc", &rel));
  ASSERT_TRUE(x_fs_path_compare_cstr(&rel, "bin/gcc") == 0);
  return 0;
}

int test_x_fs_path_split(void)
{
  // Split
  XFSPath components[4];
  size_t count;

  ASSERT_TRUE(x_fs_path_split("/usr/local/bin", components, 4, &count));
  ASSERT_TRUE(count == 3);
  ASSERT_TRUE(x_fs_path_compare_cstr(&components[0], "usr") == 0);
  ASSERT_TRUE(x_fs_path_compare_cstr(&components[1], "local") == 0);
  ASSERT_TRUE(x_fs_path_compare_cstr(&components[2], "bin") == 0);
  return 0;
}

int test_x_fs_path_exists(void)
{
  ASSERT_TRUE(x_fs_path_exists_cstr(".") == 1);
  return 0;
}

// ---------------------------------------------------------------------------
// Test filesystem functions
// ---------------------------------------------------------------------------

int test_x_fs_path_equality(void)
{
  XFSPath a = {0};
  x_fs_path(&a, "usr","local","bin");
  ASSERT_TRUE(x_fs_path_eq_cstr(&a, "usr/local/bin/"));

  XFSPath b = {0};
  x_fs_path(&b, "usr");
  x_fs_path_join(&b, "local", "bin");
  ASSERT_TRUE(x_fs_path_eq(&a, &b));
  return 0;
}

int test_x_fs_file_operations(void)
{
  FILE* f = fopen("testfile.txt", "w");
  fprintf(f, "test\n");
  fclose(f);
  ASSERT_TRUE(x_fs_path_exists_cstr("testfile.txt"));
  ASSERT_TRUE(x_fs_path_is_file_cstr("testfile.txt"));

  ASSERT_TRUE(x_fs_file_copy("testfile.txt", "copy.txt"));
  ASSERT_TRUE(x_fs_path_exists_cstr("copy.txt"));
  ASSERT_TRUE(x_fs_path_is_file_cstr("copy.txt"));

  ASSERT_TRUE(x_fs_file_rename("copy.txt", "copy_renamed.txt"));
  ASSERT_TRUE(x_fs_path_exists_cstr("copy_renamed.txt"));

  remove("testfile.txt");
  remove("copy_renamed.txt");
  return 0;
}

int test_x_fs_path_functions(void)
{
#if defined(_WIN32)
  ASSERT_TRUE(x_fs_path_is_absolute_cstr("c:/usr/bin"));
#else
  ASSERT_TRUE(x_fs_path_is_absolute("/usr/bin"));
#endif
  ASSERT_TRUE(x_fs_path_is_relative_cstr("docs/manual.txt"));
  ASSERT_TRUE(x_fs_path_exists_cstr("."));
  ASSERT_TRUE(x_fs_path_is_directory_cstr("."));
  ASSERT_FALSE(x_fs_path_is_directory_cstr("nonexistent.txt"));
  ASSERT_FALSE(x_fs_path_is_file_cstr("."));

  XSlice name = x_fs_path_basename_cstr("/usr/bin/clang");
  ASSERT_TRUE(strncmp(name.ptr, "clang", name.length) == 0);
  XSlice ext = x_fs_path_extension_cstr("program.c");
  ASSERT_TRUE(strncmp(ext.ptr, "c", ext.length) == 0);

  XSlice parent = x_fs_path_dirname_cstr("/usr/bin/clang");
  ASSERT_TRUE(strncmp(parent.ptr, "/usr/bin", parent.length) == 0);

  XFSPath path;
  ASSERT_TRUE(x_fs_path(&path, "a", "b", ".." ,"c"));
  x_fs_path_normalize(&path);
  ASSERT_TRUE(x_fs_path_eq_cstr(&path, "a/c"));
  return 0;
}

int test_x_fs_directory_operations(void)
{
  ASSERT_TRUE(x_fs_directory_create("testdir"));
  ASSERT_TRUE(x_fs_path_is_directory_cstr("testdir"));

  ASSERT_TRUE(x_fs_directory_create_recursive("testdir/deep/nested"));
  ASSERT_TRUE(x_fs_path_is_directory_cstr("testdir/deep/nested"));

  ASSERT_TRUE(x_fs_directory_delete("testdir/deep/nested"));
  ASSERT_FALSE(x_fs_path_exists_cstr("testdir/deep/nested"));

  return 0;
}

int test_x_fs_cwd_functions(void)
{
  XFSPath cwd;
  size_t len = x_fs_cwd_get(&cwd);
  ASSERT_TRUE(len > 0);
  ASSERT_TRUE(x_fs_directory_create("cwd_test"));
  ASSERT_TRUE(x_fs_cwd_set("cwd_test") > 0);

  XFSPath newCwd;
  x_fs_cwd_get(&newCwd);
  ASSERT_TRUE(strstr(newCwd.buf, "cwd_test") != NULL);

  x_fs_cwd_set(cwd.buf);  // reset
  x_fs_directory_delete("cwd_test");
  return 0;
}

int test_x_fs_directory_traversal(void)
{
  x_fs_directory_create("dirlist");
  FILE* f1 = fopen("dirlist/a.txt", "w"); fclose(f1);
  FILE* f2 = fopen("dirlist/b.txt", "w"); fclose(f2);

  XFSDireEntry entry;
  XFSDireHandle* handle = x_fs_find_first_file("dirlist", &entry);
  ASSERT_TRUE(handle != NULL);
  int count = 1;

  while (x_fs_find_next_file(handle, &entry))
  {
    count++;
  }

  x_fs_find_close(handle);
  ASSERT_TRUE(count >= 2);

  remove("dirlist/a.txt");
  remove("dirlist/b.txt");
  remove("dirlist");
  return 0;
}

int test_x_fs_temp_folder(void)
{
  XFSPath temp;
  ASSERT_TRUE(x_fs_get_temp_folder(&temp) > 0);
  ASSERT_TRUE(x_fs_path_exists(&temp));
  return 0;
}

int test_x_fs_path_join_slice(void)
{
  XFSPath out = {0};
  x_smallstr_clear(&out);

  XFSPath a;
  x_smallstr_clear(&a);
  ASSERT_TRUE(x_fs_path_set(&a, "dir") != 0);

  XFSPath b;
  x_smallstr_clear(&b);
  ASSERT_TRUE(x_fs_path_set(&b, "file.txt") != 0);

  XSlice sa = x_fs_path_as_slice(&a);
  XSlice sb = x_fs_path_as_slice(&b);

  ASSERT_TRUE(x_fs_path_join_slice(&out, &sa, &sb) != 0);

#ifdef _WIN32
  ASSERT_TRUE(x_fs_path_compare_cstr(&out, "dir\\file.txt") == 0);
#else
  ASSERT_TRUE(x_fs_path_compare_cstr(&out, "dir/file.txt") == 0);
#endif

  return 0;
}


// ---------------------------------------------------------------------------
// Test filesystem watching
// ---------------------------------------------------------------------------

static void s_test_wait_ms(int32_t milliseconds)
{
  clock_t start = clock();
  clock_t ticks =
      (clock_t)((double)CLOCKS_PER_SEC * (double)milliseconds / 1000.0);

  while ((clock() - start) < ticks)
  {
  }
}

static bool s_watch_event_wait(XFSWatch* watcher, XFSWatchEventType action,
    const char* filename, XFSWatchEvent* out_event)
{
  int32_t attempt;

  for (attempt = 0; attempt < 200; ++attempt)
  {
    XFSWatchEvent events[8];
    int32_t count = x_fs_watch_poll(watcher, events, 8);
    int32_t i;

    if (count < 0)
    {
      return false;
    }

    for (i = 0; i < count; ++i)
    {
      if (events[i].action == action && events[i].filename != NULL &&
          strcmp(events[i].filename, filename) == 0)
      {
        if (out_event)
        {
          *out_event = events[i];
        }
        return true;
      }
    }

    s_test_wait_ms(10);
  }

  return false;
}

static void s_watch_clobber_stack(void)
{
  volatile unsigned char buffer[X_FS_PATH_MAX_LENGTH * 2];
  size_t i;

  for (i = 0; i < sizeof(buffer); ++i)
  {
    buffer[i] = (unsigned char)i;
  }

  (void)buffer[0];
}

int test_x_fs_watch_empty(void)
{
  XFSWatch* watcher = x_fs_watch_open(".");
  ASSERT_TRUE(watcher != NULL);
  x_fs_watch_close(watcher);
  return 0;
}

int test_x_fs_watch_events(void)
{
  XFSPath directory = {0};
  XFSPath created = {0};
  XFSPath renamed = {0};
  XFSWatch* watcher;
  XFSWatchEvent event = {0};
  FILE* file;

  ASSERT_TRUE(x_fs_make_temp_directory("stdx_watch_", &directory));
  ASSERT_TRUE(x_fs_path(&created, directory.buf, "created.txt"));
  ASSERT_TRUE(x_fs_path(&renamed, directory.buf, "renamed.txt"));

  watcher = x_fs_watch_open(directory.buf);
  ASSERT_TRUE(watcher != NULL);

  file = fopen(created.buf, "wb");
  ASSERT_TRUE(file != NULL);
  ASSERT_TRUE(fputs("created", file) >= 0);
  fclose(file);

  ASSERT_TRUE(s_watch_event_wait(
      watcher, x_fs_watch_CREATED, "created.txt", &event));
  s_watch_clobber_stack();
  ASSERT_TRUE(event.filename != NULL);
  ASSERT_TRUE(strcmp(event.filename, "created.txt") == 0);

  file = fopen(created.buf, "ab");
  ASSERT_TRUE(file != NULL);
  ASSERT_TRUE(fputs(" modified", file) >= 0);
  fclose(file);

  ASSERT_TRUE(s_watch_event_wait(
      watcher, x_fs_watch_MODIFIED, "created.txt", NULL));

  ASSERT_TRUE(x_fs_file_rename(created.buf, renamed.buf));
  ASSERT_TRUE(s_watch_event_wait(
      watcher, x_fs_watch_RENAMED_TO, "renamed.txt", NULL));

  ASSERT_TRUE(x_fs_file_delete(renamed.buf));
  ASSERT_TRUE(s_watch_event_wait(
      watcher, x_fs_watch_DELETED, "renamed.txt", NULL));

  x_fs_watch_close(watcher);
  ASSERT_TRUE(x_fs_directory_delete(directory.buf));
  return 0;
}

int main()
{
  STDXTestCase tests[] =
  {
    X_TEST(test_x_fs_path_executable_path),
    X_TEST(test_x_fs_path_init_and_set),
    X_TEST(test_x_fs_path_normalize),
    X_TEST(test_x_fs_path_basename),
    X_TEST(test_x_fs_path_dirname),
    X_TEST(test_x_fs_path_extension),
    X_TEST(test_x_fs_path_extension_change),
    X_TEST(test_x_fs_path_is_absolute),
    X_TEST(test_x_fs_path_relative),
    X_TEST(test_x_fs_path_split),
    X_TEST(test_x_fs_path_exists),

    X_TEST(test_x_fs_path_equality),
    X_TEST(test_x_fs_path_functions),
    X_TEST(test_x_fs_file_operations),
    X_TEST(test_x_fs_directory_operations),
    X_TEST(test_x_fs_cwd_functions),
    X_TEST(test_x_fs_directory_traversal),
    X_TEST(test_x_fs_temp_folder),

    X_TEST(test_x_fs_watch_empty),
    X_TEST(test_x_fs_watch_events),
    X_TEST(test_x_fs_path_join_slice),

  };

  return x_tests_run(tests, sizeof(tests)/sizeof(tests[0]), NULL);
}
