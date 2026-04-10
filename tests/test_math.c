// tests/test_stdx_math.c
#include <stdx_common.h>
#include <math.h>
#define X_IMPL_TEST
#include <stdx_test.h>
#define X_IMPL_MATH
#include "stdx_math.h"

#ifndef X_TEST_EPS
#define X_TEST_EPS 1e-5f
#endif

#define X_CHECK(cond) ASSERT_TRUE(cond)
#define X_REQUIRE(cond) ASSERT_TRUE(cond)

#define X_CHECK_FLOAT_EQ(a, b, eps) ASSERT_TRUE(fabsf((a) - (b)) <= (eps))
#define X_CHECK_VEC3_EQ(a, b, eps) \
  ASSERT_TRUE(fabsf((a).x - (b).x) <= (eps) && \
              fabsf((a).y - (b).y) <= (eps) && \
              fabsf((a).z - (b).z) <= (eps))

static int is_orthonormal_columns(Mat4 m, float eps)
{
  Vec3 c0 = (Vec3){m.m[0], m.m[1], m.m[2]};
  Vec3 c1 = (Vec3){m.m[4], m.m[5], m.m[6]};
  Vec3 c2 = (Vec3){m.m[8], m.m[9], m.m[10]};
  float d01 = vec3_dot(c0, c1);
  float d02 = vec3_dot(c0, c2);
  float d12 = vec3_dot(c1, c2);
  float n0 = fabsf(vec3_len(c0) - 1.0f);
  float n1 = fabsf(vec3_len(c1) - 1.0f);
  float n2 = fabsf(vec3_len(c2) - 1.0f);
  return fabsf(d01) <= eps &&
         fabsf(d02) <= eps &&
         fabsf(d12) <= eps &&
         n0 <= eps &&
         n1 <= eps &&
         n2 <= eps;
}

int test_scalar_basics(void)
{
  X_CHECK(float_eq(1.0f, 1.0f + 1e-7f));
  X_CHECK(float_is_zero(5e-7f));
  X_CHECK_FLOAT_EQ(deg_to_rad(180.0f), STDXM_PI, 1e-6f);
  X_CHECK_FLOAT_EQ(rad_to_deg(STDXM_PI), 180.0f, 1e-5f);
  X_CHECK_FLOAT_EQ(float_clamp(5, 0, 3), 3, 0);
  X_CHECK_FLOAT_EQ(float_lerp(0, 10, 0.25f), 2.5f, 1e-6f);
  return 0;
}

int test_vec3_core(void)
{
  Vec3 a = vec3_make(1, 2, 3);
  Vec3 b = vec3_make(-4, 5, 0.5f);

  X_CHECK_FLOAT_EQ(vec3_dot(a, b), 1 * -4 + 2 * 5 + 3 * 0.5f, 1e-6f);
  X_CHECK_FLOAT_EQ(vec3_len(vec3_make(0, 3, 4)), 5.0f, 1e-6f);
  X_CHECK_VEC3_EQ(
    vec3_reflect(vec3_make(1, -1, 0), vec3_norm(vec3_make(0, 1, 0))),
    ((Vec3){1, 1, 0}),
    1e-6f
  );

  {
    Vec3 p = vec3_project(a, b);
    X_CHECK(fabsf(vec3_len(vec3_cross(p, b))) <= 1e-5f || vec3_len(b) <= 1e-6f);
  }

  return 0;
}

int test_vec2_norm_zero_safe(void)
{
  Vec2 z = vec2_make(0, 0);
  Vec2 nz = vec2_norm(z);
  X_CHECK(vec2_cmp(nz, z));
  return 0;
}

int test_quat_axis_angle_roundtrip(void)
{
  Vec3 axis = vec3_norm(vec3_make(2, 3, 4));
  float angle = 1.2345f;
  Quat q = quat_axis_angle(axis, angle);
  Mat4 R = mat4_from_quat(q);
  Vec3 v = vec3_make(0.2f, -0.1f, 0.7f);
  Vec3 mrot = mat4_mul_dir(R, v);
  Vec3 qrot = quat_mul_vec3(q, v);

  X_CHECK_VEC3_EQ(mrot, qrot, 1e-5f);
  return 0;
}

