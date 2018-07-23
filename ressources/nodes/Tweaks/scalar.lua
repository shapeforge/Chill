name  = input('name' , 'STRING', 'scalar_value')
value = input('value', 'SCALAR', 10.0)
min   = input('min'  , 'SCALAR', 0.0)
max   = input('max'  , 'SCALAR', 100.0)

output('scalar', 'SCALAR', ui_scalar(name, value, min, max))
