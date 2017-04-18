local L = {logLevel=4};
function L.isbtype(a) return type(a) == "boolean" end
function L.ntbtype(a) return type(a) ~= "boolean" end
function L.isntype(a) return type(a) == "number" end
function L.ntntype(a) return type(a) ~= "number" end
function L.isstype(a) return type(a) == "string" end
function L.ntstype(a) return type(a) ~= "string" end
function L.isftype(a) return type(a) == "function" end
function L.ntftype(a) return type(a) ~= "function" end
function L.isutype(a) return type(a) == "userdata" end
function L.ntutype(a) return type(a) ~= "userdata" end
function L.istable(a) return type(a) == "table" end
function L.nttable(a) return type(a) ~= "table" end
function L.isthread(a) return type(a) == "thread" end
function L.ntthread(a) return type(a) ~= "thread" end
local function Logger(level, msg, detail, ...)
  if not L.logLevel or level > L.logLevel then return end
  if not detail then print(msg, ...); return end
  print(msg .. ": " .. tostring(detail), ...)
end
-- 1:error, 2:warning, 3:important, 4:debug
function L.logE(msg, detail, ...) Logger(1, "[E] " .. msg, detail, ...) end
function L.logW(msg, detail, ...) Logger(2, "[W] " .. msg, detail, ...) end
function L.logI(msg, detail, ...) Logger(3, "[I] " .. msg, detail, ...) end
function L.logD(msg, detail, ...) Logger(4, "[D] " .. msg, detail, ...) end
function L.callinfo(level)
  local t = debug.getinfo(level+1, "nSl")
  local s = "in " .. t.what
  if t.name then s = s .. " function " .. t.name .. "()" end
  if t.short_src then s = s .. " at " .. t.short_src end
  if t.currentline then s = s .. " " .. t.currentline end
  return s
end
setmetatable(_G, { -- metamethod is called when the name is not in the table
  __newindex = function(t, n, v)
    local scope = debug.getinfo(2, "S").what
    if scope == "main" or scope == "C" then
      rawset(t, n, v) -- only lua functions are not allowed
    else
      L.logE("write undefined variable '" .. n .. "'", L.callinfo(2))
    end
  end,
  __index = function(_, n) -- forbidden to read undefined globals for all
    L.logE("read undefined variable '" .. n .. "'", L.callinfo(2))
  end
})
function L.global(name, init)
  if init == nil then
    return rawget(_G, name)
  end
  rawset(_G, name, init)
end
function L.assert(e)
  if e then return true end
  L.logE("assert fail", L.callinfo(2))
  return false
end
function L.assertEq(a, b)
  if a == b then return true end
  local ci = L.callinfo(2)
  if L.istable(a) and L.istable(b) then
    for n,v in pairs(a) do
      if v ~= b[n] then L.logE("assertEq fail a["..n.."] "..v.." ~= "..b[n], ci); return false; end
    end
    for n,v in pairs(b) do
      if v ~= a[n] then L.logE("assertEq fail b["..n.."] "..v.." ~= "..a[n], ci); return false; end
    end
    return true
  end
  L.logE("assertEq fail " .. tostring(a) .. "~=" .. tostring(b), L.callinfo(2))
  return false
end
function L.assertRg(iors, beg, endIncluded)
  local n = tonumber(iors)
  local debuginfo = L.callinfo(2)
  if n == nil then L.logE("assertRg fail " .. tostring(iors) .. " !tonumber", debuginfo); return false end
  if n >= beg and n <= endIncluded then return true end
  L.logE("assertRg fail " .. tostring(n) .. " !in [" .. tostring(beg) .. "," .. tostring(endIncluded) .. "]", debuginfo)
  return false
end
function L.yearMonth() -- month [1,12]
  return os.date("%Y%m")
end
function L.yearLastMonth() -- last month
  local year = tonumber(os.date("%Y"))
  local month = tonumber(os.date("%m"))
  if month > 1 then
    month = month - 1
  else
    month = 12
    year = year - 1
  end
  return string.format("%4d%02d", year, month)
end
function L.yearMonthDay() -- day [1,31]
  return os.date("%Y%m%d")
end
function L.hourMin() -- hour [0,23]
  return os.date("%H%M")
end
function L.hourMinSec() -- min [0,59], sec [0,61]
  return os.date("%H%M%S")
end
function L.weekday()
  return os.date("%w") -- [0,6], 0 is sunday
end
local lpeg = require "lpeg"
function L.St(t) -- match a string in the table
  local patt = lpeg.P(false)
  for _,v in ipairs(t) do
    patt = patt + lpeg.P(v)
  end
  return patt
end
function L.Cst(t) -- capture the index of matched string in the table
  local patt = lpeg.P(false)
  for i,v in ipairs(t) do
    patt = patt + lpeg.P(v) * lpeg.Cc(i)
  end
  return patt
end
L.P = lpeg.P
L.R = lpeg.R
L.S = lpeg.S
L.B = lpeg.B
L.V = lpeg.V
L.C = lpeg.C
L.Cc = lpeg.Cc
L.Cp = lpeg.Cp
L.Cg = lpeg.Cg
L.Cb = lpeg.Cb
L.Ct = lpeg.Ct
L.Cs = lpeg.Cs
L.Cf = lpeg.Cf
L.Cmt = lpeg.Cmt
L.Carg = lpeg.Carg
local EndOfFile = L.P"\x00" + L.P"\x1A" + L.P(-1)
local BlankTable = {
  "\x09", -- \t
  "\x0B", -- \v
  "\x0C", -- \f
  -- Zs 'Separator, Space' Category - www.fileformat.info/info/unicode/category/Zs/list.htm
  "\x20", -- 0x20 space
  "\xC2\xA0", -- 0xA0 no-break space
  "\xE1\x9A\x80", -- 0x1680 ogham space mark
  "\xE2\x80\xAF", -- 0x202F narrow no-break space
  "\xE2\x81\x9F", -- 0x205F medium mathematical space
  "\xE3\x80\x80", -- 0x3000 ideographic space
  L.P"\xE2\x80" * L.R"\x80\x8A", -- [0x2000, 0x200A]
  -- byte order mark
  "\xFE\xFF",
  "\xFF\xFE",
  "\xEF\xBB\xBF",
} -- \t (0x09) \n (0x0A) \v (0x0B) \f (0x0C) \r (0x0D)
local NewlineTable = {
  "\x0A\x0D",
  "\x0D\x0A",
  "\x0A", -- \n
  "\x0D", -- \r
  "\xE2\x80\xA8", -- line separator 0x2028 00100000_00101000 -> 1110'0010_10'000000_10'101000 (0xE280A8)
  "\xE2\x80\xA9", -- paragraph separator 0x2029 00100000_00101001
}
L.spc = L.P"\x20"
L.tab = L.P"\x09"
L.newline = L.St(NewlineTable) + EndOfFile
L.blank = L.St(BlankTable)
L.anyblank = L.blank + L.newline
L.nonblank = 1 - L.anyblank
L.linecomment = function (start)
  return P(start) * (1 - Newline)^0 * Newline
end
L.integer = L.R("09")^1
L.Clinequote = function (s, e)
  if e == nil then e = s end
  return L.P(s) * L.C((L.P("\\"..e) + (-L.P(e))*(-L.newline)*1)^0) * L.P(e)
end
L.readfile = function (name)
  local f = io.open(name, "rb")
  if not f then return nil end
  return f:read("a")
end
return L
