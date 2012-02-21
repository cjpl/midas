function [type, value] = getNodeAddress()
%Returns a list of node types and node addresses
%Usage: [nodeType, nodeVal] = getNodeAddress()
%nodeType values: 1-fe, 2-adc, 3-channel
%each nodeVal corresponds to a nodeType with the same array index
[type, value] = mexwrapper('192.168.2.2','sc');