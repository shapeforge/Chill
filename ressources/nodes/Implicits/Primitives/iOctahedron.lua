radius = input('radius', 'SCALAR', 0.5, 0.0, 100.0)

bbox_min = v(-radius,-radius,-radius)
bbox_max = v( radius, radius, radius)

equation = [[
float distance]]..__currentNodeId..[[(vec3 p) {
  float s = ]]..radius..[[;
  p = abs(p);
  float m = p.x+p.y+p.z-s;
  vec3 q;
       if( 3.0*p.x < m ) q = p.xyz;
  else if( 3.0*p.y < m ) q = p.yzx;
  else if( 3.0*p.z < m ) q = p.zxy;
  else return m*0.57735027;
    
  float k = clamp(0.5*(q.z-q.y+s),0.0,s); 
  return length(vec3(q.x,q.y-s+k,q.z-k)); 
}
]]

output('octahedron', 'IMPLICIT', {equation, bbox_min, bbox_max})