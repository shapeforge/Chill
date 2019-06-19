implicit1 = input('implicit1', 'IMPLICIT')
implicit2 = input('implicit2', 'IMPLICIT')

equation1 = implicit1[1]
bbox_min1 = implicit1[2]
bbox_max1 = implicit1[3]

equation2 = implicit2[1]
bbox_min2 = implicit2[2]
bbox_max2 = implicit2[3]

bbox_min = v(math.min(bbox_min1.x, bbox_min2.x), math.min(bbox_min1.y, bbox_min2.y), math.min(bbox_min1.z, bbox_min2.z))
bbox_max = v(math.max(bbox_max1.x, bbox_max2.x), math.max(bbox_max1.y, bbox_max2.y), math.max(bbox_max1.z, bbox_max2.z))

equation = equation1 .. equation2 .. [[
float distance]] .. __currentNodeId .. [[(vec3 p) {
  return min(
    distance]] .. getNodeId('implicit1') .. [[(p),
    distance]] .. getNodeId('implicit2') .. [[(p)
  );
}
]]

output('implicit', 'IMPLICIT', {equation, bbox_min, bbox_max})