implicit = input('implicit', 'IMPLICIT')

equation = implicit[1]
bbox_min = implicit[2]
bbox_max = implicit[3]

o = input('offset', 'SCALAR')

bbox_min = v(bbox_min.x-o,bbox_min.x-o,bbox_min.z)
bbox_max = v(bbox_max.x+o,bbox_max.x+o,bbox_max.z)

equation = equation ..
[[
float distance]]..__currentNodeId..[[(vec3 p) {
  vec3 q = vec3(length(p.xyz) - ]]..o..[[, length(p.xyz) - ]]..o..[[, p.z);
  return distance]]..getNodeId('implicit')..[[(q);
}
]]

output('implicit', 'IMPLICIT', {equation, bbox_min, bbox_max})