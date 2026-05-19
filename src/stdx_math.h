/* stdx_math.h — Math functions
 * Part of the STDX General Purpose C Library by marciovmf
 *
 * Column-major matrices; multiply column vectors on the right.
 *  Right-handed coordinate system by default.
 *  Angles in radians.
 *  Depth ranges: NO = −1..1 (OpenGL), ZO = 0..1 (Vulkan/D3D).
 *
 * Usage:
 * #define X_IMPL_MATH
 * #include "stdx_math.h"
 */

#ifndef X_MATH_H
#define X_MATH_H

#ifndef X_MATH_API
#define X_MATH_API
#endif

#define X_MATH_VERSION_MAJOR 1
#define X_MATH_VERSION_MINOR 1
#define X_MATH_VERSION_PATCH 0

#define X_MATH_VERSION                                                                   \
  (X_MATH_VERSION_MAJOR * 10000 + X_MATH_VERSION_MINOR * 100 + X_MATH_VERSION_PATCH)

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#ifndef STDXM_EPS
#define STDXM_EPS 1e-6f
#endif
#ifndef STDXM_PI
#define STDXM_PI 3.14159265358979323846f
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  float x, y;
} Vec2;

typedef struct {
  float x, y, z;
} Vec3;

typedef struct {
  float x, y, z, w;
} Vec4;

typedef struct {
  float m[4];
} Mat2;

typedef struct {
  float m[9];
} Mat3;

typedef struct {
  float m[16];
} Mat4;

typedef struct {
  float x, y, z, w;
} Quat;

typedef struct {
  Quat real;
  Quat dual;
} QuatDual;


inline int32_t i32_max(int32_t a, int32_t b) { return (a > b) ? a : b; }
inline int32_t i32_min(int32_t a, int32_t b) { return (a < b) ? a : b; }

inline float float_max(float a, float b) { return (a > b) ? a : b; }
inline float float_min(float a, float b) { return (a < b) ? a : b; }

X_MATH_API bool float_eq(float a, float b); /* Compare floats using epsilon tolerance. */
X_MATH_API bool float_is_zero(float a); /* Returns true if |a| <= STDXM_EPS. */
X_MATH_API float float_clamp(float x, float a, float b); /* brief Clamp x between [a,b]. */
X_MATH_API float float_lerp(float a, float b, float t); /* Linear interpolation between a and b. */
X_MATH_API float float_smoothstep(float a, float b, float t); /* Smooth Hermite interpolation between a and b (ease-in/out). */
X_MATH_API float deg_to_rad(float d); /* Degrees → radians conversion. */
X_MATH_API float rad_to_deg(float r); /* Radians → degrees conversion. */

/**
 * 2D vector operations; all return-by-value.
 */
X_MATH_API bool vec2_cmp(Vec2 a, Vec2 b); /* Approximate equality */
X_MATH_API float vec2_dot(Vec2 a, Vec2 b); /* Dot product */
X_MATH_API float vec2_len(Vec2 a); /* Vector length */
X_MATH_API float vec2_len2(Vec2 a); /* Squared length */
X_MATH_API Vec2 vec2_abs(Vec2 v); /* Component-wise abs */
X_MATH_API Vec2 vec2_add(Vec2 a, Vec2 b); /* Component-wise add */
X_MATH_API Vec2 vec2_sub(Vec2 a, Vec2 b); /* Component-wise subtract */
X_MATH_API Vec2 vec2_mul(Vec2 a, float s); /* Scale by scalar */
X_MATH_API Vec2 vec2_mul_vec2(Vec2 a, Vec2 b); /* Component-wise multiply */
X_MATH_API Vec2 vec2_div(Vec2 a, float s); /* Divide by scalar */
X_MATH_API Vec2 vec2_div_vec2(Vec2 a, Vec2 b); /* Component-wise divide */
X_MATH_API Vec2 vec2_neg(Vec2 v); /* Negate */
X_MATH_API Vec2 vec2_norm(Vec2 a); /* Normalize; returns zero if length≈0 */
X_MATH_API Vec2 vec2_clamp(Vec2 v, float a, float b); /* Clamp components */
X_MATH_API Vec2 vec2_max(Vec2 a, Vec2 b); /* Component-wise max */
X_MATH_API Vec2 vec2_min(Vec2 a, Vec2 b); /* Component-wise min */
X_MATH_API Vec2 vec2_floor(Vec2 v); /* floor() per component */
X_MATH_API Vec2 vec2_fmod(Vec2 v, float s); /* fmod() per component */
X_MATH_API Vec2 vec2_reflect(Vec2 v, Vec2 n); /* Reflect vector v about normal n */
X_MATH_API Vec2 vec2_lerp(Vec2 a, Vec2 b, float t); /* Lerp components */
X_MATH_API Vec2 vec2_smoothstep(Vec2 a, Vec2 b, float t); /* Smoothstep components */
X_MATH_API Vec2 vec2_make(float x, float y); /* Construct Vec2 */

/**
 * @brief 3D vector operations in right-handed space.
 */
X_MATH_API bool vec3_cmp(Vec3 a, Vec3 b);
X_MATH_API float vec3_dot(Vec3 a, Vec3 b);
X_MATH_API float vec3_len(Vec3 a);
X_MATH_API float vec3_len2(Vec3 a);
X_MATH_API Vec3 vec3_abs(Vec3 v);
X_MATH_API Vec3 vec3_add(Vec3 a, Vec3 b);
X_MATH_API Vec3 vec3_sub(Vec3 a, Vec3 b);
X_MATH_API Vec3 vec3_mul(Vec3 a, float s);
X_MATH_API Vec3 vec3_mul_vec3(Vec3 a, Vec3 b);
X_MATH_API Vec3 vec3_div(Vec3 a, float s);
X_MATH_API Vec3 vec3_div_vec3(Vec3 a, Vec3 b);
X_MATH_API Vec3 vec3_neg(Vec3 v);
X_MATH_API Vec3 vec3_norm(Vec3 a); /* Safe normalization */
X_MATH_API Vec3 vec3_cross(Vec3 a, Vec3 b); /* Right-handed cross product */
X_MATH_API Vec3 vec3_project(Vec3 a, Vec3 b); /* Project a onto b; returns 0 if b≈0 */
X_MATH_API Vec3 vec3_reflect(Vec3 v, Vec3 n); /* Reflect about normal n (n unit) */
X_MATH_API Vec3 vec3_refract(
    Vec3 v, Vec3 n, float eta); /* Refract v through surface with ratio η */
X_MATH_API Vec3 vec3_clamp(Vec3 v, float a, float b);
X_MATH_API Vec3 vec3_max(Vec3 a, Vec3 b);
X_MATH_API Vec3 vec3_min(Vec3 a, Vec3 b);
X_MATH_API Vec3 vec3_floor(Vec3 v);
X_MATH_API Vec3 vec3_fmod(Vec3 v, float s);
X_MATH_API Vec3 vec3_lerp(Vec3 a, Vec3 b, float t);
X_MATH_API Vec3 vec3_smoothstep(Vec3 a, Vec3 b, float t);
X_MATH_API Vec3 vec3_make(float x, float y, float z);

/**
 * 4D vector operations, used mainly for homogeneous coords.
 */
