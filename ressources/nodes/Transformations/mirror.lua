shape  = input('shape', 'SHAPE', 0)
x = input('X', 'BOOLEAN')
y = input('Y', 'BOOLEAN')
z = input('Z', 'BOOLEAN')

if x then
  shape = mirror(X) * shape
end

if y then
  shape = mirror(Y) * shape
end

if z then
  shape = mirror(Z) * shape
end

output('shape', 'SHAPE', shape)