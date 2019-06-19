size = input('size', 'VEC3', {10.0, 10.0, 10.0}, 0.0, 100.0)

bbox_min = -size/2
bbox_max =  size/2

vec3 = "vec3("..bbox_max.x..","..bbox_max.y..","..bbox_max.z..")"

equation = [[
float distance]]..__currentNodeId..[[(vec3 p) {
  vec3 d = abs(p) - ]]..vec3..[[;
  return length(max(d,0.0)) + min(max(d.x,max(d.y,d.z)),0.0);
}
]]

output('box', 'IMPLICIT', {equation, bbox_min, bbox_max})