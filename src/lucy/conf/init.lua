local lucy = LUCY_GLOBAL_TABLE
local root = lucy.rootdir
assert(root, "lucy root dir doesn't exist")
package.path = package.path .. ";" .. root .. "?.lua"
package.cpath = package.cpath .. ";" .. root .. "?.so;" .. root .. "?.dll"

