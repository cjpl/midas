%get a list of node addresses
[nodeType, nodeAddr] = getNodeAddress();

%get value of a register
result = setVal(nodeType, nodeAddr, 'fthresh', 8);

%set value of a register to 996
setVal(nodeType, nodeAddr, 'fthresh', 8, 996);

%get value of a register
result = setVal(nodeType, nodeAddr, 'fthresh', 8);



