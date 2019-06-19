r1 = input('radius1', 'SCALAR', 5.0, 0.0, 50.0)
r2 = input('radius2', 'SCALAR', 0.0, 0.0, 50.0)
h = input('height', 'SCALAR', 10.0, 0.0, 50.0)

h = h/2

bbox_min = v(-math.max(r1,r2),-math.max(r1,r2),-h)
bbox_max = v( math.max(r1,r2), math.max(r1,r2), h)

equation = [[
float distance]]..__currentNodeId..[[(vec3 p) {
  vec2 q = vec2( length(p.xy), p.z );
  vec2 k1 = vec2(]]..r2..[[,]]..h..[[);
  vec2 k2 = vec2(]]..r2..[[-]]..r1..[[,2.0*]]..h..[[);
  vec2 ca = vec2(q.x-min(q.x,(q.y < 0.0)?]]..r1..[[:]]..r2..[[), abs(q.y)-]]..h..[[);
  vec2 cb = q - k1 + k2*clamp( dot(k1-q,k2)/dot(k2,k2), 0.0, 1.0 );
  float s = (cb.x < 0.0 && ca.y < 0.0) ? -1.0 : 1.0;
  return s*sqrt( min(dot(ca,ca),dot(cb,cb)) );
}
]]

output('cone', 'IMPLICIT', {equation, bbox_min, bbox_max})