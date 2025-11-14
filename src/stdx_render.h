#ifndef X_RENDER_H
#define X_RENDER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <stdx_math.h>
#include <stdx_log.h>

  // ============================================================================
  // Basic Types 
  // ============================================================================
  typedef struct { float r, g, b, a } Color;
  typedef struct { float x, y, w, h } Rect;

  // Default Vertex layout (PNUV: Position, Normal, UV)
  typedef struct {
    float px, py, pz;  // position
    float nx, ny, nz;  // normal
    float u,  v;       // texcoord
  } Vertex_PNUV;

  // ============================================================================
  // Opaque Objects
  // ============================================================================
  typedef struct RendererContext RendererContext;
  typedef struct RenderTarget    RenderTarget;
  typedef struct RenderPass      RenderPass;
  typedef struct Mesh            Mesh;
  typedef struct Texture         Texture;
  typedef struct Material        Material;
  typedef struct DebugDraw       DebugDraw;   // batch de debug 3D (linhas/quads)

  // ============================================================================
  // Renderer Initialization
  // ============================================================================
  typedef enum {
    DEPTH_RANGE_NEGATIVE_ONE_TO_ONE, // NDC −1..1 (OpenGL)
    DEPTH_RANGE_ZERO_TO_ONE          // NDC  0..1 (Vulkan/D3D)
  } DepthRange;

  typedef struct {
    int width;
    int height;
    DepthRange depth_range;
  } RendererDesc;

  RendererContext* renderer_create(const RendererDesc* desc);
  void             renderer_destroy(RendererContext* context);
  const char*      renderer_last_error(const RendererContext* context);

  // O backbuffer (window) exposed as a “virtual” RenderTarget 
  RenderTarget*    renderer_backbuffer(RendererContext* context);

  // ============================================================================
  // Render targets
  // ============================================================================
  typedef enum { COLOR_FORMAT_RGBA8, COLOR_FORMAT_RGBA16F } ColorFormat;
  typedef enum { DEPTH_FORMAT_NONE,  DEPTH_FORMAT_D24S8   } DepthFormat;

  typedef struct {
    int         width;
    int         height;
    ColorFormat color_format;
    DepthFormat depth_format;      // NONE for color only targets
    bool        generate_mipmaps;  // 
    bool        sampled;           // Will be read by another shader/pass ?
  } RenderTargetDesc;

  RenderTarget* render_target_create(RendererContext* context, const RenderTargetDesc* desc);
  void          render_target_destroy(RendererContext* context, RenderTarget* target);
  Texture*      render_target_get_texture(RenderTarget* target); // Chain passes

  // ============================================================================
  // Resources: Mesh and Texture
  // ============================================================================
  typedef struct {
    const void*     vertices;       // interleaved
    uint32_t        vertex_count;
    uint32_t        vertex_stride;  // bytes per vertex (ex.: sizeof(Vertex_PNUV))
    const uint32_t* indices;        // Optional
    uint32_t        index_count;
  } MeshDesc;

  Mesh* mesh_create(RendererContext* context, const MeshDesc* desc);
  void  mesh_destroy(RendererContext* context, Mesh* mesh);

  typedef struct {
    const void* pixels_rgba8; // RGBA8 contiguos
    int         width;
    int         height;
    bool        generate_mipmaps;
  } TextureDesc;

  Texture* texture_create(RendererContext* context, const TextureDesc* desc);
  void     texture_destroy(RendererContext* context, Texture* texture);

  // ============================================================================
  // Materials: Fixed shader families
  // ============================================================================
  typedef enum {
    MATERIAL_KIND_UNLIT,           // cor/texture no light
    MATERIAL_KIND_LIT_LAMBERT,     // difuse (normal + simple lighting)
    MATERIAL_KIND_SPRITE,          // UI/2D (optional pre-multiplied)
    MATERIAL_KIND_POSTPROCESS,     // full-screen (uses 1..4 textures as input)
    MATERIAL_KIND_COLORED_UNLIT    // Vertex color (debug/lines/quads)
  } MaterialKind;

  typedef struct {
    MaterialKind kind;

    // Shader sources (NULL → default source for that kind)
    const char*  vertex_shader_source;
    const char*  fragment_shader_source;

    // Main textures (can be null depending on the shader kind)
    Texture*     main_texture;      // base color / sprite
    Texture*     normal_texture;    // for LIT_LAMBERT
    Texture*     secondary_texture; // post-process (bloom/compose etc.)

    // State flags
    bool         depth_test;
    bool         depth_write;
    bool         alpha_blend;

    // Post-process: how many inputes does the FS expets 
    int          expected_inputs;
  } MaterialDesc;

  Material* material_create(RendererContext* context, const MaterialDesc* desc);
  void      material_destroy(RendererContext* context, Material* material);

  // ============================================================================
  // Lights
  // ============================================================================
  typedef enum { LIGHT_DIRECTIONAL, LIGHT_POINT } LightType;

  typedef struct {
    LightType type;
    Vec3      direction_or_position; // Unit if directional, position if point.
    Color     color;                  // optional intencity
    float     range;                  // for point light
  } Light;

  typedef struct {
    const Light* lights;     // array
    int          light_count; // p.ex. max 4 no v1
  } Lighting;

  // ============================================================================
  // Render passes
  // ============================================================================
  typedef enum { PASS_LOAD_CLEAR, PASS_LOAD_KEEP } PassLoadOp;

  typedef struct {
    RenderTarget* color_target; // mandatory
    RenderTarget* depth_target; // NULL → uses the depth of target if present
    PassLoadOp    load_op;      // CLEAR ou KEEP
  } PassDesc;

  // Parâmetros por quadro (3D)
  typedef struct {
    Mat4  view_projection;
    Color clear_color;   // used if load_op == CLEAR
    float clear_depth;   // tipicamente 1.0
  } Frame3D;

  // Per draw parameters (3D)
  typedef struct {
    Mat4  model;
    Color base_tint;     // multiplier RGBA
  } Draw3D;

  RenderPass* pass_begin(RendererContext* context,
      const PassDesc*   desc,
      const Frame3D*    frame_3d,   // can be NULL (post-process)
      const Lighting*   lighting);  // can be NULL

  // Draw meshes using selected material layout
  void pass_draw_mesh(RendererContext* context,
      RenderPass*      pass,
      const Mesh*      mesh,
      const Material*  material,
      const Draw3D*    draw);

  // Fullscreen (post-process / composition)
  void pass_draw_fullscreen(RendererContext* context,
      RenderPass*      pass,
      const Material*  postprocess_material,
      Texture* const*  input_textures, int input_count);

  void pass_end(RendererContext* context, RenderPass* pass);

  // ============================================================================
  // UI / 2D (Screen space/blend)
  // ============================================================================
  typedef struct {
    Mat4        projection;     // Ortho 0..w/0..h by default; identity if not set.
    PassLoadOp  load_op;        // CLEAR ou KEEP
    Color       clear_color;    // se CLEAR
  } Frame2D;

  typedef struct {
    Texture* texture;           // spritesheet; NULL → sólido
    Color    tint;
    Vec2     position;          // pixels
    Vec2     size;              // pixels
    float    rotation_radians;
    Vec2     anchor;            // 0..1 (ex.: {0.5,0.5} centro)
    bool     premultiplied_alpha;
  } Sprite;

  void ui_begin(RendererContext* context,
      RenderTarget*    color_target,
      const Frame2D*   frame_2d);

  void ui_draw_sprite(RendererContext* context, const Sprite* sprite);

  // Colored 2D primitives (thickness in pixels)
  void ui_draw_quad_filled(RendererContext* context, Rect rect, Color color);
  void ui_draw_quad_outline(RendererContext* context, Rect rect, float thickness_px, Color color);
  void ui_draw_line(RendererContext* context, Vec2 p0, Vec2 p1, float thickness_px, Color color);

  void ui_end(RendererContext* context);

  // ============================================================================
  // Debug 3D (gizmos, lines/colored quads) in world space
  // ============================================================================
  DebugDraw* debug_begin(RendererContext* context,
      RenderPass*      pass,
      const Frame3D*   frame_3d);

  // lines in world space. 
  void debug_line(DebugDraw* dbg,
      Vec3   a, Vec3 b,
      Color  color,
      float  thickness_world);

  void debug_quad (DebugDraw* dbg, Mat4 model, Color color); // dois triângulos no plano XY
  void debug_axes (DebugDraw* dbg, Mat4 transform, float length);
  void debug_aabb (DebugDraw* dbg, Vec3 minp, Vec3 maxp, Color color);
  void debug_end(DebugDraw* dbg);

#ifdef __cplusplus
} // extern "C"
#endif
#endif //X_RENDER_H
