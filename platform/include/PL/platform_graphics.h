/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>
*/

#pragma once

#include "platform.h"
#include "platform_math.h"
#include "platform_image.h"

#define PL_MODE_OPENGL

typedef enum PLGfxMode {
    PLGFX_MODE_SOFTWARE,

    PLGFX_MODE_OPENGL,
    PLGFX_MODE_OPENGL_CORE,
    PLGFX_MODE_OPENGL_ES,

    PLGFX_MODE_VULKAN,

    PLGFX_MODE_GLIDE,

    PLGFX_MODE_DIRECT3D,

    PLGFX_MODE_OTHER,
} PLGfxMode;

// todo, move these into platform_graphics.cpp
#if defined (PL_MODE_OPENGL)

#   ifdef __APPLE__
#       if 0
#           include <OpenGL/gl3.h>
#           include <OpenGL/gl3ext.h>

#           define VL_MODE_OPENGL_CORE
#           define PL_MODE_OPENGL_CORE

#       else
#           include <OpenGL/gl.h>
#           include <OpenGL/glext.h>
#           include <OpenGL/glu.h>

#           include <OpenGL/gl3.h>
#           include <OpenGL/gl3ext.h>
#       endif
#   else
//#	    include <GL/glew.h>
#       if !defined(PL_MODE_OPENGL_CORE)
#           define GL_GLEXT_PROTOTYPES
#           include <GL/gl.h>
#           include <GL/glcorearb.h>
#           include <GL/glext.h>
#       else
#           include <GL/glcorearb.h>
#       endif
#   endif

#	ifdef _WIN32
#		include <GL/wglew.h>
#	endif
#elif defined (VL_MODE_OPENGL_CORE)

#	include <GL/glcorearb.h>

#elif defined (VL_MODE_GLIDE)

#	ifdef _MSC_VER
#		define __MSC__
#	endif

#	include <glide.h>

#elif defined (VL_MODE_DIRECT3D)

#	include <d3d11.h>

#	pragma comment (lib, "d3d11.lib")
#	pragma comment (lib, "d3dx11.lib")
#	pragma comment (lib, "d3dx10.lib")

#elif defined (VL_MODE_VULKAN)

#else

#   define PLGRAPHICS_SW_MAX_TEXTUREUNITS  16

#endif

#if defined(PL_INTERNAL)
#   include "platform_log.h"
#   define PL_GRAPHICS_LOG  "pl_graphics"
#   ifdef _DEBUG
#	    define plGraphicsLog(...) plWriteLog(PL_GRAPHICS_LOG, __VA_ARGS__)
#   else
#	    define plGraphicsLog(...)
#   endif
#endif

typedef unsigned int PLVertexArray;
typedef unsigned int PLRenderBuffer;

typedef void PLGraphicsContext;

typedef struct PLFrameBuffer {
#if defined(PL_MODE_OPENGL)
    unsigned int fbo, rbo;
#endif
} PLFrameBuffer;

typedef enum PLDataFormat {
#if defined (PL_MODE_OPENGL)
    PL_UNSIGNED_BYTE                = GL_UNSIGNED_BYTE,
    PL_UNSIGNED_INT_8_8_8_8_REV     = GL_UNSIGNED_INT_8_8_8_8_REV,
#else
    PL_UNSIGNED_BYTE,
#endif
} PLDataFormat;

typedef enum PLBufferMask {
#if defined (PL_MODE_OPENGL)
    VL_MASK_COLOUR      = GL_COLOR_BUFFER_BIT,
    VL_MASK_DEPTH       = GL_DEPTH_BUFFER_BIT,
    VL_MASK_ACCUM,
    VL_MASK_STENCIL     = GL_STENCIL_BUFFER_BIT,
#else
    VL_MASK_COLOUR		= (1 << 0),
    VL_MASK_DEPTH		= (1 << 1),
    VL_MASK_ACCUM		= (1 << 2),
    VL_MASK_STENCIL		= (1 << 3),
#endif
} PLBufferMask;

typedef enum VLCullMode {   
    PL_CULL_POSTIVE,
    PL_CULL_NEGATIVE
} PLCullMode;

