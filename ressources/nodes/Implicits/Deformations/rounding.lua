implicit = input('implicit', 'IMPLICIT')

equation = implicit[1]
bbox_min = implicit[2]
bbox_max = implicit[3]

r = input('radius', 'SCALAR')

bbox_min = bbox_min - v(r,r,r)
bbox_max = bbox_max + v(r,r,r)

equation = equation ..
[[
float distance]]..__currentNodeId..[[(vec3 p) {
  return distance]]..getNodeId('implicit')..[[(p) - ]]..r..[[;
}
]]

output('implicit', 'IMPLICIT', {equation, bbox_min, bbox_max})