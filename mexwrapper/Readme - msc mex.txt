Matlab usage for mexwrapper:

1. place the mex file (mexwrapper) into the active workspace opened in Matlab

2. scan and return lists of node types and node addresses
	[nodeType, nodeAddr] = mexwrapper(ip,node_addr,'sc');

3. reading through mscb
	[a,b] = mexwrapper(ip,node_addr,'r',read_addr);
	
4. writing through mscb
	mexwrapper(ip,node_addr,'w',write_addr,write_val);

5. print node information
	mexwrapper(ip,node_addr,'i');

data format
	ip:string - eg.'172.23.1.153'
	node_addr:int
	read_addr/write_addr:int
	write_val: int
	a: string containing the register name
	b: register value
	nodeType: array of node types (1 - FEBRD, 2 - FEADC, 3 - FECHAN)
	nodeAddr: array of node addresses corresponding to a nodeType with same array index (nodeAddr[i] <-> nodeType[i]) 

--------------------------------------------------------------------------------------------------

Building mex file in Matlab command window with:

1. mex -setup

2. Choose Visual Studio Compiler

3. Build mex file in Matlab command window with the command:
	mex mexwrapper.c msc.c mscb.c mscbrpc.c musbstd.c mxml.c strlcpy.c libusb.lib wsock32.lib -v