// Blending Modes
typedef enum PLBlend {
#if defined (PL_MODE_OPENGL)
    PL_BLEND_ZERO                   = GL_ZERO,
    PL_BLEND_ONE                    = GL_ONE,
    VL_BLEND_SRC_COLOR              = GL_SRC_COLOR,
    VL_BLEND_ONE_MINUS_SRC_COLOR    = GL_ONE_MINUS_SRC_COLOR,
    VL_BLEND_SRC_ALPHA              = GL_SRC_ALPHA,
    VL_BLEND_ONE_MINUS_SRC_ALPHA    = GL_ONE_MINUS_SRC_ALPHA,
    VL_BLEND_DST_ALPHA              = GL_DST_ALPHA,
    VL_BLEND_ONE_MINUS_DST_ALPHA    = GL_ONE_MINUS_DST_ALPHA,
    VL_BLEND_DST_COLOR              = GL_DST_COLOR,
    VL_BLEND_ONE_MINUS_DST_COLOR    = GL_ONE_MINUS_DST_COLOR,
    VL_BLEND_SRC_ALPHA_SATURATE     = GL_SRC_ALPHA_SATURATE,
#elif defined (VL_MODE_GLIDE)
    VL_BLEND_ZERO					= GR_BLEND_ZERO,
    VL_BLEND_ONE					= GR_BLEND_ONE,
    VL_BLEND_SRC_COLOR				= GR_BLEND_SRC_COLOR,
    VL_BLEND_ONE_MINUS_SRC_COLOR	= GR_BLEND_ONE_MINUS_SRC_COLOR,
    VL_BLEND_SRC_ALPHA				= GR_BLEND_SRC_ALPHA,
    VL_BLEND_ONE_MINUS_SRC_ALPHA	= GR_BLEND_ONE_MINUS_SRC_ALPHA,
    VL_BLEND_DST_ALPHA				= GR_BLEND_DST_ALPHA,
    VL_BLEND_ONE_MINUS_DST_ALPHA	= GR_BLEND_ONE_MINUS_DST_ALPHA,
    VL_BLEND_DST_COLOR				= GR_BLEND_DST_COLOR,
    VL_BLEND_ONE_MINUS_DST_COLOR	= GR_BLEND_ONE_MINUS_DST_COLOR,
    VL_BLEND_SRC_ALPHA_SATURATE		= GR_BLEND_ALPHA_SATURATE,
#else
    VL_BLEND_ZERO,
    VL_BLEND_ONE,
    VL_BLEND_SRC_COLOR,
    VL_BLEND_ONE_MINUS_SRC_COLOR,
    VL_BLEND_SRC_ALPHA,
    VL_BLEND_ONE_MINUS_SRC_ALPHA,
    VL_BLEND_DST_ALPHA,
    VL_BLEND_ONE_MINUS_DST_ALPHA,
    VL_BLEND_DST_COLOR,
    VL_BLEND_ONE_MINUS_DST_COLOR,
    VL_BLEND_SRC_ALPHA_SATURATE,
#endif
} PLBlend;

// Blending
#define PL_BLEND_ADDITIVE   VL_BLEND_SRC_ALPHA, VL_BLEND_ONE
#define PL_BLEND_DEFAULT    VL_BLEND_SRC_ALPHA, VL_BLEND_ONE_MINUS_SRC_ALPHA

//-----------------
// Capabilities

/* Replace these with generic functions, rather than relying on just flags?
 * plEnableFog(bool active)
 * plEnableAlphaTest(bool active)
 * plEnableBlend(bool active)
 */

typedef enum PLGraphicsCapability {
    PL_CAPABILITY_FOG               = (1 << 0),     // Fog.
    PL_CAPABILITY_ALPHA_TEST        = (1 << 1),     // Alpha-testing.
    PL_CAPABILITY_BLEND             = (1 << 2),     // Blending.
    PL_CAPABILITY_TEXTURE_2D        = (1 << 3),     // Enables/disables textures.
    PL_CAPABILITY_TEXTURE_GEN_S     = (1 << 4),     // Generate S coordinate.
    PL_CAPABILITY_TEXTURE_GEN_T     = (1 << 5),     // Generate T coordinate.
    PL_CAPABILITY_DEPTHTEST         = (1 << 6),     // Depth-testing.
    PL_CAPABILITY_STENCILTEST       = (1 << 7),     // Stencil-testing.
    PL_CAPABILITY_MULTISAMPLE       = (1 << 8),     // Multisampling.
    PL_CAPABILITY_CULL_FACE         = (1 << 9),     // Automatically cull faces.
    PL_CAPABILITY_SCISSORTEST       = (1 << 10),    // Scissor test for buffer clear.

    // Texture Generation
    PL_CAPABILITY_GENERATEMIPMAP    = (1 << 20),
} PLGraphicsCapability;

PL_EXTERN_C

PL_EXTERN bool plIsGraphicsStateEnabled(unsigned int flags);