X_MATH_API float vec4_dot(Vec4 a, Vec4 b);
X_MATH_API Vec4 vec4_abs(Vec4 v);
X_MATH_API Vec4 vec4_add(Vec4 a, Vec4 b);
X_MATH_API Vec4 vec4_sub(Vec4 a, Vec4 b);
X_MATH_API Vec4 vec4_mul(Vec4 a, float s);
X_MATH_API Vec4 vec4_mul_vec4(Vec4 a, Vec4 b);
X_MATH_API Vec4 vec4_div(Vec4 a, float s);
X_MATH_API Vec4 vec4_lerp(Vec4 a, Vec4 b, float t);
X_MATH_API Vec4 vec4_smoothstep(Vec4 a, Vec4 b, float t);
X_MATH_API Vec4 vec4_make(float x, float y, float z, float w);

/**
 * All matrices are column-major; multiply column vectors on the right.
 */
X_MATH_API Mat2 mat2_identity(void);
X_MATH_API Mat2 mat2_make(float a, float b, float c, float d);
X_MATH_API Mat2 mat2_transpose(Mat2 m);
X_MATH_API float mat2_det(Mat2 m);
X_MATH_API Mat2 mat2_inverse(Mat2 m);
X_MATH_API Mat3 mat3_identity(void);
X_MATH_API Mat3 mat3_make(float xx, float xy, float xz, float yx, float yy, float yz,
    float zx, float zy, float zz);
X_MATH_API Mat3 mat3_transpose(Mat3 a);
X_MATH_API float mat3_det(Mat3 m);
X_MATH_API Mat3 mat3_inverse(Mat3 m);
X_MATH_API Mat3 mat3_rot_x(float a);
X_MATH_API Mat3 mat3_rot_y(float a);
X_MATH_API Mat3 mat3_rot_z(float a);
X_MATH_API Mat3 mat3_scale(Vec3 s);

/**
 * 4×4 matrix functions (column-major, right-handed by default).
 * note Multiply column vectors: p' = M * p
 */

#define mat4() ((Mat4) { 0 })

X_MATH_API Mat4 mat4_identity(void);
X_MATH_API Mat4 mat4_make(float xx, float xy, float xz, float xw, float yx, float yy,
    float yz, float yw, float zx, float zy, float zz, float zw, float wx, float wy,
    float wz, float ww);
X_MATH_API Mat4 mat4_transpose(Mat4 a);
X_MATH_API Mat4 mat4_mul(Mat4 a, Mat4 b); /* a·b (apply b then a) */
X_MATH_API Vec3 mat4_mul_point(Mat4 m, Vec3 p); /* Apply transform to point (w=1) */
X_MATH_API Vec3 mat4_mul_dir(Mat4 m, Vec3 v); /* Apply transform to direction (w=0) */
X_MATH_API Mat4 mat4_translate(Vec3 t);
X_MATH_API Mat4 mat4_scale(Vec3 s);
X_MATH_API Mat4 mat4_rot_x(float a);
X_MATH_API Mat4 mat4_rot_y(float a);
X_MATH_API Mat4 mat4_rot_z(float a);
X_MATH_API Mat4 mat4_from_axis_angle(Vec3 axis, float angle);
X_MATH_API Mat4 mat4_from_euler(float rx, float ry, float rz);
X_MATH_API Mat4 mat4_from_quat(Quat q);
X_MATH_API Mat4 mat4_compose(Vec3 t, Quat r, Vec3 s);
X_MATH_API float mat4_det(Mat4 m); /* 4×4 determinant */
X_MATH_API float mat4_minor(Mat4 m, int row, int col);
X_MATH_API float mat4_cofactor(Mat4 m, int row, int col);
X_MATH_API Mat4 mat4_inverse_affine(
    Mat4 m); /* Fast inverse for affine TRS without shear */
X_MATH_API Mat4 mat4_inverse_affine_uniform_scale(Mat4 m);
X_MATH_API Mat4 mat4_inverse_full(Mat4 m, bool* ok);
X_MATH_API Mat4 mat4_look_at_rh(Vec3 eye, Vec3 target, Vec3 up); /**< Right-handed view */
X_MATH_API Mat4 mat4_look_at_lh(Vec3 eye, Vec3 target, Vec3 up); /**< Left-handed view */
X_MATH_API Mat4 mat4_perspective_rh_no(
    float fovy, float aspect, float n, float f); /* RH, NO (−1..1) */
X_MATH_API Mat4 mat4_perspective_rh_zo(
    float fovy, float aspect, float n, float f); /* RH, ZO (0..1) */
X_MATH_API Mat4 mat4_perspective_lh_no(
    float fovy, float aspect, float n, float f); /* LH, NO (−1..1) */
X_MATH_API Mat4 mat4_perspective_lh_zo(
    float fovy, float aspect, float n, float f); /* LH, ZO (0..1) */
X_MATH_API Mat4 mat4_orthographic_rh_no(
    float l, float r, float b, float t, float n, float f); /* RH, NO (−1..1) */
X_MATH_API Mat4 mat4_orthographic_rh_zo(
    float l, float r, float b, float t, float n, float f); /* RH, ZO (0..1) */
X_MATH_API Mat4 mat4_orthographic_lh_no(
    float l, float r, float b, float t, float n, float f); /* LH, NO (−1..1) */
X_MATH_API Mat4 mat4_orthographic_lh_zo(
    float l, float r, float b, float t, float n, float f); /* LH, ZO (0..1) */
X_MATH_API void mat4_decompose(
    Mat4 m, Vec3* out_t, Quat* out_r, Vec3* out_s); /** Decompose TRS (no shear) */

/**
 * Quaternion math (right-handed, scalar-last {x,y,z,w}).
 */
X_MATH_API Quat quat_id(void);
X_MATH_API Quat quat_make(float x, float y, float z, float w);
X_MATH_API Quat quat_norm(Quat q);
X_MATH_API Quat quat_conjugate(Quat q);
X_MATH_API Quat quat_inverse(Quat q);
X_MATH_API Quat quat_unit_inverse(Quat q);
X_MATH_API Quat quat_mul(Quat a, Quat b);
X_MATH_API Quat quat_axis_angle(Vec3 axis, float angle);
X_MATH_API Quat quat_from_to(Vec3 from, Vec3 to);
X_MATH_API Quat quat_from_mat3(Mat3 R);
X_MATH_API Quat quat_slerp(Quat a, Quat b, float t);
X_MATH_API Quat quat_exp(Quat w);
X_MATH_API Quat quat_log(Quat q);
X_MATH_API Quat quat_constrain(Quat q, Vec3 axis);
X_MATH_API Quat quat_constrain_y(Quat q);
X_MATH_API Quat quat_neg(Quat q);
X_MATH_API Quat quat_scale(Quat q, float s);
X_MATH_API Quat quat_interpolate(const Quat* qs, const float* ws, int count);
X_MATH_API Vec3 quat_mul_vec3(Quat q, Vec3 v);
X_MATH_API Vec3 quat_get_right(Quat q);
X_MATH_API Vec3 quat_get_up(Quat q);
X_MATH_API Vec3 quat_get_forward(Quat q);
X_MATH_API Vec3 quat_to_euler_xyz(Quat q);

/**
 * Dual quaternion operations (translation + rotation).
 */
X_MATH_API Quat quat_add(Quat a, Quat b);
X_MATH_API QuatDual quatdual_id(void);
X_MATH_API QuatDual quatdual_make(Quat real, Quat dual);
X_MATH_API QuatDual quatdual_from_rt(
    Quat r, Vec3 t); /* Build dual Quat from rotation+translation */