int test_quat_slerp_endpoints(void)
{
  Quat a = quat_axis_angle(vec3_make(0, 1, 0), 0.0f);
  Quat b = quat_axis_angle(vec3_make(0, 1, 0), STDXM_PI * 0.5f);
  Quat s0 = quat_slerp(a, b, 0.0f);
  Quat s1 = quat_slerp(a, b, 1.0f);
  Vec3 v = vec3_make(1, 0, 0);

  X_CHECK_VEC3_EQ(quat_mul_vec3(s0, v), quat_mul_vec3(a, v), 1e-5f);
  X_CHECK_VEC3_EQ(quat_mul_vec3(s1, v), quat_mul_vec3(b, v), 1e-5f);
  return 0;
}

int test_quat_from_to_edges(void)
{
  Vec3 x = vec3_make(1, 0, 0);
  Vec3 nx = vec3_make(-1, 0, 0);
  Quat qsame = quat_from_to(x, x);
  Quat qoppo = quat_from_to(x, nx);

  X_CHECK_VEC3_EQ(quat_mul_vec3(qsame, x), x, 1e-5f);
  X_CHECK_VEC3_EQ(quat_mul_vec3(qoppo, x), nx, 1e-5f);
  return 0;
}

int test_quatdual_transform_matches_mat4(void)
{
  Vec3 t = vec3_make(1.0f, 2.0f, -3.0f);
  Quat r = quat_axis_angle(vec3_make(0, 1, 0), 0.75f);
  QuatDual qd = quatdual_from_rt(r, t);
  Vec3 p = vec3_make(0.5f, -1.0f, 2.0f);

  Mat4 M = mat4_compose(t, r, vec3_make(1, 1, 1));
  Vec3 pM = mat4_mul_point(M, p);
  Vec3 pQD = quatdual_mul_vec3(qd, p);
  X_CHECK_VEC3_EQ(pM, pQD, 1e-5f);

  {
    Vec3 d = vec3_make(0.1f, 0.2f, 0.3f);
    X_CHECK_VEC3_EQ(mat4_mul_dir(M, d), quatdual_mul_vec3_rot(qd, d), 1e-5f);
  }

  return 0;
}

int test_mat4_look_at_rh_orthonormal(void)
{
  Vec3 eye = vec3_make(1, 2, 3);
  Vec3 tgt = vec3_make(0, 0, 0);
  Vec3 up = vec3_make(0, 1, 0);
  Mat4 V = mat4_look_at_rh(eye, tgt, up);

  X_CHECK(is_orthonormal_columns(V, 1e-5f));

  {
    Vec3 f = vec3_neg((Vec3){V.m[2], V.m[6], V.m[10]});
    Vec3 dir = vec3_norm(vec3_sub(tgt, eye));
    X_CHECK_VEC3_EQ(f, dir, 1e-5f);
  }

  return 0;
}

int test_mat4_compose_decompose_roundtrip(void)
{
  Vec3 t = vec3_make(-2.0f, 0.5f, 5.0f);
  Quat r = quat_axis_angle(vec3_norm(vec3_make(1.0f, 2.0f, 3.0f)), 1.0f);
  Vec3 s = vec3_make(-2.0f, 3.0f, 0.5f);

  Mat4 M = mat4_compose(t, r, s);

  Vec3 td;
  Quat rd;
  Vec3 sd;
  mat4_decompose(M, &td, &rd, &sd);

  {
    Mat4 M2 = mat4_compose(td, rd, sd);
    const float EPS = 5e-5f;
    const int idx[] = {0, 1, 2, 4, 5, 6, 8, 9, 10, 12, 13, 14};

    X_CHECK_FLOAT_EQ(td.x, t.x, 1e-6f);
    X_CHECK_FLOAT_EQ(td.y, t.y, 1e-6f);
    X_CHECK_FLOAT_EQ(td.z, t.z, 1e-6f);

    for (int k = 0; k < (int)(sizeof(idx) / sizeof(idx[0])); ++k)
    {
      int i = idx[k];
      X_CHECK_FLOAT_EQ(M2.m[i], M.m[i], EPS);
    }

    {
      Mat4 Mx = mat4_mul(
        mat4_mul(mat4_translate(td), mat4_from_quat(rd)),
        mat4_scale(sd)
      );

      for (int k = 0; k < (int)(sizeof(idx) / sizeof(idx[0])); ++k)
      {
        int i = idx[k];
        X_CHECK_FLOAT_EQ(Mx.m[i], M2.m[i], EPS);
      }
    }
  }

  return 0;
}