PL_EXTERN void plEnableGraphicsStates(unsigned int flags);
PL_EXTERN void plDisableGraphicsStates(unsigned int flags);

PL_EXTERN_C_END

//-----------------
// Textures

#include "platform_graphics_texture.h"

//-----------------
// Meshes

#include "platform_mesh.h"

//-----------------
// Framebuffers

typedef enum PLFBOTarget {
#if defined (PL_MODE_OPENGL) || defined (VL_MODE_OPENGL_CORE)
    PL_FRAMEBUFFER_DEFAULT = GL_FRAMEBUFFER,
    PL_FRAMEBUFFER_DRAW = GL_DRAW_FRAMEBUFFER,
    PL_FRAMEBUFFER_READ = GL_READ_FRAMEBUFFER
#else
    PL_FRAMEBUFFER_DEFAULT,
    PL_FRAMEBUFFER_DRAW,
    PL_FRAMEBUFFER_READ
#endif
} PLFBOTarget;

enum {
    PL_BUFFER_COLOUR    = (1 << 0),
    PL_BUFFER_DEPTH     = (1 << 1),
    PL_BUFFER_STENCIL   = (1 << 2),
};

PL_EXTERN_C

PL_EXTERN void plSetClearColour(PLColour rgba);

PL_EXTERN void plClearBuffers(unsigned int buffers);

PL_EXTERN_C_END

//-----------------
// Lighting

typedef enum PLLightType {
    PLLIGHT_TYPE_SPOT,  // Spotlight
    PLLIGHT_TYPE_OMNI   // Omni-directional
} PLLightType;

typedef struct PLLight {
    PLVector3D position;
    PLVector3D angles;

    PLColour colour;

    PLLightType type;
} PLLight;

//-----------------
// Viewport / Camera

enum {
    PLCAMERA_MODE_PERSPECTIVE,
    PLCAMERA_MODE_ORTHOGRAPHIC,
    PLCAMERA_MODE_ISOMETRIC
};

typedef struct PLViewport {
    int x, y;
    unsigned int w, h;

    uint8_t *v_buffer;
    unsigned int buffers[32];

    unsigned int r_width, r_height;
    unsigned int old_r_width, old_r_height;
} PLViewport;

typedef struct PLCamera {
    double fov;
    double near, far;
    unsigned int mode;

    PLVector3D angles, position;
    PLAABB bounds;

    // Viewport
    PLViewport viewport;
} PLCamera;

PL_EXTERN_C

PL_EXTERN PLCamera *plCreateCamera(void);
PL_EXTERN void plDeleteCamera(PLCamera *camera);

PL_EXTERN void plSetupCamera(PLCamera *camera);

PL_EXTERN void plDrawPerspective(void);

PL_EXTERN void plViewport(int x, int y, unsigned int width, unsigned int height);
PL_EXTERN void plScissor(int x, int y, unsigned int width, unsigned int height);

PL_EXTERN void plPerspective(double fov_y, double aspect, double near, double far);

PL_EXTERN_C_END

//-----------------
// Shaders

typedef int PLSampler1D, PLSampler2D, PLSampler3D;
typedef int PLSamplerCube;
typedef int PLSampler1DShadow, PLSampler2DShadow;

enum {
    PLSHADER_TYPE_VERTEX,
    PLSHADER_TYPE_FRAGMENT,
    PLSHADER_TYPE_GEOMETRY,
    PLSHADER_TYPE_COMPUTE,
};

typedef struct PLShaderProgram {
    uint32_t id;
} PLShaderProgram;

typedef struct PLShader {
    uint32_t id;
} PLShader;

//-----------------

PL_EXTERN_C

PL_EXTERN void plSetDefaultGraphicsState(void);

PL_EXTERN void plFinish(void);

// Hardware Information
PL_EXTERN const char *_plGetHWExtensions(void);
PL_EXTERN const char *_plGetHWRenderer(void);
PL_EXTERN const char *_plGetHWVendor(void);
PL_EXTERN const char *_plGetHWVersion(void);

PL_EXTERN bool plHWSupportsMultitexture(void);
PL_EXTERN bool plHWSupportsShaders(void);

PL_EXTERN unsigned int plGetCurrentShaderProgram(void);

PL_EXTERN void plEnableShaderProgram(unsigned int program);
PL_EXTERN void plDisableShaderProgram(unsigned int program);

PL_EXTERN void plSetCullMode(PLCullMode mode);
PL_EXTERN void plSetBlendMode(PLBlend a, PLBlend b);

PL_EXTERN void plProcessGraphics(void);

PL_EXTERN_C_END

#include "platform_graphics_shader.h"
