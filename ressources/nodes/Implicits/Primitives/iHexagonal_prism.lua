radius = input('radius', 'SCALAR', 0.0, 1.0, 50.0)
height = input('height', 'SCALAR', 0.0, 1.0, 50.0)

height = height/2

ratio = 1.1547 -- 2/sqrt(3)

bbox_min = v(-radius*1.1547, -radius, -height)
bbox_max = v( radius*1.1547,  radius,  height)

h = "vec2("..radius..","..height..")"

equation = [[
float distance]]..__currentNodeId..[[(vec3 p) {
  const vec3 k = vec3(-0.8660254, 0.5, 0.57735);
  vec2 h = ]]..h..[[;
  p = abs(p);
  p.xy -= 2.0*min(dot(k.xy, p.xy), 0.0)*k.xy;
  vec2 d = vec2(
     length(p.xy-vec2(clamp(p.x,-k.z*h.x,k.z*h.x), h.x))*sign(p.y-h.x),
     p.z-h.y
  );
  return min(max(d.x,d.y),0.0) + length(max(d,0.0));
}
]]

output('hexagonal_prism', 'IMPLICIT', {equation, bbox_min, bbox_max})