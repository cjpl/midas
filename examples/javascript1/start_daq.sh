#!/bin/sh

odbedit -c cleanup
mhttpd -D -p 8080
mlogger -D

echo Please connect to midas status page at http://localhost:8080

#end
