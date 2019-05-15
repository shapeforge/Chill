shape  = input('shape', 'SHAPE', 0)
vector = input('rotation', 'VEC3', {0, 0, 0}, -1000, 1000)

output('shape', 'SHAPE', rotate(vector) * shape)