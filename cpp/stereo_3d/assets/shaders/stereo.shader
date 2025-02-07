#if 0
Name "Vertex Position and Texture (Stereo 3D)"

Passes "Main"

#endif


Main {

/**
Cull Back
Blend SrcAlpha OneMinusSrcAlpha Add
Depth LEqual
**/

#define TRANSFORM
#define VTX_P_T
#include "includes.glsl"

vary vec4 var_t;

#ifdef __vert__

void main(){
  gl_Position = PVW * vtx_p;
  var_t = vtx_t;
}

#endif

#ifdef __frag__

layout(binding = 0) uniform sampler2D ltex;
layout(binding = 1) uniform sampler2D rtex;


void main(){
  vec3 c;
  c.r = texture( ltex, var_t.xy ).r;
  c.gb = texture( rtex, var_t.xy ).gb;
  gl_FragData[0] = vec4( c, 1.0 );
}

#endif

}