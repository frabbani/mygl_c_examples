#if 0
Name "Vertex Simple"

Passes "Main"

#endif


Main {

/**
Cull Back
Blend SrcAlpha OneMinusSrcAlpha Add
Depth LEqual
**/

#define TRANSFORM

layout(location = 0) in vec4 vtx_p;
#include "includes.glsl"

#ifdef __vert__

void main(){
  gl_Position = PVW * vtx_p;
}

#endif

#ifdef __frag__


void main(){
  gl_FragData[0] = vec4( 1.0, 1.0, 0.0, 1.0 );
}

#endif

}