graph_0 = createGraph('GraphName')

node_0 = createNode('NodeName', 'nodes/inputs/stl')
node_1 = createNode('NodeName', 'nodes/outputs/emit')
link_0 = createLink(node_0, 'shape', node_1, 'shape')

addNode(graph_0, node_0)
addNode(graph_0, node_1)
addLink(graph_0, link_0)

setInputValue(node_0, 'path', './stl/dog.stl')