local lucy = LUCY_GLOBAL_TABLE
assert(lucy, "lucy global table !exist")
local root = lucy.rootdir
assert(root, "lucy root dir !exist")
package.path = package.path .. ";" .. root .. "?.lua"
package.cpath = package.cpath .. ";" .. root .. "?.so;" .. root .. "?.dll"

