radius = input('radius', 'SCALAR', 0.0, 1.0, 50.0)
height = input('height', 'SCALAR', 0.0, 1.0, 50.0)

bbox_min = v(-radius, -radius, -height/2)
bbox_max = v( radius,  radius,  height/2)

h = "vec2("..radius..","..height..")"

equation = [[
float distance]]..__currentNodeId..[[(vec3 p) {
  vec2 d = abs(vec2(length(p.xy),p.z)) - ]]..h..[[;
  return min(max(d.x,d.y),0.0) + length(max(d,0.0));
}
]]

output('cylinder', 'IMPLICIT', {equation, bbox_min, bbox_max})