X_MATH_API QuatDual quatdual_mul(QuatDual a, QuatDual b);
X_MATH_API QuatDual quatdual_norm(QuatDual qd);
X_MATH_API Vec3 quatdual_mul_vec3_rot(QuatDual qd, Vec3 v); /* Rotate vector only */
X_MATH_API Vec3 quatdual_mul_vec3(
    QuatDual qd, Vec3 v); /* Full transform (rotate + translate) */

#ifdef __cplusplus
}
#endif

#ifdef X_IMPL_MATH

float float_clamp(float x, float a, float b) { return x < a ? a : (x > b ? b : x); }

float float_lerp(float a, float b, float t) { return a + (b - a) * t; }

float float_smoothstep(float a, float b, float t)
{
  if (float_eq(a, b)) {
    return (t < a) ? 0.0f : 1.0f;
  }

  t = float_clamp((t - a) / (b - a), 0.0f, 1.0f);
  return t * t * (3.0f - 2.0f * t);
}

X_MATH_API bool float_eq(float a, float b) { return fabsf(a - b) <= STDXM_EPS; }

X_MATH_API bool float_is_zero(float a) { return fabsf(a) <= STDXM_EPS; }

X_MATH_API float deg_to_rad(float d) { return d * (STDXM_PI / 180.0f); }

X_MATH_API float rad_to_deg(float r) { return r * (180.0f / STDXM_PI); }

X_MATH_API Vec2 vec2_make(float x, float y) { return (Vec2) { x, y }; }

X_MATH_API Vec2 vec2_add(Vec2 a, Vec2 b) { return vec2_make(a.x + b.x, a.y + b.y); }

X_MATH_API Vec2 vec2_sub(Vec2 a, Vec2 b) { return vec2_make(a.x - b.x, a.y - b.y); }

X_MATH_API Vec2 vec2_mul(Vec2 a, float s) { return vec2_make(a.x * s, a.y * s); }

X_MATH_API Vec2 vec2_mul_vec2(Vec2 a, Vec2 b) { return vec2_make(a.x * b.x, a.y * b.y); }

X_MATH_API Vec2 vec2_div(Vec2 a, float s) { return vec2_make(a.x / s, a.y / s); }

X_MATH_API Vec2 vec2_div_vec2(Vec2 a, Vec2 b) { return vec2_make(a.x / b.x, a.y / b.y); }

X_MATH_API Vec2 vec2_neg(Vec2 v) { return vec2_make(-v.x, -v.y); }

X_MATH_API Vec2 vec2_abs(Vec2 v) { return vec2_make(fabsf(v.x), fabsf(v.y)); }

X_MATH_API Vec2 vec2_floor(Vec2 v) { return vec2_make(floorf(v.x), floorf(v.y)); }

X_MATH_API Vec2 vec2_fmod(Vec2 v, float s)
{
  return vec2_make(fmodf(v.x, s), fmodf(v.y, s));
}

X_MATH_API Vec2 vec2_max(Vec2 a, Vec2 b)
{
  return vec2_make(fmaxf(a.x, b.x), fmaxf(a.y, b.y));
}

X_MATH_API Vec2 vec2_min(Vec2 a, Vec2 b)
{
  return vec2_make(fminf(a.x, b.x), fminf(a.y, b.y));
}

X_MATH_API Vec2 vec2_clamp(Vec2 v, float a, float b)
{
  return vec2_make(float_clamp(v.x, a, b), float_clamp(v.y, a, b));
}

X_MATH_API float vec2_dot(Vec2 a, Vec2 b) { return a.x * b.x + a.y * b.y; }

X_MATH_API float vec2_len(Vec2 a) { return sqrtf(vec2_dot(a, a)); }

X_MATH_API float vec2_len2(Vec2 a) { return vec2_dot(a, a); }

X_MATH_API Vec2 vec2_norm(Vec2 a)
{
  float L = vec2_len(a);
  return (L > STDXM_EPS) ? vec2_div(a, L) : vec2_make(0, 0);
}

X_MATH_API Vec2 vec2_lerp(Vec2 a, Vec2 b, float t)
{
  return vec2_make(float_lerp(a.x, b.x, t), float_lerp(a.y, b.y, t));
}

X_MATH_API Vec2 vec2_smoothstep(Vec2 a, Vec2 b, float t)
{
  return vec2_make(float_smoothstep(a.x, b.x, t), float_smoothstep(a.y, b.y, t));
}

X_MATH_API bool vec2_cmp(Vec2 a, Vec2 b)
{
  return (float_eq(a.x, b.x) && float_eq(a.y, b.y));
}

X_MATH_API Vec2 vec2_reflect(Vec2 v, Vec2 n)
{
  float k = 2.0f * vec2_dot(v, n);
  return vec2_sub(v, vec2_mul(n, k));
}

X_MATH_API Vec3 vec3_make(float x, float y, float z) { return (Vec3) { x, y, z }; }

X_MATH_API Vec3 vec3_add(Vec3 a, Vec3 b)
{
  return vec3_make(a.x + b.x, a.y + b.y, a.z + b.z);
}

X_MATH_API Vec3 vec3_sub(Vec3 a, Vec3 b)
{
  return vec3_make(a.x - b.x, a.y - b.y, a.z - b.z);
}

X_MATH_API Vec3 vec3_mul(Vec3 a, float s) { return vec3_make(a.x * s, a.y * s, a.z * s); }

X_MATH_API Vec3 vec3_mul_vec3(Vec3 a, Vec3 b)
{
  return vec3_make(a.x * b.x, a.y * b.y, a.z * b.z);
}

X_MATH_API Vec3 vec3_div(Vec3 a, float s) { return vec3_make(a.x / s, a.y / s, a.z / s); }

X_MATH_API Vec3 vec3_div_vec3(Vec3 a, Vec3 b)
{
  return vec3_make(a.x / b.x, a.y / b.y, a.z / b.z);
}

X_MATH_API Vec3 vec3_neg(Vec3 v) { return vec3_make(-v.x, -v.y, -v.z); }

X_MATH_API Vec3 vec3_abs(Vec3 v) { return vec3_make(fabsf(v.x), fabsf(v.y), fabsf(v.z)); }

X_MATH_API Vec3 vec3_floor(Vec3 v)
{
  return vec3_make(floorf(v.x), floorf(v.y), floorf(v.z));
}

X_MATH_API Vec3 vec3_fmod(Vec3 v, float s)
{
  return vec3_make(fmodf(v.x, s), fmodf(v.y, s), fmodf(v.z, s));
}

X_MATH_API Vec3 vec3_max(Vec3 a, Vec3 b)
{
  return vec3_make(fmaxf(a.x, b.x), fmaxf(a.y, b.y), fmaxf(a.z, b.z));
}

X_MATH_API Vec3 vec3_min(Vec3 a, Vec3 b)
{
  return vec3_make(fminf(a.x, b.x), fminf(a.y, b.y), fminf(a.z, b.z));
}

X_MATH_API Vec3 vec3_clamp(Vec3 v, float a, float b)
{
  return vec3_make(
      float_clamp(v.x, a, b), float_clamp(v.y, a, b), float_clamp(v.z, a, b));
}

X_MATH_API float vec3_dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

