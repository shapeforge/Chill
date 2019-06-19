equation1 = input('equation1', 'STRING')
bbox_min1 = input('bbox_min1', 'VEC3')
bbox_max1 = input('bbox_max1', 'VEC3')

equation2 = input('equation2', 'STRING')
bbox_min2 = input('bbox_min2', 'VEC3')
bbox_max2 = input('bbox_max2', 'VEC3')

equation = equation1 .. equation2 ..
[[
float distance]]..__currentNodeId..[[(vec3 p) {
  return distance]]..getNodeId('equation1')..[[(p) * 0.5 + 0.5 * distance]]..getNodeId('equation2')..[[(p);
}
]]

bbox_min = v(math.min(bbox_min1.x,bbox_min2.x), math.min(bbox_min1.y,bbox_min2.y), math.min(bbox_min1.z,bbox_min2.z))
bbox_max = v(math.max(bbox_max1.x,bbox_max2.x), math.max(bbox_max1.y,bbox_max2.y), math.max(bbox_max1.z,bbox_max2.z))

output('equation', 'STRING', equation)
output('bbox_min', 'VEC3' , bbox_min)
output('bbox_max', 'VEC3' , bbox_max)