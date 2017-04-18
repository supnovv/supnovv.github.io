local root = "/path/to/src/"
package.path = package.path .. ";" .. root .. "lualib/?.lua"
package.cpath = package.cpath .. ";" .. root .. "share/?.so;" .. root .. "share/?.dll"
local L = require "thatlcore"
local M = require "markdown"
local function CORTEST()
  -- debug info
  L.assertEq(debug.getinfo(0, "S").what, "C") -- 0:getinfo itself
  L.assertEq(debug.getinfo(1, "S").what, "Lua") -- 1:the function that called getinfo
  L.assertEq(debug.getinfo(2, "S").what, "main")
  L.assertEq(debug.getinfo(3, "S").what, "C")
  -- nil and boolean
  local noinit, s, y, m
  L.assert(noinit == nil)
  if nil then L.assert(false)
  else L.assert(true) end
  L.assertEq(L.global("NaN"), nil) -- NaN is not a global variable in lua
  L.assertEq(L.undefined, nil)
  -- comapare
  L.assert(nil ~= false)
  L.assert("1" ~= 1)
  L.assert("ab" == "ab")
  L.assert("ab" > "")
  L.assert("ab" > "aac")
  L.assert("ab" < "abc")
  L.assert("ab" < "acc")
  -- concate number
  L.assertEq("ab" .. 1200, "ab1200")
  L.assertEq("ab" .. 0.10, "ab0.1")
  L.assertEq("ab" .. 1.00, "ab1.0")
  L.assertEq("ab" .. 1., "ab1.0")
  L.assertEq("ab" .. .0, "ab0.0")
  L.assertEq("ab" .. .10, "ab0.1")
  -- if statements
  local x, y, z, m = 1, 1, 1, 1
  if x == 1 then L.assertEq(x, 1) end
  if y == 2 then print("y==2") else L.assertEq(y, 1) end
  if z == 2 then print("z==2") elseif z == 1 then L.assertEq(z, 1) end
  if m == 3 then print("m==3") elseif m == 2 then print("m==2") else L.assertEq(m, 1) end
  if x == 1 and y == 1 then L.assertEq(x+y, 2) end
  if x == 1 and (y == 1 or z == 2) then L.assertEq(x+y+z, 3) end
  -- for loop
  local a, r = {"a", "b", "c", type="pub", color="red"}, {}
  for i = 1, 5 do table.insert(r,i) end
  L.assertEq(r, {1, 2, 3, 4, 5}); r = {}
  for i = 1, 5, 2 do table.insert(r,i) end
  L.assertEq(r, {1, 3, 5}); r = {}
  for i,v in ipairs(a) do table.insert(r, i..v) end
  L.assertEq(r, {"1a", "2b", "3c"}); r = {}
  for n,v in pairs(a) do r[n]=v end
  L.assertEq(r, {[1]="a"; [2]="b", [3]="c", type="pub", color="red"})
  -- string
  local s = "aBcD"
  L.assertEq(#s, 4)
  L.assertEq(s:len(), 4)
  L.assertEq(s:lower(), "abcd")
  L.assertEq(s:upper(), "ABCD")
  L.assertEq(s:sub(1,2), "aB")
  L.assertEq(s:reverse(), "DcBa")
  -- table
  local t = {}
  L.assertEq(#t, 0)
  table.insert(t, "a") -- append a element to table
  L.assertEq(t, {"a"})
  table.insert(t, 1, "b") -- insert a element to the position 1
  L.assertEq(t, {"b", "a"})
  table.remove(t) -- remove the last element in the table
  L.assertEq(t, {"b"})
  table.remove(t, 2)
  L.assertEq(t, {"b"})
  table.remove(t, 1)
  L.assertEq(t, {})
  table.remove(t)
  L.assertEq(t, {})
  -- date
  s = L.yearMonth()
  L.assert(s:len() >= 6)
  y = tonumber(s:sub(1,-3))
  m = tonumber(s:sub(-2))
  L.assertRg(m, 1, 12)
  s = L.yearLastMonth()
  L.assertRg(s:sub(-2), 1, 12)
  if m > 1 then
    L.assertEq(tonumber(s:sub(-2)), m-1)
  else
    L.assertEq(s:sub(-2), "12")
    L.assertEq(tonumber(s:sub(1,-3)), y-1)
  end
  s = L.yearMonthDay()
  L.assert(s:len() >= 8);
  L.assertRg(s:sub(-4,-3), 1, 12)
  L.assertRg(s:sub(-2), 1, 31)
  s = L.hourMin()
  L.assertEq(s:len(), 4)
  L.assertRg(s:sub(1,2), 0, 23)
  L.assertRg(s:sub(3,4), 0, 59)
  s = L.hourMinSec()
  L.assertEq(s:len(), 6)
  L.assertRg(s:sub(1,2), 0, 23)
  L.assertRg(s:sub(3,4), 0, 59)
  L.assertRg(s:sub(5,6), 0, 61)
  s = L.weekday()
  L.assertRg(s, 0, 6)
  L.logD(L.yearMonthDay().." "..L.hourMinSec().." weekday "..L.weekday().." last month "..L.yearLastMonth())
  -- global and environment
  L.assertEq(L.global("undefinedGlobal"), nil)
  L.global("ntdefined", 1)
  L.assertEq(ntdefined, 1)
  ntdefined = nil
  L.assertEq(L.global("ntdefined"), nil)
  L.assertEq(_ENV, _G)
  L.assertEq(L.global("_G"), _G)
  _ENV = {}
  local foo
  do
    local _ENV = _ENV
    function foo() return X end --> bind to _ENV.X
  end
  X = 13
  L.assertEq(L.global("X"), nil)
  L.assertEq(_ENV.X, 13)
  _ENV = nil
  L.assertEq(foo(), 13) -- X = 0 -- error: attempt to index a nil
  function foo(_ENV, a) return a + b end
  L.assertEq(foo({b=1}, 1), 2)
  L.assertEq(foo({b=2}, 2), 4)
end
local function PEGTEST()
  -- basic match
  local matchabc = L.P("abc")
  local match4char = L.P(4) -- positive number
  local match0char = L.P(0) -- always success
  local matchsucc = L.P(true)
  local matchfail = L.P(false)
  L.assertEq(matchabc:match(""), nil)
  L.assertEq(matchabc:match("ab"), nil)
  L.assertEq(matchabc:match("abc"), 4)
  L.assertEq(matchabc:match("abcd"), 4)
  L.assertEq(matchabc:match("xabc"), nil)
  L.assertEq(matchabc:match("xabc", 2), 5)
  L.assertEq(matchabc:match("xabcd", 2), 5)
  L.assertEq(matchabc:match("xabcd", 6), nil)
  L.assertEq(matchabc:match("xabcd", 7), nil)
  L.assertEq(match4char:match(""), nil)
  L.assertEq(match4char:match("abc"), nil)
  L.assertEq(match4char:match("abcd"), 5)
  L.assertEq(match4char:match("abcde"), 5)
  L.assertEq(match4char:match("abcdef"), 5)
  L.assertEq(match4char:match("abcdef", 2), 6)
  L.assertEq(match4char:match("abcdef", 7), nil)
  L.assertEq(match4char:match("abcdef", 8), nil)
  L.assertEq(match0char:match(""), 1)
  L.assertEq(match0char:match("a"), 1)
  L.assertEq(match0char:match("ab"), 1)
  L.assertEq(match0char:match("abc", 3), 3)
  L.assertEq(match0char:match("abc", 4), 4)
  L.assertEq(match0char:match("abc", 5), 4)
  L.assertEq(match0char:match("", 2), 1)
  L.assertEq(match0char:match("", 3), 1)
  L.assertEq(matchsucc:match(""), 1)
  L.assertEq(matchsucc:match("a"), 1)
  L.assertEq(matchsucc:match("ab"), 1)
  L.assertEq(matchsucc:match("ab", 2), 2)
  L.assertEq(matchsucc:match("ab", 3), 3)
  L.assertEq(matchsucc:match("ab", 4), 3)
  L.assertEq(matchfail:match(""), nil)
  L.assertEq(matchfail:match("a"), nil)
  L.assertEq(matchfail:match("a", 2), nil)
  L.assertEq(matchfail:match("a", 3), nil)
  -- match before
  local abcbefore = L.B(matchabc)
  local fcharbefore = L.B(match4char)
  local zcharbefore = L.B(match0char)
  L.assertEq(abcbefore:match("abc", 3), nil)
  L.assertEq(abcbefore:match("abc", 4), 4)
  L.assertEq(abcbefore:match("abc", 5), 4) -- bigger index is converted to len+1
  L.assertEq(abcbefore:match("abc", 6), 4) -- bigger index is converted to len+1
  L.assertEq(abcbefore:match("xabc", 4), nil)
  L.assertEq(abcbefore:match("xabc", 5), 5)
  L.assertEq(fcharbefore:match("abc", 4), nil)
  L.assertEq(fcharbefore:match("abcd", 4), nil)
  L.assertEq(fcharbefore:match("abcd", 5), 5)
  L.assertEq(zcharbefore:match(""), 1)
  L.assertEq(zcharbefore:match("", 2), 1)
  L.assertEq(zcharbefore:match("ab", 1), 1)
  L.assertEq(zcharbefore:match("ab", 2), 2)
  L.assertEq(zcharbefore:match("ab", 3), 3)
  L.assertEq(zcharbefore:match("ab", 4), 3)
  -- match predicate (positive)
  L.assertEq((#matchabc):match("xabc"), nil)
  L.assertEq((#matchabc):match("xabc", 2), 2) -- predicate doesn't consume characters
  L.assertEq((#matchabc):match("xabc", 3), nil)
  L.assertEq((#match4char):match("abcde"), 1)
  L.assertEq((#match4char):match("abcde", 2), 2)
  L.assertEq((#match4char):match("abcde", 3), nil)
  L.assertEq((#match0char):match(""), 1)
  L.assertEq((#match0char):match("a"), 1)
  L.assertEq((#match0char):match("a", 2), 2)
  L.assertEq((#match0char):match("a", 3), 2)
  -- match predicate (negative)
  L.assertEq((-matchabc):match("xabc"), 1) -- default index is 1 if not specified
  L.assertEq((-matchabc):match("xabc", 2), nil)
  L.assertEq((-matchabc):match("xabc", 3), 3)
  L.assertEq((-match4char):match("abcd"), nil)
  L.assertEq((-match4char):match("abcd", 2), 2)
  L.assertEq((-match4char):match("abcd", 3), 3)
  L.assertEq((-match0char):match(""), nil)
  L.assertEq((-match0char):match("a"), nil)
  L.assertEq((-match0char):match("a", 2), nil)
  L.assertEq((-match0char):match("a", 3), nil)
  -- match a char in the range
  local atoz = L.R("az", "AZ")
  L.assertEq(atoz:match(""), nil)
  L.assertEq(atoz:match("a"), 2)
  L.assertEq(atoz:match("X2"), 2)
  L.assertEq(atoz:match("X2", 2), nil)
  -- match a char in a set
  local oper = L.S("+-*/")
  L.assertEq(oper:match("+"), 2)
  L.assertEq(oper:match("-"), 2)
  L.assertEq(oper:match("*"), 2)
  L.assertEq(oper:match("/"), 2)
  L.assertEq(oper:match("/z", 2), nil)
  -- ordered choice
  local ab = L.P("ab")
  local cd = L.P("cd")
  local abc = L.P("abc")
  L.assertEq((ab+cd):match("xabcd"), nil)
  L.assertEq((ab+cd):match("xabcd", 2), 4)
  L.assertEq((ab+cd):match("xabcd", 3), nil)
  L.assertEq((ab+cd):match("xabcd", 4), 6)
  L.assertEq((ab+cd):match("cd"), 3)
  L.assertEq((ab+abc):match("abcd"), 3)
  L.assertEq((abc+ab):match("abcd"), 4)
  -- continue match
  L.assertEq((ab*cd):match("abcd"), 5)
  L.assertEq((-abc*ab):match("abcd"), nil)
  L.assertEq((-abc*ab):match("abxd"), 3)
  -- repetition
  local zeromore = L.P(ab^0)
  local onemore = L.P(ab^1)
  local twomore = L.P(ab^2)
  local zeroone = L.P(ab^-1)
  local zeroto3 = L.P(ab^-3)
  L.assertEq(zeromore:match(""), 1)
  L.assertEq(zeromore:match("acc"), 1)
  L.assertEq(zeromore:match("abc"), 3)
  L.assertEq(zeromore:match("ababc"), 5)
  L.assertEq(zeromore:match("xabc", 2), 4)
  L.assertEq(onemore:match(""), nil)
  L.assertEq(onemore:match("xab"), nil)
  L.assertEq(onemore:match("xab", 2), 4)
  L.assertEq(onemore:match("abc"), 3)
  L.assertEq(onemore:match("ababc"), 5)
  L.assertEq(twomore:match(""), nil)
  L.assertEq(twomore:match("abc"), nil)
  L.assertEq(twomore:match("ababc"), 5)
  L.assertEq(twomore:match("ababab"), 7)
  L.assertEq(zeroone:match"", 1)
  L.assertEq(zeroone:match"ax", 1)
  L.assertEq(zeroone:match"ab", 3)
  L.assertEq(zeroone:match"abc", 3)
  L.assertEq(zeroone:match"abab", 3)
  L.assertEq(zeroto3:match"", 1)
  L.assertEq(zeroto3:match"ax", 1)
  L.assertEq(zeroto3:match"ab", 3)
  L.assertEq(zeroto3:match"abab", 5)
  L.assertEq(zeroto3:match"ababab", 7)
  L.assertEq(zeroto3:match"abababab", 7)
  -- grammar
  local balance = L.P{
   "S", -- indicates the initial symbol is S, this S is defined below
    S = "(" * (1 - L.S"()" + L.V"S") ^ 0 * ")"
  }                     -- ^ current char is ( or ) if goes here,
                        -- for ) it will match fail then go out of the repetition and to match the last ")",
                        -- for ( need match balanced parentheses recursively
  L.assertEq(balance:match"(", nil)
  L.assertEq(balance:match")", nil)
  L.assertEq(balance:match"()", 3)
  L.assertEq(balance:match"(a)", 4)
  L.assertEq(balance:match"(()", nil)
  L.assertEq(balance:match"()))", 3)
  L.assertEq(balance:match"(a)))", 4)
  L.assertEq(balance:match"(())))", 5)
  L.assertEq(balance:match"(a())))", 6)
  L.assertEq(balance:match"(a(b))))", 7)
  L.assertEq(balance:match"(a(b)c)))", 8)
  -- simple capture
  -- the rule of values returned:
  -- * if the pattern matches failed then return nil end
  -- * if no captures in the pattern || no capture values produced
  -- *   return the position after the matched substring
  -- * end
  -- * return capture values including nested ones
  local czeroone = L.C(ab^-1)
  L.assertEq(czeroone:match"", "")
  L.assertEq(czeroone:match"ax", "")
  L.assertEq(czeroone:match"ab", "ab")
  L.assertEq(czeroone:match"abab", "ab")
  local cabzeroone = L.C(ab)^-1
  L.assertEq(cabzeroone:match"", 1) -- match success (zero ab), but capture failed (ab no match)
  L.assertEq(cabzeroone:match"ab", "ab")
  L.assertEq(cabzeroone:match"abab", "ab")
  local a, b, c, d = L.C(L.C"a" * L.C(L.P"b" * L.C"c")):match("abc")
  L.assertEq(a, "abc")
  L.assertEq(b, "a")
  L.assertEq(c, "bc")
  L.assertEq(d, "c")
  L.assertEq(L.C(1):match("ab"), "a")
  L.assertEq(L.C(2):match("ab"), "ab")
  -- argument capture
  local extra1st = L.Carg(1) -- index need >=1 otherwise will throw a error
  local extra2nd = L.Carg(2) -- match function need has enough extra arguments, otherwise will throw a error
  L.assertEq(extra1st:match("", 1, "extra_arg1"), "extra_arg1")
  L.assertEq(extra1st:match("a", 1, 3.14), 3.14)             -- must have at least 1 extra argument
  L.assertEq(extra2nd:match("a", 1, "arg1", "arg2"), "arg2") -- must have at least 2 extra arguments
  L.assertEq(extra2nd:match("a", 1, "arg1", 123), 123)
  -- constant capture
  local ccst = L.Cc("val1", 2, 3) -- related pattern always match success and doesn't consume any characters
  a, b, c = ccst:match("")
  L.assertEq(a, "val1")
  L.assertEq(b, 2)
  L.assertEq(c, 3)
  a = ccst:match("abc")
  L.assertEq(a, "val1")
  -- position capture
  local cpos = L.Cp() * L.P("ab") * L.Cp() * L.P("c") * L.Cp()
  a, b, c = cpos:match("abcd")
  L.assertEq(a, 1)
  L.assertEq(b, 3)
  L.assertEq(c, 4)
  a, b, c = cpos:match("ab")
  L.assertEq(a, nil) -- values captured only when the entire pattern success
  L.assertEq(b, nil)
  L.assertEq(c, nil)
  -- string capture
  local cap = L.C("ab") * L.P("c") * L.C("d") * L.P("e")
  local scap = cap / "%% 1st %1 2nd %2: %0" -- %% stands for %, %0 stands for whole match, %1~%9 n-th capture
  L.assertEq(scap:match"", nil)
  L.assertEq(scap:match"abcd", nil)
  L.assertEq(scap:match"abcdefg", "% 1st ab 2nd d: abcde")
  -- number capture
  local capturenone = cap / 0
  L.assertEq(capturenone:match(""), nil)
  L.assertEq(capturenone:match("abcd"), nil)
  L.assertEq(capturenone:match("abcdefg"), 6) -- no capture value so return match result
  local capture1st = cap / 1
  L.assertEq(capture1st:match(""), nil)
  L.assertEq(capture1st:match("abcd"), nil)
  L.assertEq(capture1st:match("abcdefg"), "ab")
  local capture2nd = cap / 2
  -- local capture3rd = cap / 3  -- the 3rd capture value is not exist, it will report a error
  L.assertEq(capture2nd:match(""), nil)
  L.assertEq(capture2nd:match("abcd"), nil)
  L.assertEq(capture2nd:match("abcdefg"), "d")
  -- query capture
  local t = {}; t["k1"] = "val1"; t["k2"] = "val2"
  local queryc = (L.P("a") * L.C("k1") * L.P("z")) / t
  L.assertEq(queryc:match(""), nil)
  L.assertEq(queryc:match("ak1zz"), "val1")
  queryc = (L.P"k" * L.P"2") / t -- if no capture then use the whole match string to query the table
  L.assertEq(queryc:match("k"), nil)
  L.assertEq(queryc:match("k2z"), "val2")
  queryc = (L.P("a") * L.C("k3") * L.P("z")) / t
  L.assertEq(queryc:match("ak3"), nil)
  L.assertEq(queryc:match("ak3zz"), 5)  -- if query the table failed then no capture value returned
  queryc = L.P("a") * L.C("k1") * L.C("k2") * L.C("k3") / t
  a, b, c = queryc:match("ak1k2k3")
  L.assertEq(a, "val1")
  L.assertEq(b, nil) -- only the 1st capture value is used to query the table, other capture values are throwed
  L.assertEq(c, nil)
  -- function capture
  local function func(str1, str2)
    if str1 == "s1" then return "r1" end
    if str2 == "s2" then return "r2" end
    return
  end  -- all captured values or the whole match string passed as the function's arguments
  L.assertEq((L.P"a" * L.C"s1" * L.P"b" / func):match("as1b"), "r1")
  L.assertEq((L.P"a" * L.C"s" * L.P"b" * L.C"s2" / func):match("asbs2"), "r2")
  L.assertEq((L.P"s" * L.P"1" / func):match("s1"), "r1")
  L.assertEq((L.P"a" * L.C"s" / func):match("aa"), nil)
  L.assertEq((L.P"a" * L.C"s" / func):match("as"), 3)
  -- substitution capture: capture the matched string and all captures are replaced by its value
  local subc = L.Cs(L.P"a" * L.C"b" * L.P"c" / "z%1" * L.P"d" * L.Cc("xy") * L.P"e" * L.Cp() * L.P"f")
  L.assertEq(subc:match("abcdef"), "zbdxye6f")
  -- fold capture
  local function concate(str1, str2)
    return str1 .. str1 .. str2 .. str2
  end
  L.assertEq(L.Cf("a" * L.Cc"12", concate):match("z"), nil)
  L.assertEq(L.Cf("a" * L.Cc"12", concate):match("a"), "12") -- if only one capture value, just return this value
  L.assertEq(L.Cf(L.Cc"1" * L.Cc"2", concate):match(""), "1122") -- concate("1", "2")
  L.assertEq(L.Cf(L.Cc"1" * L.Cc"2" * L.Cc"3", concate):match(""), "1122112233") -- concate(concate("1", "2"), "33")
  -- L.Cf("a") -- need pass a function as argument or error reported
  -- L.Cf("a", concate):match("a") -- fold capture need at least one capture value or error reported
  L.assertEq(L.Cf("a", concate):match(""), nil)
  -- group capture
  local anonygroup = L.Cg(L.C"ab" * L.P"x" * L.C"cd")
  L.assertEq(anonygroup:match"", nil)
  a, b = anonygroup:match"abxcd"
  L.assertEq(a, "ab")
  L.assertEq(b, "cd")
  anonygroup = L.Cg(L.Cc(3.12) * L.Cc(3) * L.Cc(false) * L.Cc(nil) * L.Cc"abc")
  local a, b, c, d, e = anonygroup:match""
  L.assertEq(a, 3.12) -- anonymous group capture returns all capture values in the group
  L.assertEq(b, 3)
  L.assertEq(c, false)
  L.assertEq(d, nil)
  L.assertEq(e, "abc")
  local namedgroup = L.Cg(L.C"a" * L.P"z" * L.C"b", "name")
  L.assertEq(namedgroup:match("azb"), 4) -- named group capture doesn't return any value, unless it is used in Cb or Ct
  L.assertEq(namedgroup:match(""), nil)
  L.assertEq((namedgroup * namedgroup):match("azbazb"), 7)
  -- back capture
  namedgroup = L.Cg(L.C"a" * L.P"z" * L.C"b" * L.P"z" * L.C"c", "group3")
  local backcapture = L.Cb("group3") -- back capture matches the empty string itself
  cap = L.C("1") * backcapture * namedgroup * "2"
  -- a, b, c, d = cap:match("1azbzc2") -- back capture can only be appeared after the named group capture
  cap = L.C("1") * namedgroup * "2" * backcapture
  a, b, c, d = cap:match("1azbzc2") -- if the entire pattern matched, back capture return all capture values in the group
  L.assertEq(a, "1")
  L.assertEq(b, "a")
  L.assertEq(c, "b")
  L.assertEq(d, "c")
  a, b, c, d = cap:match("1azbzc")
  L.assertEq(a, nil)
  L.assertEq(b, nil)
  L.assertEq(c, nil)
  L.assertEq(d, nil)
  -- table capture
  local namedgroup1 = L.Cg(L.Cc(88) * L.Cc(99), "g1")
  local namedgroup2 = L.Cg(L.C"a" * L.P"b" * L.C"c", "g2")
  local tablecap = L.Ct(L.C"1" * L.P"2" * L.C"3" * namedgroup1 * namedgroup2)
  L.assertEq(tablecap:match(""), nil)
  t = tablecap:match("123abc")
  L.assertEq(t[1], "1") -- table capture will return a talbe contains the captured values
  L.assertEq(t[2], "3") -- normal capture values is stored in the sequence
  L.assertEq(t.g1, 88)  -- named group capture is stored in the related group name field
  L.assertEq(t.g2, "a") -- if there are more than one captures in the group capture, only the first one is stored
  tablecap = L.Ct(L.C("a" * L.Cg("b", "key")))
  t = tablecap:match("ab")
  L.assertEq(t[1], "ab")
  L.assertEq(t.key, nil) -- the named group must be a direct child of the Ct, otherwise it will be ignored
  tablecap = L.Ct(L.C("a" * L.Cg(L.C"b" * L.C"c")))
  t = tablecap:match("abc") -- annoymous group capture will return all capture values in the group
  L.assertEq(t[1], "abc")
  L.assertEq(t[2], "b")
  L.assertEq(t[3], "c")
  -- match-time capture
  local function f_matchtime_return_fail(subject, curpos, ...)
    L.logD("Cmt", "f_matchtime_return_fail", subject, curpos, ...)
  end
  local function f_matchtime(subject, curpos, ...)
    L.logD("Cmt", "f_matchtime", subject, curpos, ...)
    return curpos
  end
  local function f_matchtime_return_capture_value(subject, curpos, ...)
    L.logD("Cmt", "f_matchtime_return_capture_value", subject, curpos, ...)
    return curpos, "capture value"
  end
  local function f_call_function_in_the_middle_of_match(subject, curpos, s)
    L.logD("Cmt", "f_call_function_in_the_middle_of_match", subject, curpos, '"' .. s .. '"')
    return true
  end
  -- the function is called with subject_string, current_position_after_match_patt, capture_values or whole_match
  -- the function return false, nil, or no value indicates match failed, match failed pattern will return nil
  -- return true or current_position_after_match_patt indicates the match success and can continue to match next
  -- return a new current position indicates match success and can continue to match at the new position
  -- if the function return values more than one, the extra values will become final captured values of patt
  L.assertEq(L.Cmt(L.P"abc", f_matchtime_return_fail):match("abcde"), nil)
  L.assertEq(L.Cmt(L.P"abc", f_matchtime):match("abcde"), 4)
  L.assertEq(L.Cmt(L.P"abc", f_matchtime_return_capture_value):match("abcde"), "capture value")
  L.assertEq(L.Cmt(L.P"a" * L.C"b" * L.P"c" * L.C"d", f_matchtime):match("abcde"), 5)
  L.assertEq(L.C(L.P"ab" * L.P(f_call_function_in_the_middle_of_match) * L.P"cd"):match("abcde"), "abcd")
end
local function MKDTEST()
  L.assertEq(M.parse(""), "")
  L.assertEq(M.parse(" \r\n"), "") -- blank line
  L.assertEq(M.parse("    indent"), "<pre><code>"..M.NL.."indent"..M.NL.."</code></pre>"..M.NL)
  L.assertEq(M.parse(" para\nline two"), "<p> para"..M.NL.."line two</p>"..M.NL)
  L.assertEq(M.parse(" \npara  \n \npara two"), "<p>para<br></p>"..M.NL.."<p>para two</p>"..M.NL)
  L.assertEq(M.parse(" # h1 "), "<h1>h1</h1>"..M.NL)
  L.assertEq(M.parse(" ### h3 ##### "), "<h3>h3</h3>"..M.NL)
  L.assertEq(M.parse(" h1  \n == "), "<h1>h1</h1>"..M.NL)
  L.assertEq(M.parse("    h2 \n --- "), "<h2>h2</h2>"..M.NL)
  L.assertEq(M.parse(" -- \n --- "), "<h2></h2>"..M.NL.."<hr>"..M.NL)
  L.assertEq(M.parse(" *** \n ___ "), "<hr>"..M.NL.."<hr>"..M.NL)
  L.assertEq(M.parse(" [ a ]: /link/oh my "), "")
  L.assertEq(M.parse(" [b ]: /link/a.com (hint) "), "")
  L.assertEq(M.parse(" [b ]: </link//b.com> "), "")
  L.assertEq(M.parse(" [b ]: /link/a.com (hi\\)nt)"), "")
  L.assertEq(M.parse(" [ c]: /link/a.com 'title\\'text'"), "")
  L.assertEq(M.parse(' [ c]: /link/a.com \n "title\\"text"'), "")
  L.assertEq(M.parse(" [b ]: </link/b.com > (hi\\)nt)"), "")
  L.assertEq(M.parse(" [ c]: < /link/b.com> 'title\\'text'"), "")
  L.assertEq(M.parse(' [ c]: </link/b .com> \n "title\\"text"'), "")
  L.assertEq(M.parse("[a]: /link \n para \n # h"), "<p> para</p>"..M.NL.."<h1>h</h1>"..M.NL)
  L.assertEq(M.parse(" ``` \n    code\n ``` "), "<pre><code>"..M.NL.."    code"..M.NL.."</code></pre>"..M.NL)
  L.assertEq(M.parse("``` clang \none\n``` two \n```"), "<pre><code class=\"clang\">"..M.NL.."one"..M.NL.."``` two "..M.NL.."</code></pre>"..M.NL)
  L.assertEq(M.parse("| Color | RGB \n| --: | :-: | \n | red | f00"), "<table>"..M.NL.."<tr><th>Color</th><th>RGB</th></tr>"..M.NL..
    "<tr><td class=\"right-aligned\">red</td><td class=\"center-aligned\">f00</td></tr>"..M.NL.."</table>"..M.NL)
  L.assertEq(M.parse(" >  quote\n # h"), "<blockquote><p> quote</p>"..M.NL.."<h1>h</h1>"..M.NL.."</blockquote>"..M.NL)
  L.assertEq(M.parse(" > q1 \n >> q2 \n > q2 \n >>> q3 \n q3 \n >> q3"), "<blockquote><p>q1</p>"..M.NL.."<blockquote><p>q2"..M.NL.."q2</p>"..M.NL..
    "<blockquote><p>q3"..M.NL.." q3"..M.NL.."q3</p>"..M.NL.."</blockquote>"..M.NL.."</blockquote>"..M.NL.."</blockquote>"..M.NL)
  L.assertEq(M.parse(" * a \n + b \n 1. c \n - d"), "<ul><li>a</li>"..M.NL.."<li>b</li>"..M.NL.."<li>c</li>"..M.NL.."<li>d</li>"..M.NL.."</ul>"..M.NL)
  L.assertEq(M.parse(" 1. ordered list"), "<ol><li>ordered list</li>"..M.NL.."</ol>"..M.NL)
  L.assertEq(M.parse("* l1\n  1. l2\n  2. l2\n    * l3\n* l1"), "<ul><li>l1<ol><li>l2</li>"..M.NL.."<li>l2<ul><li>l3</li>"..M.NL..
    "</ul></li>"..M.NL.."</ol></li>"..M.NL.."<li>l1</li>"..M.NL.."</ul>"..M.NL)
  L.assertEq(M.parse("* # h\npara"), "<ul><li><h1>h</h1>"..M.NL.."<p>para</p>"..M.NL.."</li>"..M.NL.."</ul>"..M.NL)
end
CORTEST()
PEGTEST()
MKDTEST()