X_MATH_API Vec3 vec3_cross(Vec3 a, Vec3 b)
{
  return vec3_make(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}

X_MATH_API float vec3_len(Vec3 a) { return sqrtf(vec3_dot(a, a)); }

X_MATH_API float vec3_len2(Vec3 a) { return vec3_dot(a, a); }

X_MATH_API Vec3 vec3_norm(Vec3 a)
{
  float L = vec3_len(a);
  return (L > STDXM_EPS) ? vec3_div(a, L) : vec3_make(0, 0, 0);
}

X_MATH_API bool vec3_cmp(Vec3 a, Vec3 b)
{
  return (float_eq(a.x, b.x) && float_eq(a.y, b.y) && float_eq(a.z, b.z));
}

X_MATH_API Vec3 vec3_lerp(Vec3 a, Vec3 b, float t)
{
  return vec3_make(
      float_lerp(a.x, b.x, t), float_lerp(a.y, b.y, t), float_lerp(a.z, b.z, t));
}

X_MATH_API Vec3 vec3_smoothstep(Vec3 a, Vec3 b, float t)
{
  return vec3_make(float_smoothstep(a.x, b.x, t), float_smoothstep(a.y, b.y, t),
      float_smoothstep(a.z, b.z, t));
}

X_MATH_API Vec3 vec3_reflect(Vec3 v, Vec3 n)
{
  float k = 2.0f * vec3_dot(v, n);
  return vec3_sub(v, vec3_mul(n, k));
}

X_MATH_API Vec3 vec3_refract(Vec3 v, Vec3 n, float eta)
{
  float cosi = -fmaxf(-1.0f, fminf(1.0f, vec3_dot(v, n)));
  float etai = 1.0f, etat = eta;
  Vec3 nn = n;
  if (cosi < 0.0f) {
    cosi = -cosi;
    float tmp = etai;
    etai = etat;
    etat = tmp;
    nn = vec3_neg(n);
  }
  float eta2 = etai / etat;
  float k = 1.0f - eta2 * eta2 * (1.0f - cosi * cosi);
  return (k < 0.0f) ? vec3_make(0, 0, 0)
                    : vec3_add(vec3_mul(v, eta2), vec3_mul(nn, eta2 * cosi - sqrtf(k)));
}

X_MATH_API Vec3 vec3_project(Vec3 a, Vec3 b)
{
  float d = vec3_dot(b, b);
  if (d <= STDXM_EPS)
    return vec3_make(0, 0, 0);
  return vec3_mul(b, vec3_dot(a, b) / d);
}

X_MATH_API Vec4 vec4_make(float x, float y, float z, float w)
{
  return (Vec4) { x, y, z, w };
}

X_MATH_API Vec4 vec4_add(Vec4 a, Vec4 b)
{
  return vec4_make(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
}

X_MATH_API Vec4 vec4_sub(Vec4 a, Vec4 b)
{
  return vec4_make(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w);
}

X_MATH_API Vec4 vec4_mul(Vec4 a, float s)
{
  return vec4_make(a.x * s, a.y * s, a.z * s, a.w * s);
}

X_MATH_API Vec4 vec4_mul_vec4(Vec4 a, Vec4 b)
{
  return vec4_make(a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w);
}

X_MATH_API Vec4 vec4_div(Vec4 a, float s)
{
  return vec4_make(a.x / s, a.y / s, a.z / s, a.w / s);
}

X_MATH_API float vec4_dot(Vec4 a, Vec4 b)
{
  return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

X_MATH_API Vec4 vec4_lerp(Vec4 a, Vec4 b, float t)
{
  return vec4_make(float_lerp(a.x, b.x, t), float_lerp(a.y, b.y, t),
      float_lerp(a.z, b.z, t), float_lerp(a.w, b.w, t));
}

X_MATH_API Vec4 vec4_smoothstep(Vec4 a, Vec4 b, float t)
{
  return vec4_make(float_smoothstep(a.x, b.x, t), float_smoothstep(a.y, b.y, t),
      float_smoothstep(a.z, b.z, t), float_smoothstep(a.w, b.w, t));
}

X_MATH_API Vec4 vec4_abs(Vec4 v)
{
  return vec4_make(fabsf(v.x), fabsf(v.y), fabsf(v.z), fabsf(v.w));
}

X_MATH_API Mat2 mat2_identity(void)
{
  Mat2 M = { { 1, 0, 0, 1 } };
  return M;
}

X_MATH_API Mat2 mat2_make(float a, float b, float c, float d)
{
  Mat2 M = { { a, b, c, d } };
  return M;
}

X_MATH_API Mat2 mat2_transpose(Mat2 m)
{
  Mat2 r = m;
  float t = r.m[1];
  r.m[1] = r.m[2];
  r.m[2] = t;
  return r;
}

X_MATH_API float mat2_det(Mat2 m) { return m.m[0] * m.m[3] - m.m[2] * m.m[1]; }

X_MATH_API Mat2 mat2_inverse(Mat2 m)
{
  float d = mat2_det(m);
  if (float_is_zero(d))
    return mat2_identity();
  float inv = 1.0f / d;
  return mat2_make(m.m[3] * inv, -m.m[1] * inv, -m.m[2] * inv, m.m[0] * inv);
}

X_MATH_API Mat3 mat3_identity(void)
{
  Mat3 M = { { 1, 0, 0, 0, 1, 0, 0, 0, 1 } };
  return M;
}

X_MATH_API Mat3 mat3_make(float xx, float xy, float xz, float yx, float yy, float yz,
    float zx, float zy, float zz)
{
  Mat3 M = { { xx, xy, xz, yx, yy, yz, zx, zy, zz } };
  return M;
}

X_MATH_API Mat3 mat3_transpose(Mat3 a)
{
  Mat3 r = a;
  float t;
#define SW3(i, j)                                                                        \
  t = r.m[i];                                                                            \
  r.m[i] = r.m[j];                                                                       \
  r.m[j] = t
  SW3(1, 3);
  SW3(2, 6);
  SW3(5, 7);
#undef SW3
  return r;
}

X_MATH_API float mat3_det(Mat3 m)
{
  return m.m[0] * (m.m[4] * m.m[8] - m.m[5] * m.m[7])
      - m.m[3] * (m.m[1] * m.m[8] - m.m[2] * m.m[7])
      + m.m[6] * (m.m[1] * m.m[5] - m.m[2] * m.m[4]);
}

X_MATH_API Mat3 mat3_inverse(Mat3 m)
{
  float d = mat3_det(m);
  if (float_is_zero(d))
    return mat3_identity();
  float inv = 1.0f / d;
  Mat3 r = { { (m.m[4] * m.m[8] - m.m[5] * m.m[7]) * inv,
      -(m.m[1] * m.m[8] - m.m[2] * m.m[7]) * inv,
      (m.m[1] * m.m[5] - m.m[2] * m.m[4]) * inv,
      -(m.m[3] * m.m[8] - m.m[5] * m.m[6]) * inv,
      (m.m[0] * m.m[8] - m.m[2] * m.m[6]) * inv,
      -(m.m[0] * m.m[5] - m.m[2] * m.m[3]) * inv,
      (m.m[3] * m.m[7] - m.m[4] * m.m[6]) * inv,
      -(m.m[0] * m.m[7] - m.m[1] * m.m[6]) * inv,
      (m.m[0] * m.m[4] - m.m[1] * m.m[3]) * inv } };
  return mat3_transpose(r);
}

X_MATH_API Mat3 mat3_rot_x(float a)
{
  float s = sinf(a), c = cosf(a);
  return mat3_make(1, 0, 0, 0, c, s, 0, -s, c);
}

X_MATH_API Mat3 mat3_rot_y(float a)
{
  float s = sinf(a), c = cosf(a);
  return mat3_make(c, 0, -s, 0, 1, 0, s, 0, c);
}

X_MATH_API Mat3 mat3_rot_z(float a)
{
  float s = sinf(a), c = cosf(a);
  return mat3_make(c, s, 0, -s, c, 0, 0, 0, 1);
}

X_MATH_API Mat3 mat3_scale(Vec3 s) { return mat3_make(s.x, 0, 0, 0, s.y, 0, 0, 0, s.z); }

/* Access: m[col*4 + row] */
X_MATH_API Mat4 mat4_identity(void)
{
  Mat4 M = { { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 } };
  return M;
}

X_MATH_API Mat4 mat4_make(float xx, float xy, float xz, float xw, float yx, float yy,
    float yz, float yw, float zx, float zy, float zz, float zw, float wx, float wy,
    float wz, float ww)
{
  Mat4 M = { { xx, xy, xz, xw, yx, yy, yz, yw, zx, zy, zz, zw, wx, wy, wz, ww } };
  return M;
}

X_MATH_API Mat4 mat4_transpose(Mat4 a)
{
  Mat4 r = a;
  float t;
#define SW4(i, j)                                                                        \
  t = r.m[i];                                                                            \
  r.m[i] = r.m[j];                                                                       \
  r.m[j] = t
  SW4(1, 4);
  SW4(2, 8);
  SW4(3, 12);
  SW4(6, 9);
  SW4(7, 13);
  SW4(11, 14);
#undef SW4
  return r;
}

/* c = a·b (apply b then a) */
X_MATH_API Mat4 mat4_mul(Mat4 a, Mat4 b)
{
  Mat4 r = { { 0 } };
  for (int c = 0; c < 4; c++) {
    for (int r0 = 0; r0 < 4; r0++) {
      r.m[c * 4 + r0] = a.m[0 * 4 + r0] * b.m[c * 4 + 0]
          + a.m[1 * 4 + r0] * b.m[c * 4 + 1] + a.m[2 * 4 + r0] * b.m[c * 4 + 2]
          + a.m[3 * 4 + r0] * b.m[c * 4 + 3];
    }
  }
  return r;
}

X_MATH_API Vec3 mat4_mul_point(Mat4 m, Vec3 p)
{
  float x = m.m[0] * p.x + m.m[4] * p.y + m.m[8] * p.z + m.m[12];
  float y = m.m[1] * p.x + m.m[5] * p.y + m.m[9] * p.z + m.m[13];
  float z = m.m[2] * p.x + m.m[6] * p.y + m.m[10] * p.z + m.m[14];
  float w = m.m[3] * p.x + m.m[7] * p.y + m.m[11] * p.z + m.m[15];
  if (!float_is_zero(w)) {
    x /= w;
    y /= w;
    z /= w;
  }
  return vec3_make(x, y, z);
}

X_MATH_API Vec3 mat4_mul_dir(Mat4 m, Vec3 v)
{
  return vec3_make(m.m[0] * v.x + m.m[4] * v.y + m.m[8] * v.z,
      m.m[1] * v.x + m.m[5] * v.y + m.m[9] * v.z,
      m.m[2] * v.x + m.m[6] * v.y + m.m[10] * v.z);
}

X_MATH_API Mat4 mat4_from_quat(Quat q)
{
  q = quat_norm(q);
  float x = q.x, y = q.y, z = q.z, w = q.w;
  float xx = x * x, yy = y * y, zz = z * z, xy = x * y, xz = x * z, yz = y * z,
        wx = w * x, wy = w * y, wz = w * z;
  return mat4_make(1 - 2 * (yy + zz), 2 * (xy + wz), 2 * (xz - wy), 0, 2 * (xy - wz),
      1 - 2 * (xx + zz), 2 * (yz + wx), 0, 2 * (xz + wy), 2 * (yz - wx),
      1 - 2 * (xx + yy), 0, 0, 0, 0, 1);
}

X_MATH_API Mat4 mat4_translate(Vec3 t)
{
  Mat4 M = mat4_identity();
  M.m[12] = t.x;
  M.m[13] = t.y;
  M.m[14] = t.z;
  return M;
}

X_MATH_API Mat4 mat4_scale(Vec3 s)
{
  Mat4 M = { { s.x, 0, 0, 0, 0, s.y, 0, 0, 0, 0, s.z, 0, 0, 0, 0, 1 } };
  return M;
}

X_MATH_API Mat4 mat4_rot_x(float a)
{
  float s = sinf(a);
  float c = cosf(a);
  return mat4_make(1, 0, 0, 0, 0, c, s, 0, 0, -s, c, 0, 0, 0, 0, 1);
}

X_MATH_API Mat4 mat4_rot_y(float a)
{
  float s = sinf(a);
  float c = cosf(a);
  return mat4_make(c, 0, -s, 0, 0, 1, 0, 0, s, 0, c, 0, 0, 0, 0, 1);
}

X_MATH_API Mat4 mat4_rot_z(float a)
{
  float s = sinf(a);
  float c = cosf(a);
  return mat4_make(c, s, 0, 0, -s, c, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
}

X_MATH_API Mat4 mat4_from_axis_angle(Vec3 axis, float angle)
{
  return mat4_from_quat(quat_axis_angle(axis, angle));
}

X_MATH_API Mat4 mat4_from_euler(float rx, float ry, float rz)
{
  return mat4_mul(mat4_mul(mat4_rot_z(rz), mat4_rot_y(ry)), mat4_rot_x(rx));
}

X_MATH_API Mat4 mat4_look_at_rh(Vec3 eye, Vec3 target, Vec3 up)
{
  Vec3 f = vec3_norm(vec3_sub(target, eye));
  Vec3 s = vec3_norm(vec3_cross(f, up));
  Vec3 u = vec3_cross(s, f);
  Mat4 M = mat4_identity();
  M.m[0] = s.x;
  M.m[4] = s.y;
  M.m[8] = s.z;
  M.m[1] = u.x;
  M.m[5] = u.y;
  M.m[9] = u.z;
  M.m[2] = -f.x;
  M.m[6] = -f.y;
  M.m[10] = -f.z;
  M.m[12] = -(s.x * eye.x + s.y * eye.y + s.z * eye.z);
  M.m[13] = -(u.x * eye.x + u.y * eye.y + u.z * eye.z);
  M.m[14] = (f.x * eye.x + f.y * eye.y + f.z * eye.z);
  return M;
}

X_MATH_API Mat4 mat4_perspective_rh_no(float fovy, float aspect, float n, float f)
{
  float y = 1.0f / tanf(0.5f * fovy), x = y / aspect;
  return mat4_make(x, 0, 0, 0, 0, y, 0, 0, 0, 0, -(f + n) / (f - n), -1, 0, 0,
      -(2.0f * f * n) / (f - n), 0);
}

X_MATH_API Mat4 mat4_perspective_rh_zo(float fovy, float aspect, float n, float f)
{
  float y = 1.0f / tanf(0.5f * fovy), x = y / aspect;
  return mat4_make(
      x, 0, 0, 0, 0, y, 0, 0, 0, 0, f / (n - f), -1, 0, 0, (f * n) / (n - f), 0);
}

X_MATH_API Mat4 mat4_orthographic_rh_no(
    float l, float r, float b, float t, float n, float f)
{
  return mat4_make(2.0f / (r - l), 0, 0, 0, 0, 2.0f / (t - b), 0, 0, 0, 0,
      -2.0f / (f - n), 0, -(r + l) / (r - l), -(t + b) / (t - b), -(f + n) / (f - n), 1);
}

X_MATH_API Mat4 mat4_look_at_lh(Vec3 eye, Vec3 target, Vec3 up)
{
  Vec3 f = vec3_norm(vec3_sub(target, eye));
  Vec3 s = vec3_norm(vec3_cross(up, f)); /* note a ordem p/ LH */
  Vec3 u = vec3_cross(f, s);
  Mat4 M = mat4_identity();
  M.m[0] = s.x;
  M.m[4] = s.y;
  M.m[8] = s.z;
  M.m[1] = u.x;
  M.m[5] = u.y;
  M.m[9] = u.z;
  M.m[2] = f.x;
  M.m[6] = f.y;
  M.m[10] = f.z;
  M.m[12] = -(s.x * eye.x + s.y * eye.y + s.z * eye.z);
  M.m[13] = -(u.x * eye.x + u.y * eye.y + u.z * eye.z);
  M.m[14] = -(f.x * eye.x + f.y * eye.y + f.z * eye.z);
  return M;
}

X_MATH_API Mat4 mat4_perspective_lh_no(float fovy, float aspect, float n, float f)
{
  float y = 1.0f / tanf(0.5f * fovy), x = y / aspect;
  /* nota: signs of m[10], m[11], m[14] are different than RH_NO */
  return mat4_make(x, 0, 0, 0, 0, y, 0, 0, 0, 0, (f + n) / (f - n), 1, 0, 0,
      (-2.0f * f * n) / (f - n), 0);
}

X_MATH_API Mat4 mat4_perspective_lh_zo(float fovy, float aspect, float n, float f)
{
  float y = 1.0f / tanf(0.5f * fovy), x = y / aspect;
  return mat4_make(
      x, 0, 0, 0, 0, y, 0, 0, 0, 0, f / (f - n), 1, 0, 0, (-n * f) / (f - n), 0);
}

X_MATH_API Mat4 mat4_orthographic_lh_no(
    float l, float r, float b, float t, float n, float f)
{
  return mat4_make(2.0f / (r - l), 0, 0, 0, 0, 2.0f / (t - b), 0, 0, 0, 0, 2.0f / (f - n),
      0, -(r + l) / (r - l), -(t + b) / (t - b), -(f + n) / (f - n), 1);
}

X_MATH_API Mat4 mat4_orthographic_rh_zo(
    float l, float r, float b, float t, float n, float f)
{
  /* RH_ZO: Z' = (f - z)/(f - n)  → scale +1/(f-n), trans −n/(f-n) signed RH */
  return mat4_make(2.0f / (r - l), 0, 0, 0, 0, 2.0f / (t - b), 0, 0, 0, 0,
      -1.0f / (f - n), 0, -(r + l) / (r - l), -(t + b) / (t - b), -n / (f - n), 1);
}

X_MATH_API Mat4 mat4_orthographic_lh_zo(
    float l, float r, float b, float t, float n, float f)
{
  /* LH_ZO: Z' = (z - n)/(f - n)  → scale +1/(f-n), trans −n/(f-n) signed LH */
  return mat4_make(2.0f / (r - l), 0, 0, 0, 0, 2.0f / (t - b), 0, 0, 0, 0, 1.0f / (f - n),
      0, -(r + l) / (r - l), -(t + b) / (t - b), -n / (f - n), 1);
}

X_MATH_API static float s_m3det(float a00, float a01, float a02, float a10, float a11,
    float a12, float a20, float a21, float a22)
{
  return a00 * (a11 * a22 - a12 * a21) - a01 * (a10 * a22 - a12 * a20)
      + a02 * (a10 * a21 - a11 * a20);
}

X_MATH_API float mat4_det(Mat4 m)
{
  return m.m[0] * mat4_cofactor(m, 0, 0) + m.m[4] * mat4_cofactor(m, 0, 1)
      + m.m[8] * mat4_cofactor(m, 0, 2) + m.m[12] * mat4_cofactor(m, 0, 3);
}

X_MATH_API static float s_m4minor_det(Mat4 m, int row, int col)
{
  float a[9];
  int k = 0;
  int c;
  int r;

  for (c = 0; c < 4; ++c) {
    if (c == col) {
      continue;
    }

    for (r = 0; r < 4; ++r) {
      if (r == row) {
        continue;
      }

      a[k++] = m.m[c * 4 + r];
    }
  }

  return s_m3det(a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8]);
}

X_MATH_API float mat4_minor(Mat4 m, int row, int col)
{
  if (row < 0 || row > 3 || col < 0 || col > 3) {
    return 0.0f;
  }

  return s_m4minor_det(m, row, col);
}

X_MATH_API float mat4_cofactor(Mat4 m, int row, int col)
{
  float minor;

  if (row < 0 || row > 3 || col < 0 || col > 3) {
    return 0.0f;
  }

  minor = s_m4minor_det(m, row, col);
  return (((row + col) & 1) != 0) ? -minor : minor;
}

X_MATH_API Mat4 mat4_inverse_affine_uniform_scale(Mat4 m)
{
  Vec3 X = vec3_make(m.m[0], m.m[1], m.m[2]);
  Vec3 Y = vec3_make(m.m[4], m.m[5], m.m[6]);
  Vec3 Z = vec3_make(m.m[8], m.m[9], m.m[10]);
  Vec3 t = vec3_make(m.m[12], m.m[13], m.m[14]);

  float sx = vec3_len(X);
  float sy = vec3_len(Y);
  float sz = vec3_len(Z);

  if (sx <= STDXM_EPS || sy <= STDXM_EPS || sz <= STDXM_EPS) {
    return mat4_identity();
  }

  /* For uniform scale these should be approximately equal. */
  {
    float s = sx;
    Vec3 x = vec3_div(X, s);
    Vec3 y = vec3_div(Y, s);
    Vec3 z = vec3_div(Z, s);
    float invs = 1.0f / s;
    Mat4 M = mat4_identity();
    Vec3 tt;

    M.m[0] = x.x * invs;
    M.m[1] = y.x * invs;
    M.m[2] = z.x * invs;

    M.m[4] = x.y * invs;
    M.m[5] = y.y * invs;
    M.m[6] = z.y * invs;

    M.m[8] = x.z * invs;
    M.m[9] = y.z * invs;
    M.m[10] = z.z * invs;

    tt = vec3_make(-(M.m[0] * t.x + M.m[4] * t.y + M.m[8] * t.z),
        -(M.m[1] * t.x + M.m[5] * t.y + M.m[9] * t.z),
        -(M.m[2] * t.x + M.m[6] * t.y + M.m[10] * t.z));

    M.m[12] = tt.x;
    M.m[13] = tt.y;
    M.m[14] = tt.z;
    return M;
  }
}

X_MATH_API Mat4 mat4_inverse_affine(Mat4 m)
{
  Vec3 X = (Vec3) { m.m[0], m.m[1], m.m[2] };
  Vec3 Y = (Vec3) { m.m[4], m.m[5], m.m[6] };
  Vec3 Z = (Vec3) { m.m[8], m.m[9], m.m[10] };
  Vec3 t = (Vec3) { m.m[12], m.m[13], m.m[14] };

  float sx = vec3_len(X);
  float sy = vec3_len(Y);
  float sz = vec3_len(Z);

  if (sx <= STDXM_EPS || sy <= STDXM_EPS || sz <= STDXM_EPS) {
    return mat4_identity();
  }

  Vec3 x = vec3_div(X, sx);
  Vec3 y = vec3_div(Y, sy);
  Vec3 z = vec3_div(Z, sz);

  float det = x.x * (y.y * z.z - y.z * z.y) - x.y * (y.x * z.z - y.z * z.x)
      + x.z * (y.x * z.y - y.y * z.x);

  if (det < 0.0f) {
    if (fabsf(sx) >= fabsf(sy) && fabsf(sx) >= fabsf(sz)) {
      sx = -sx;
      x = vec3_neg(x);
    } else if (fabsf(sy) >= fabsf(sx) && fabsf(sy) >= fabsf(sz)) {
      sy = -sy;
      y = vec3_neg(y);
    } else {
      sz = -sz;
      z = vec3_neg(z);
    }
  }

  Mat4 inv = mat4_identity();

  inv.m[0] = x.x / sx;
  inv.m[1] = y.x / sy;
  inv.m[2] = z.x / sz;

  inv.m[4] = x.y / sx;
  inv.m[5] = y.y / sy;
  inv.m[6] = z.y / sz;

  inv.m[8] = x.z / sx;
  inv.m[9] = y.z / sy;
  inv.m[10] = z.z / sz;

  Vec3 tt = (Vec3) { -(inv.m[0] * t.x + inv.m[4] * t.y + inv.m[8] * t.z),
    -(inv.m[1] * t.x + inv.m[5] * t.y + inv.m[9] * t.z),
    -(inv.m[2] * t.x + inv.m[6] * t.y + inv.m[10] * t.z) };

  inv.m[12] = tt.x;
  inv.m[13] = tt.y;
  inv.m[14] = tt.z;

  return inv;
}

X_MATH_API Quat quat_id(void) { return (Quat) { 0, 0, 0, 1 }; }

X_MATH_API Quat quat_make(float x, float y, float z, float w)
{
  return (Quat) { x, y, z, w };
}

X_MATH_API Quat quat_norm(Quat a)
{
  float L = sqrtf(a.x * a.x + a.y * a.y + a.z * a.z + a.w * a.w);
  return (L > STDXM_EPS) ? quat_make(a.x / L, a.y / L, a.z / L, a.w / L) : quat_id();
}

X_MATH_API Quat quat_conjugate(Quat q) { return quat_make(-q.x, -q.y, -q.z, q.w); }

X_MATH_API Quat quat_inverse(Quat q)
{
  float n = q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w;
  if (n <= STDXM_EPS)
    return quat_id();
  float inv = 1.0f / n;
  return quat_make(-q.x * inv, -q.y * inv, -q.z * inv, q.w * inv);
}

X_MATH_API Quat quat_unit_inverse(Quat q) { return quat_conjugate(quat_norm(q)); }

X_MATH_API Quat quat_mul(Quat a, Quat b)
{
  return quat_make(a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
      a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
      a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
      a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z);
}

X_MATH_API Vec3 quat_mul_vec3(Quat q, Vec3 v)
{ /* rotate v by q */
  Vec3 u = vec3_make(q.x, q.y, q.z);
  Vec3 t = vec3_mul(vec3_cross(u, v), 2.0f);
  return vec3_add(v, vec3_add(vec3_mul(t, q.w), vec3_cross(u, t)));
}

X_MATH_API Quat quat_axis_angle(Vec3 axis, float angle)
{
  axis = vec3_norm(axis);
  float s = sinf(0.5f * angle);
  return quat_make(axis.x * s, axis.y * s, axis.z * s, cosf(0.5f * angle));
}

X_MATH_API Quat quat_from_to(Vec3 from, Vec3 to)
{
  Vec3 f = vec3_norm(from), t = vec3_norm(to);
  float d = float_clamp(vec3_dot(f, t), -1.0f, 1.0f);
  if (d > 1.0f - 1e-6f)
    return quat_id();
  if (d < -1.0f + 1e-6f) {
    Vec3 ax = vec3_cross(vec3_make(1, 0, 0), f);
    if (vec3_len2(ax) < 1e-8f)
      ax = vec3_cross(vec3_make(0, 1, 0), f);
    return quat_axis_angle(vec3_norm(ax), STDXM_PI);
  }
  return quat_axis_angle(vec3_norm(vec3_cross(f, t)), acosf(d));
}

X_MATH_API Quat quat_slerp(Quat a, Quat b, float t)
{
  float dot = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
  if (dot < 0) {
    dot = -dot;
    b = quat_make(-b.x, -b.y, -b.z, -b.w);
  }
  if (1.0f - dot < 1e-6f) {
    Quat r = quat_make(a.x + t * (b.x - a.x), a.y + t * (b.y - a.y),
        a.z + t * (b.z - a.z), a.w + t * (b.w - a.w));
    return quat_norm(r);
  }
  float w = acosf(float_clamp(dot, -1.0f, 1.0f)), s = sinf(w);
  float s0 = sinf((1.0f - t) * w) / s, s1 = sinf(t * w) / s;
  return quat_norm(quat_make(a.x * s0 + b.x * s1, a.y * s0 + b.y * s1,
      a.z * s0 + b.z * s1, a.w * s0 + b.w * s1));
}

/* column-major Mat3 -> scalar entries (m[col*3 + row]) */
X_MATH_API static inline Quat quat_from_mat3(Mat3 R)
{
  /* correct loads for column-major: */
  float m00 = R.m[0], m01 = R.m[3], m02 = R.m[6];
  float m10 = R.m[1], m11 = R.m[4], m12 = R.m[7];
  float m20 = R.m[2], m21 = R.m[5], m22 = R.m[8];

  float trace = m00 + m11 + m22;
  Quat q;
  if (trace > 0.0f) {
    float s = sqrtf(trace + 1.0f) * 2.0f; /* s = 4*w */
    q.w = 0.25f * s;
    q.x = (m21 - m12) / s;
    q.y = (m02 - m20) / s;
    q.z = (m10 - m01) / s;
  } else if (m00 > m11 && m00 > m22) {
    float s = sqrtf(1.0f + m00 - m11 - m22) * 2.0f; /* s = 4*x */
    q.w = (m21 - m12) / s;
    q.x = 0.25f * s;
    q.y = (m01 + m10) / s;
    q.z = (m02 + m20) / s;
  } else if (m11 > m22) {
    float s = sqrtf(1.0f + m11 - m00 - m22) * 2.0f; /* s = 4*y */
    q.w = (m02 - m20) / s;
    q.x = (m01 + m10) / s;
    q.y = 0.25f * s;
    q.z = (m12 + m21) / s;
  } else {
    float s = sqrtf(1.0f + m22 - m00 - m11) * 2.0f; /* s = 4*z */
    q.w = (m10 - m01) / s;
    q.x = (m02 + m20) / s;
    q.y = (m12 + m21) / s;
    q.z = 0.25f * s;
  }
  return quat_norm(q);
}

/* replace your current mat4_decompose with this */
X_MATH_API void mat4_decompose(Mat4 m, Vec3* out_t, Quat* out_r, Vec3* out_s)
{
  if (out_t) {
    *out_t = (Vec3) { m.m[12], m.m[13], m.m[14] };
  }

  /* basis columns (col-major) */
  Vec3 X = (Vec3) { m.m[0], m.m[1], m.m[2] };
  Vec3 Y = (Vec3) { m.m[4], m.m[5], m.m[6] };
  Vec3 Z = (Vec3) { m.m[8], m.m[9], m.m[10] };

  float sx = vec3_len(X), sy = vec3_len(Y), sz = vec3_len(Z);

  /* degenerate guard */
  if (sx <= STDXM_EPS || sy <= STDXM_EPS || sz <= STDXM_EPS) {
    if (out_s)
      *out_s = (Vec3) { 1, 1, 1 };
    if (out_r)
      *out_r = quat_id();
    return;
  }

  /* normalize to get rotation columns */
  Vec3 x = vec3_div(X, sx);
  Vec3 y = vec3_div(Y, sy);
  Vec3 z = vec3_div(Z, sz);

  /* ensure proper rotation (det>0). If det<0, flip ONE axis and its scale sign */
  float det = x.x * (y.y * z.z - y.z * z.y) - x.y * (y.x * z.z - y.z * z.x)
      + x.z * (y.x * z.y - y.y * z.x);

  if (det < 0.0f) {
    /* flip largest-magnitude axis for stability */
    if (fabsf(sx) >= fabsf(sy) && fabsf(sx) >= fabsf(sz)) {
      sx = -sx;
      x = vec3_neg(x);
    } else if (fabsf(sy) >= fabsf(sx) && fabsf(sy) >= fabsf(sz)) {
      sy = -sy;
      y = vec3_neg(y);
    } else {
      sz = -sz;
      z = vec3_neg(z);
    }
  }

  if (out_s)
    *out_s = (Vec3) { sx, sy, sz };

  if (out_r) {
    Mat3 R = (Mat3) { { x.x, x.y, x.z, y.x, y.y, y.z, z.x, z.y, z.z } };
    *out_r = quat_from_mat3(R);
  }
}

X_MATH_API Quat quat_neg(Quat q) { return quat_make(-q.x, -q.y, -q.z, -q.w); }

X_MATH_API Quat quat_scale(Quat q, float s)
{
  return quat_make(q.x * s, q.y * s, q.z * s, q.w * s);
}

X_MATH_API Vec3 quat_get_right(Quat q) { return quat_mul_vec3(q, vec3_make(1, 0, 0)); }

X_MATH_API Vec3 quat_get_up(Quat q) { return quat_mul_vec3(q, vec3_make(0, 1, 0)); }

X_MATH_API Vec3 quat_get_forward(Quat q) { return quat_mul_vec3(q, vec3_make(0, 0, 1)); }

X_MATH_API Quat quat_log(Quat q)
{
  Quat n = quat_norm(q);
  float vlen = sqrtf(n.x * n.x + n.y * n.y + n.z * n.z);
  if (vlen < STDXM_EPS)
    return quat_make(0, 0, 0, 0.0f);
  float a = acosf(float_clamp(n.w, -1.0f, 1.0f));
  float s = a / vlen;
  return quat_make(n.x * s, n.y * s, n.z * s, 0.0f);
}

X_MATH_API Quat quat_exp(Quat w)
{
  float vlen = sqrtf(w.x * w.x + w.y * w.y + w.z * w.z);
  float s = (vlen > STDXM_EPS) ? sinf(vlen) / vlen : 1.0f;
  return quat_make(w.x * s, w.y * s, w.z * s, cosf(vlen));
}

X_MATH_API Quat quat_constrain(Quat q, Vec3 axis)
{
  Vec3 f = quat_get_forward(q);
  Vec3 p = vec3_project(f, axis);
  Vec3 ortho = vec3_norm(vec3_sub(f, p));
  return quat_from_to(vec3_make(0, 0, 1), ortho);
}

X_MATH_API Quat quat_constrain_y(Quat q) { return quat_constrain(q, vec3_make(0, 1, 0)); }

X_MATH_API Quat quat_interpolate(const Quat* qs, const float* ws, int count)
{
  /* barycentric on the sphere via exp/log */
  Quat acc = (Quat) { 0, 0, 0, 0 };
  for (int i = 0; i < count; i++) {
    Quat wlog = quat_log(qs[i]);
    acc.x += ws[i] * wlog.x;
    acc.y += ws[i] * wlog.y;
    acc.z += ws[i] * wlog.z;
  }
  return quat_norm(quat_exp(acc));
}

X_MATH_API QuatDual quatdual_id(void)
{
  return (QuatDual) { quat_id(), (Quat) { 0, 0, 0, 0 } };
}

X_MATH_API QuatDual quatdual_make(Quat real, Quat dual)
{
  return (QuatDual) { real, dual };
}

X_MATH_API QuatDual quatdual_from_rt(Quat r, Vec3 t)
{
  Quat d = quat_scale(quat_mul((Quat) { t.x, t.y, t.z, 0 }, r), 0.5f);
  return quatdual_make(r, d);
}

X_MATH_API QuatDual quatdual_mul(QuatDual a, QuatDual b)
{
  return quatdual_make(quat_mul(a.real, b.real),
      quat_add(quat_mul(a.real, b.dual), quat_mul(a.dual, b.real)));
}

X_MATH_API Quat quat_add(Quat a, Quat b)
{
  return quat_make(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
}

X_MATH_API QuatDual quatdual_norm(QuatDual qd)
{
  float n = sqrtf(qd.real.x * qd.real.x + qd.real.y * qd.real.y + qd.real.z * qd.real.z
      + qd.real.w * qd.real.w);
  if (n <= STDXM_EPS)
    return quatdual_id();
  float inv = 1.0f / n;
  return quatdual_make(
      quat_make(qd.real.x * inv, qd.real.y * inv, qd.real.z * inv, qd.real.w * inv),
      quat_make(qd.dual.x * inv, qd.dual.y * inv, qd.dual.z * inv, qd.dual.w * inv));
}

X_MATH_API Vec3 quatdual_mul_vec3_rot(QuatDual qd, Vec3 v)
{
  return quat_mul_vec3(qd.real, v);
}

X_MATH_API Vec3 quatdual_mul_vec3(QuatDual qd, Vec3 v)
{
  /* p' = r * (0,v) * r^{-1} + 2*(qd.dual * r^{-1}).xyz */
  Quat rinv = quat_unit_inverse(qd.real);
  Vec3 rv = quat_mul_vec3(qd.real, v);
  Quat trans = quat_mul(quat_scale(qd.dual, 2.0f), rinv);
  return vec3_add(rv, vec3_make(trans.x, trans.y, trans.z));
}

X_MATH_API Mat4 mat4_compose(Vec3 t, Quat r, Vec3 s)
{
  return mat4_mul(mat4_mul(mat4_translate(t), mat4_from_quat(r)), mat4_scale(s));
}

X_MATH_API Vec3 quat_to_euler_xyz(Quat q)
{
  Vec3 e;
  float sinr_cosp = 2.0f * (q.w * q.x + q.y * q.z);
  float cosr_cosp = 1.0f - 2.0f * (q.x * q.x + q.y * q.y);
  e.x = atan2f(sinr_cosp, cosr_cosp);

  float sinp = 2.0f * (q.w * q.y - q.z * q.x);
  e.y = (fabsf(sinp) >= 1.0f) ? copysignf(STDXM_PI * 0.5f, sinp) : asinf(sinp);

  float siny_cosp = 2.0f * (q.w * q.z + q.x * q.y);
  float cosy_cosp = 1.0f - 2.0f * (q.y * q.y + q.z * q.z);
  e.z = atan2f(siny_cosp, cosy_cosp);
  return e;
}

X_MATH_API Mat4 mat4_inverse_full(Mat4 m, bool* ok)
{
  Mat4 r = { 0 };
  float det;
  float invdet;
  int row;
  int col;

  det = mat4_det(m);

  if (ok) {
    *ok = fabsf(det) > STDXM_EPS;
  }

  if (fabsf(det) <= STDXM_EPS) {
    return mat4_identity();
  }

  invdet = 1.0f / det;

  for (row = 0; row < 4; ++row) {
    for (col = 0; col < 4; ++col) {
      r.m[col * 4 + row] = mat4_cofactor(m, col, row) * invdet;
    }
  }

  return r;
}

#endif // X_IMPL_MATH
#endif // X_MATH_H
