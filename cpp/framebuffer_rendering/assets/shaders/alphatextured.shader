#if 0
Name "Vertex Position and Color with Alpha Texture"

Passes "Main"

#endif


Main {

/**
Cull Back
Blend SrcAlpha OneMinusSrcAlpha Add
Depth LEqual
**/

#define TRANSFORM
#define VTX_P_C_T
#include "includes.glsl"

vary vec4 var_t, var_c;

#ifdef __vert__

void main(){
  gl_Position = PVW * vtx_p;
  var_t = vtx_t;
  var_c = vtx_c;
}

#endif

#ifdef __frag__

layout(binding = 0) uniform sampler2D tex;


void main(){
  gl_FragData[0] = vec4( var_c.xyz, texture( tex, var_t.xy ).r  );
}

#endif

}