int test_mat4_inverse_affine_matches_full(void)
{
  Vec3 t = vec3_make(3, -2, 1);
  Quat r = quat_axis_angle(vec3_make(0, 0, 1), 0.3f);
  Vec3 s = vec3_make(2, 2, 2);
  Mat4 M = mat4_compose(t, r, s);

  Mat4 A = mat4_inverse_affine(M);

  {
    bool ok = false;
    Mat4 F = mat4_inverse_full(M, &ok);
    const float EPS = 5e-5f;
    Mat4 IA;
    Mat4 AI;
    int diag[4] = {0, 5, 10, 15};

    X_REQUIRE(ok);

    IA = mat4_mul(M, A);
    AI = mat4_mul(A, M);

    for (int d = 0; d < 4; ++d)
    {
      X_CHECK_FLOAT_EQ(IA.m[diag[d]], 1.0f, EPS);
      X_CHECK_FLOAT_EQ(AI.m[diag[d]], 1.0f, EPS);
    }

    for (int i = 0; i < 16; ++i)
    {
      if (i == 0 || i == 5 || i == 10 || i == 15)
      {
        continue;
      }

      X_CHECK_FLOAT_EQ(IA.m[i], 0.0f, EPS);
      X_CHECK_FLOAT_EQ(AI.m[i], 0.0f, EPS);
    }

    /* Separate full-inverse validation belongs in its own test. */
    (void)F;
  }

  return 0;
}

int test_mat4_inverse_full_is_inverse(void)
{
  Vec3 t = vec3_make(3, -2, 1);
  Quat r = quat_axis_angle(vec3_make(0, 0, 1), 0.3f);
  Vec3 s = vec3_make(2, 2, 2);
  Mat4 M = mat4_compose(t, r, s);

  bool ok = false;
  Mat4 F = mat4_inverse_full(M, &ok);
  Mat4 IF = mat4_mul(M, F);
  Mat4 FI = mat4_mul(F, M);
  const float EPS = 5e-5f;
  int diag[4] = {0, 5, 10, 15};

  X_REQUIRE(ok);

  for (int d = 0; d < 4; ++d)
  {
    X_CHECK_FLOAT_EQ(IF.m[diag[d]], 1.0f, EPS);
    X_CHECK_FLOAT_EQ(FI.m[diag[d]], 1.0f, EPS);
  }

  for (int i = 0; i < 16; ++i)
  {
    if (i == 0 || i == 5 || i == 10 || i == 15)
    {
      continue;
    }

    X_CHECK_FLOAT_EQ(IF.m[i], 0.0f, EPS);
    X_CHECK_FLOAT_EQ(FI.m[i], 0.0f, EPS);
  }

  return 0;
}

int test_mat4_inverse_full_singular_reports_failure(void)
{
  Mat4 M = mat4();
  bool ok = true;
  Mat4 I;

  M.m[0] = 1.0f;
  M.m[5] = 0.0f;
  M.m[10] = 1.0f;
  M.m[15] = 1.0f;

  I = mat4_inverse_full(M, &ok);
  (void)I;

  X_CHECK(ok == false);
  return 0;
}

int test_mat4_inverse_affine_uniform_scale_basic(void)
{
  Vec3 t = vec3_make(1, 2, 3);
  Quat r = quat_axis_angle(vec3_make(0, 0, 1), 0.5f);
  Vec3 s = vec3_make(2, 2, 2);
  Mat4 M = mat4_compose(t, r, s);
  Mat4 I = mat4_mul(M, mat4_inverse_affine_uniform_scale(M));
  const float EPS = 5e-5f;
  int diag[4] = {0, 5, 10, 15};

  for (int d = 0; d < 4; ++d)
  {
    X_CHECK_FLOAT_EQ(I.m[diag[d]], 1.0f, EPS);
  }

  for (int i = 0; i < 16; ++i)
  {
    if (i == 0 || i == 5 || i == 10 || i == 15)
    {
      continue;
    }

    X_CHECK_FLOAT_EQ(I.m[i], 0.0f, EPS);
  }

  return 0;
}

