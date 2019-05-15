implicit1 = input('implicit1', 'IMPLICIT')
implicit2 = input('implicit2', 'IMPLICIT')

equation1 = implicit1[1]
bbox_min1 = implicit1[2]
bbox_max1 = implicit1[3]

equation2 = implicit2[1]
bbox_min2 = implicit2[2]
bbox_max2 = implicit2[3]

bbox_min = bbox_min1
bbox_max = bbox_max1

equation = equation1 .. equation2 .. [[
float distance]] .. __currentNodeId .. [[(vec3 p) {
  return max(
    distance]] .. getNodeId('implicit1') .. [[(p),
    -distance]] .. getNodeId('implicit2') .. [[(p)
  );
}
]]

output('implicit', 'IMPLICIT', {equation, bbox_min, bbox_max})