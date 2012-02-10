Matlab usage for mexwrapper:
1. place the mex file (mexwrapper) into the active workspace opened in Matlab
2. reading through mscb
	[a,b] = mexwrapper(ip,node_addr,'r',read_addr);
	
3. writing through mscb
	mexwrapper(ip,node_addr,'w',write_addr,write_val);

data format
	ip:string - eg.'172.23.1.153'
	node_addr:int
	read_addr/write_addr:int
	write_val: int
	a: string containing the register name
	b: number containing the register value