int test_perspective_rh_no_project_unproject_basic(void)
{
  float fovy = deg_to_rad(60.0f);
  float aspect = 16.0f / 9.0f;
  float n = 0.1f;
  float f = 100.0f;
  Mat4 P = mat4_perspective_rh_no(fovy, aspect, n, f);

  {
    Vec3 p = vec3_make(0, 0, -1.0f);
    Vec3 clip = mat4_mul_point(P, p);
    X_CHECK(fabsf(clip.x) < 1.0f + 1e-4f);
    X_CHECK(fabsf(clip.y) < 1.0f + 1e-4f);
  }

  {
    Vec3 pnear = vec3_make(0, 0, -n);
    Vec3 znear_ndc = mat4_mul_point(P, pnear);
    X_CHECK(znear_ndc.z <= -0.999f);
  }

  return 0;
}

int test_orthographic_zo_depth_mapping_signs(void)
{
  float l = -2;
  float r = 2;
  float b = -1;
  float t = 1;
  float n = 0.5f;
  float f = 10.0f;
  Mat4 Orh = mat4_orthographic_rh_zo(l, r, b, t, n, f);
  Mat4 Olh = mat4_orthographic_lh_zo(l, r, b, t, n, f);

  X_CHECK(Orh.m[10] < 0.0f);
  X_CHECK(Olh.m[10] > 0.0f);

  X_CHECK_FLOAT_EQ(Orh.m[14], -n / (f - n), 1e-6f);
  X_CHECK_FLOAT_EQ(Olh.m[14], -n / (f - n), 1e-6f);
  return 0;
}

int test_mat4_mul_dir_point_conventions(void)
{
  Mat4 T = mat4_translate(vec3_make(1, 2, 3));
  Vec3 p = vec3_make(0, 0, 0);
  Vec3 d = vec3_make(0, 1, 0);

  X_CHECK_VEC3_EQ(mat4_mul_point(T, p), ((Vec3){1, 2, 3}), 1e-6f);
  X_CHECK_VEC3_EQ(mat4_mul_dir(T, d), d, 1e-6f);
  return 0;
}

int test_mat4_minor_cofactor_identity(void)
{
  Mat4 I = mat4_identity();

  /* For identity:
     - diagonal minors are 1
     - off-diagonal minors are 0
     - cofactors match the same values here
  */
  X_CHECK_FLOAT_EQ(mat4_minor(I, 0, 0), 1.0f, 1e-6f);
  X_CHECK_FLOAT_EQ(mat4_minor(I, 1, 1), 1.0f, 1e-6f);
  X_CHECK_FLOAT_EQ(mat4_minor(I, 2, 2), 1.0f, 1e-6f);
  X_CHECK_FLOAT_EQ(mat4_minor(I, 3, 3), 1.0f, 1e-6f);

  X_CHECK_FLOAT_EQ(mat4_minor(I, 0, 1), 0.0f, 1e-6f);
  X_CHECK_FLOAT_EQ(mat4_minor(I, 0, 2), 0.0f, 1e-6f);
  X_CHECK_FLOAT_EQ(mat4_minor(I, 0, 3), 0.0f, 1e-6f);
  X_CHECK_FLOAT_EQ(mat4_minor(I, 1, 0), 0.0f, 1e-6f);
  X_CHECK_FLOAT_EQ(mat4_minor(I, 2, 0), 0.0f, 1e-6f);
  X_CHECK_FLOAT_EQ(mat4_minor(I, 3, 0), 0.0f, 1e-6f);

  X_CHECK_FLOAT_EQ(mat4_cofactor(I, 0, 0), 1.0f, 1e-6f);
  X_CHECK_FLOAT_EQ(mat4_cofactor(I, 1, 1), 1.0f, 1e-6f);
  X_CHECK_FLOAT_EQ(mat4_cofactor(I, 2, 2), 1.0f, 1e-6f);
  X_CHECK_FLOAT_EQ(mat4_cofactor(I, 3, 3), 1.0f, 1e-6f);

  X_CHECK_FLOAT_EQ(mat4_cofactor(I, 0, 1), 0.0f, 1e-6f);
  X_CHECK_FLOAT_EQ(mat4_cofactor(I, 1, 0), 0.0f, 1e-6f);
  X_CHECK_FLOAT_EQ(mat4_cofactor(I, 2, 3), 0.0f, 1e-6f);
  X_CHECK_FLOAT_EQ(mat4_cofactor(I, 3, 2), 0.0f, 1e-6f);

  return 0;
}

