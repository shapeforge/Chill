centered = input('centered', 'BOOLEAN', false)
size     = input('size'    , 'VEC3'   , {10.0, 10.0, 10.0}, 0.0, 100.0)

if centered then
  shape = ccube(1, 1, 1)
else
  shape = cube(1, 1, 1)
end
output('cube', 'SHAPE', scale(size) * shape)