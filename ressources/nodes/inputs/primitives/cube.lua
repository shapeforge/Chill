width  = input('width',  SCALAR)
height = input('height', SCALAR)
depth  = input('depth',  SCALAR)

centered = tweak('centered', BOOL)


if centered then
  shape = ccube(width, height, depth)
else
  shape = cube(width, height, depth)
end

output('cube', SHAPE, shape)