int test_mat4_cofactor_sign_relation(void)
{
  Mat4 M = mat4_identity();

  /* Give the matrix some non-trivial values while keeping it easy to reason about. */
  M.m[1]  = 2.0f;
  M.m[4]  = -3.0f;
  M.m[6]  = 4.0f;
  M.m[9]  = 5.0f;
  M.m[12] = 7.0f;

  for (int row = 0; row < 4; ++row)
  {
    for (int col = 0; col < 4; ++col)
    {
      float minor = mat4_minor(M, row, col);
      float expected = (((row + col) & 1) != 0) ? -minor : minor;
      float cofactor = mat4_cofactor(M, row, col);
      X_CHECK_FLOAT_EQ(cofactor, expected, 1e-6f);
    }
  }

  return 0;
}

int test_mat4_det_identity_is_one(void)
{
  Mat4 I = mat4_identity();
  X_CHECK_FLOAT_EQ(mat4_det(I), 1.0f, 1e-6f);
  return 0;
}

int test_mat4_det_scale_product(void)
{
  Vec3 s = vec3_make(2.0f, 3.0f, 4.0f);
  Mat4 M = mat4_scale(s);

  /* determinant of a pure scale matrix is sx * sy * sz */
  X_CHECK_FLOAT_EQ(mat4_det(M), 24.0f, 1e-6f);
  return 0;
}

int test_mat4_minor_cofactor_out_of_range_returns_zero(void)
{
  Mat4 I = mat4_identity();

  X_CHECK_FLOAT_EQ(mat4_minor(I, -1, 0), 0.0f, 0.0f);
  X_CHECK_FLOAT_EQ(mat4_minor(I, 0, -1), 0.0f, 0.0f);
  X_CHECK_FLOAT_EQ(mat4_minor(I, 4, 0), 0.0f, 0.0f);
  X_CHECK_FLOAT_EQ(mat4_minor(I, 0, 4), 0.0f, 0.0f);

  X_CHECK_FLOAT_EQ(mat4_cofactor(I, -1, 0), 0.0f, 0.0f);
  X_CHECK_FLOAT_EQ(mat4_cofactor(I, 0, -1), 0.0f, 0.0f);
  X_CHECK_FLOAT_EQ(mat4_cofactor(I, 4, 0), 0.0f, 0.0f);
  X_CHECK_FLOAT_EQ(mat4_cofactor(I, 0, 4), 0.0f, 0.0f);

  return 0;
}


int main(void)
{
  STDXTestCase tests[] =
  {
    X_TEST(test_scalar_basics),
    X_TEST(test_vec3_core),
    X_TEST(test_vec2_norm_zero_safe),
    X_TEST(test_quat_axis_angle_roundtrip),
    X_TEST(test_quat_slerp_endpoints),
    X_TEST(test_quat_from_to_edges),
    X_TEST(test_quatdual_transform_matches_mat4),
    X_TEST(test_mat4_look_at_rh_orthonormal),
    X_TEST(test_mat4_compose_decompose_roundtrip),
    X_TEST(test_mat4_inverse_affine_matches_full),
    X_TEST(test_mat4_inverse_full_is_inverse),
    X_TEST(test_mat4_inverse_full_singular_reports_failure),
    X_TEST(test_mat4_inverse_affine_uniform_scale_basic),
    X_TEST(test_perspective_rh_no_project_unproject_basic),
    X_TEST(test_orthographic_zo_depth_mapping_signs),
    X_TEST(test_mat4_mul_dir_point_conventions),
    X_TEST(test_mat4_minor_cofactor_identity),
    X_TEST(test_mat4_cofactor_sign_relation),
    X_TEST(test_mat4_det_identity_is_one),
    X_TEST(test_mat4_det_scale_product),
    X_TEST(test_mat4_minor_cofactor_out_of_range_returns_zero),
  };

  return x_tests_run(tests, sizeof(tests) / sizeof(tests[0]), NULL);
}
