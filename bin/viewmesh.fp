#version 120

uniform sampler2D u_Tex;

uniform int       u_ShowTexCoord;
uniform int       u_ShowNormal;
uniform vec3      u_Diffuse;

varying vec3      v_Pos;

void main()
{

  if (u_ShowTexCoord != 0) {
	  gl_FragColor   = vec4 (fract(gl_TexCoord[0].xy),0,1);
  } else if (u_ShowNormal != 0) {
    gl_FragColor   = vec4 (gl_TexCoord[1].xyz*0.5+0.5,1);
  } else {
	  gl_FragColor   = texture2D(u_Tex,gl_TexCoord[0].xy) * vec4(u_Diffuse,1);  
  }
  
}
