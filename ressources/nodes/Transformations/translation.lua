shape  = input('shape', 'SHAPE', 0)
vector = input('translation', 'VEC3', {0, 0, 0}, -1000, 1000)

output('shape', 'SHAPE', translate(vector) * shape)