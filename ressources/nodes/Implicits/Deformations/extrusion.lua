implicit = input('implicit', 'IMPLICIT')

equation = implicit[1]
bbox_min = implicit[2]
bbox_max = implicit[3]

elongation = input('distance', 'VEC3')

bbox_min = bbox_min - elongation
bbox_max = bbox_max + elongation

vec3 = "vec3("..elongation.x..","..elongation.y..","..elongation.z..")"

equation = equation .. [[
float distance]] .. __currentNodeId .. [[(vec3 p) {
  vec3 h = ]]..vec3..[[;
  vec3 q = p - clamp( p, -h, h );
  return distance]] .. getNodeId('implicit') .. [[(q);
}
]]

bbox_min = bbox_min - elongation
bbox_max = bbox_max + elongation

output('implicit', 'IMPLICIT', {equation, bbox_min, bbox_max})