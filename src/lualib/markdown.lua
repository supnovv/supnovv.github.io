local L = require "thatlcore"
local blanktail = L.blank^0 * L.newline
local blankline = L.C(L.blank^0) * L.newline
local indent = (L.spc*L.spc*L.spc*L.spc + L.tab + L.spc*L.tab + L.spc*L.spc*L.tab + L.spc*L.spc*L.spc*L.tab)
local indentline = L.C(L.C(indent) * L.C(L.blank^0) * L.C((1 - blanktail)^0) * L.C(L.blank^0)) * L.newline
local headspace = L.C((L.spc + L.tab)^0)
local atxstart = headspace * L.C(L.P("#") * L.P("#")^-5)
local atxtail = L.blank^0 * L.P("#")^0 * blanktail
local atxline = L.C(atxstart * L.blank^0 * L.C((1 - atxtail)^0) * L.blank^0 * L.P("#")^0 * L.blank^0) * L.newline
local stxline = L.C(headspace * L.C(L.P("=")^1 + L.P("-")^1) * L.blank^0) * L.newline
local horzline = L.C(headspace * L.C(L.P("*")^3 + L.P("-")^3 + L.P("_")^3) * L.blank^0) * L.newline
local referfmttail = L.blank^0 * L.P"]:"
local referstart = headspace * L.P"[" * L.blank^0 * L.C((1-referfmttail)^0) * referfmttail
local referhintcontent = L.Clinequote("\"") + L.Clinequote("'") + L.Clinequote("(", ")")
local referhint = L.blank^0 * (L.Cc("")*referhintcontent + L.C(L.newline)*L.blank^0*referhintcontent + L.Cc("")*L.Cc("")) * L.blank^0
local referline = L.C(referstart * L.blank^0 * (L.Clinequote("<", ">") + L.C((1-referhint*L.newline)^1)) * referhint) * L.newline
local fencedline = L.C(headspace * L.P"```" * L.blank^0 * L.C(L.nonblank^0) * L.blank^0) * L.newline
local tablestart, columntail = headspace * L.P"|", L.blank^0 * L.P"|"
local tableline = L.C(tablestart * L.P{"col"; col = L.blank^0 * L.C((1 - (columntail + blanktail))^0) * (columntail * L.V"col" + blanktail)})
local quoteline = L.C(headspace * L.C((">" * L.blank^0)^1) * L.C((1 - blanktail)^0) * L.C(L.blank^0)) * L.newline
local liststart = headspace * L.C(L.S("*+-") + L.integer * ".") * L.blank^1
local listline = L.C(liststart * L.C((1 - blanktail)^0) * L.C(L.blank^0)) * L.newline
local paraline = L.C(headspace * L.C((1 - blanktail)^0) * L.C(L.blank^0)) * L.newline
local NEWLINE, LT = "\r\n", {} -- LT (line type)
local LTSTR = {"NULL", "BLAN", "INDE", "ATXH", "STXH", "HORZ", "REFE", "FENC", "FMID", "TABL", "QUOT", "LIST", "PARA"}
for i,s in ipairs(LTSTR) do LT[s] = i end
local paralinebreak = false
local useparatag = true
local function setforcebreak(fb)
  if fb then paralinebreak = true
  else paralinebreak = false end
end
local function setuseparatag(use)
  if use then useparatag = true
  else useparatag = false end
end
local refdefs = {}
local function encode(content, lang)
  -- TODO
  return content
end
local function parse(mdstr)
  local outhtml = ""
  local curblock = LT.NULL
  local block = {}
  local prevline = {}
  local function closeindentblock()
    -- INDE block can contain indentline and blankline
    local html = "<pre><code>"..NEWLINE
    for _,line in ipairs(block) do
      if line.type == LT.INDE then
        html = html..encode(line.content)..NEWLINE
      elseif line.type == LT.BLAN then
        html = html..NEWLINE
      else
        L.logE("invalid line in INDE block " .. line.type)
      end
    end
    outhtml = outhtml..html.."</code></pre>"..NEWLINE
  end
  local function closefencedblock()
    -- FENC block can contain any line
    local lang = block[1].lang
    local html = "<pre><code>"..NEWLINE;
    if #lang > 0 then html = "<pre><code class=\""..lang.."\">"..NEWLINE end
    for i = 2, #block do
      html = html..encode(block[i].content, lang)..NEWLINE
    end
    outhtml = outhtml..html.."</code></pre>"..NEWLINE
  end
  -- <table>
  --   <tr><th></th><th></th></tr>
  --   <tr><td></td><td></td></tr>
  -- </table>
  local function isalignformat(t, i)
    if i > #t then return false end
    local s = t[i]
    for idx = 1, s:len() do
      local c = s:sub(idx,idx)
      if c ~= ":" and c ~= "-" and c ~= " " then
        return false
      end
    end
    return true
  end
  local function getalignstr(t, i)
    if i > #t then return "left-aligned" end
    local s = t[i]
    if s:len() == 1 then
      return "left-aligned"
    end --[[ "----" ":---" ":--:" "---:" ]]
    local left = (s:sub(1,1) == ":")
    local right = (s:sub(-1,-1) == ":")
    if left and right then return "center-aligned" end
    if left then return "left-aligned" end
    return "right-aligned"
  end
  local function columnhtml(t, i)
    if i > #t then return "" end
    return encode(t[i])
  end
  local function closetable()
    -- TABL block can contain tableline and blankline
    local count, cols, aligns = 0, 0, {}
    local html = "<table>"..NEWLINE
    for _,line in ipairs(block) do
      if line.type == LT.TABL then
        count = count + 1
        if count == 1 then
          html = html.."<tr>"
          cols = #(line.columns)
          for i = 1, cols do
            html = html.."<th>"..encode(line.columns[i]).."</th>"
            table.insert(aligns, "left-aligned")
          end
          html = html.."</tr>"..NEWLINE
        elseif count == 2 and isalignformat(line.columns, 1) then
          for i = 1, cols do
            aligns[i] = getalignstr(line.columns, i)
          end
        else
          html = html.."<tr>"
          for i = 1, cols do
            html = html.."<td class=\""..aligns[i].."\">"..columnhtml(line.columns, i).."</td>"
          end
          html = html.."</tr>"..NEWLINE
        end
      end
    end
    outhtml = outhtml..html.."</table>"..NEWLINE
  end
  local function closeparagraph()
    -- PARA block can contain paraline, indentline and blankline
    local i, len = 1, #block
    repeat
      local paralines, html = 0, ""
      while i <= len do -- continue until blank line or end
        if block[i].type == LT.BLAN then break end
        paralines = paralines + 1
        if paralines == 1 then html = html..encode(block[i].content)
        else html = html..NEWLINE..encode(block[i].content) end
        if paralinebreak or block[i].tail:len() >= 2 then
          html = html.."<br>"
        end
        i = i + 1
      end
      if useparatag then html = "<p>"..html.."</p>"..NEWLINE end
      outhtml = outhtml..html
      while i <= len do -- skip blank lines
        if block[i].type == LT.BLAN then i = i + 1 else break end
      end
    until i > len
  end
  local function closequote()
    -- QUOT block can contain any line
    local prevlevel, level, count = 0, 1, 0
    local curquote, blocks = "", 0
    for _,line in ipairs(block) do
      count = count + 1
      L.assert(L.isntype(line.type))
      L.assert(L.isstype(line.content))
      if line.type == LT.QUOT then
        L.assert(L.isntype(line.level))
        if count == 1 then
          level = line.level
          curquote = curquote..line.content..NEWLINE
        else
          if line.level <= level then
            curquote = curquote..line.content..NEWLINE
          else
            for i = 1, level-prevlevel do
              outhtml = outhtml.."<blockquote>"
            end
            blocks = blocks + level-prevlevel
            outhtml = outhtml..parse(curquote)
            prevlevel = level
            level = line.level
            curquote = line.content..NEWLINE
          end
        end
      else
        curquote = curquote..line.content..NEWLINE
      end
    end
    for i = 1, level-prevlevel do
      outhtml = outhtml.."<blockquote>"
    end
    blocks = blocks + level-prevlevel
    outhtml = outhtml..parse(curquote)
    for i = 1, blocks do
      outhtml = outhtml.."</blockquote>"..NEWLINE
    end
  end
  local tagname = {[true]="ol"; [false]="ul"}
  local function parsesingleline(t, a)
    L.assertEq(t[a].type, LT.LIST)
    setuseparatag(false)
    local html = parse(t[a].content)
    setuseparatag(true)
    return html
  end
  local function parsemultiline(t, a, b)
    L.assertEq(t[a].type, LT.LIST)
    local text = t[a].content..t[a].tailblank..NEWLINE
    for i = a+1, b do
      L.assert(t[i].type ~= LT.LIST)
      text = text..t[i].content..NEWLINE -- non-list type has origline text
    end
    return parse(text)
  end
  local function parsecurlist(t, s, e)
    L.assertEq(t[s].type, LT.LIST)
    if s == e then return "<li>"..parsesingleline(t, s).."</li>"..NEWLINE end
    local prev, cur = s, s+1
    local html = ""
    while true do
      while cur <= e do
        if t[cur].type == LT.LIST then
          break
        end
        cur = cur + 1
      end
      if cur > e then
        if cur == prev+1 then html = html.."<li>"..parsesingleline(t, prev).."</li>"..NEWLINE
        else html = html.."<li>"..parsemultiline(t, prev, e).."</li>"..NEWLINE end
        break
      end
      L.assert(t[cur].level >= t[prev].level)
      if t[cur].level > t[prev].level then
        if cur == prev+1 then html = html.."<li>"..parsesingleline(t, prev)
        else html = html.."<li>"..parsemultiline(t, prev, cur-1) end
        local tag = tagname[t[cur].ordered]
        local curlevel = t[cur].level
        local curend = cur
        while curend <= e do
          if t[curend].type == LT.LIST and t[curend].level < curlevel then
            break
          end
          curend = curend + 1
        end
        if curend > e then
          html = html.."<"..tag..">"..parsecurlist(t, cur, e).."</"..tag..">"
          html = html.."</li>"..NEWLINE
          break
        end
        html = html.."<"..tag..">"..parsecurlist(t, cur, curend-1).."</"..tag..">"
        html = html.."</li>"..NEWLINE
        cur = curend
      else
        if cur == prev+1 then html = html.."<li>"..parsesingleline(t, prev).."</li>"..NEWLINE
        else html = html.."<li>"..parsemultiline(t, prev, cur-1).."</li>"..NEWLINE end
      end
      prev = cur
      cur = cur+1
    end
    return html
  end
  local function closelist()
    -- LIST block can contain any line
    L.assert(L.isntype(block[1].level))
    L.assert(L.isbtype(block[1].ordered))
    -- remove tail blank lines
    while #block > 0 and block[#block].type == LT.BLAN do
      table.remove(block)
    end
    -- the first list level must be the min level
    local level = block[1].level
    for i = 1, #block do
      if block[i].type == LT.LIST and block[i].level < level then
        block[i].level = level
      end
    end
    local tag = tagname[block[1].ordered]
    outhtml = outhtml.."<"..tag..">" .. parsecurlist(block, 1, #block) .. "</"..tag..">"..NEWLINE
  end
  local closefunc = {
    [LT.LIST] = closelist;
    [LT.QUOT] = closequote;
    [LT.TABL] = closetable;
    [LT.INDE] = closeindentblock;
    [LT.FENC] = closefencedblock;
    [LT.PARA] = closeparagraph;
  }
  local function closecurblock()
    if curblock == LT.NULL then return end
    closefunc[curblock]()
    curblock = LT.NULL
    block = {}
  end
  local function appendblockline(type, text)
    table.insert(block, {type=type, content=text})
    prevline = {type=type; text=text}
  end
  local function addtocurblock(type, origtext)
    if (curblock == LT.QUOT and prevline.type ~= LT.BLAN) or
       (curblock == LT.LIST and prevline.type ~= LT.BLAN) then
      appendblockline(type, origtext)
      return true
    end
    if curblock == LT.FENC then
      if type == LT.FENC then return false end
      appendblockline(type, origtext)
      return true
    end
    return false
  end
  local function parseatxline(origtext, spaces, strf, text)
    if addtocurblock(LT.ATXH, origtext) then return end
    closecurblock()
    local tagname = "h" .. strf:len()
    outhtml = outhtml.."<"..tagname..">"..encode(text).."</"..tagname..">"..NEWLINE
    prevline = {type=LT.ATXH; text=text}
  end
  local function parsestxline(origtext, spaces, strf)
    if addtocurblock(LT.STXH, origtext) then return end
    local header = ""
    if prevline.type == LT.PARA or prevline.type == LT.INDE then
      header = prevline.text
      table.remove(block)
      if #block == 0 then curblock = LT.NULL end
    end
    closecurblock()
    if header == "" and #strf >= 3 and strf:sub(1,1) == "-" then
      outhtml = outhtml.."<hr>"..NEWLINE
      prevline = {type=LT.HORZ; text=strf}
      return
    end
    local t = {["="]="1", ["-"]="2"}
    local tagname = "h" .. t[strf:sub(1,1)]
    outhtml = outhtml.."<"..tagname..">"..encode(header).."</"..tagname..">"..NEWLINE
    prevline = {type=LT.STXH; text=strf}
  end
  local function parsehorzline(origtext, spaces, strf)
    if addtocurblock(LT.HORZ, origtext) then return end
    closecurblock()
    outhtml = outhtml.."<hr>"..NEWLINE
    prevline = {type=LT.HORZ; text=strf}
  end
  local function parsereferline(origtext, spaces, name, href, newline, hint)
    local firstline = origtext
    local secondline = ""
    if newline ~= nil and newline ~= "" then
      local patt = L.C(L.P(1-L.newline)^0) * L.newline * L.Cp()
      local idx = 1
      firstline, idx = patt:match(origtext, idx)
      secondline, idx = patt:match(origtext, idx)
    end
    if addtocurblock(LT.REFE, firstline) then
      if secondline ~= nil and secondline ~= "" then addtocurblock(LT.REFE, secondline) end
      return
    end
    closecurblock()
    prevline = {type=LT.REFE; text="["..name.."]: <"..href.."> ("..hint..")"}
  end
  local function parseblankline(origtext)
    if curblock ~= LT.NULL then
      table.insert(block, {type=LT.BLAN; content=origtext})
    end
    prevline = {type=LT.BLAN; text=origtext}
  end
  local function parseparaline(origtext, spaces, text, tailblanks)
    if addtocurblock(LT.PARA, origtext) then return end
    if curblock ~= LT.PARA then
      closecurblock()
      curblock = LT.PARA
    end
    table.insert(block, {type=LT.PARA; content=spaces..text; tail=tailblanks})
    prevline = {type=LT.PARA; text=text}
  end
  local function parsetableline(origtext, spaces, ...)
    local patt = L.C((1 - L.newline)^0) * L.newline
    origtext = patt:match(origtext)
    if addtocurblock(LT.TABL, origtext) then return end
    if curblock ~= LT.TABL then
      closecurblock()
      curblock = LT.TABL
    end
    table.insert(block, {type=LT.TABL; columns={...}})
    prevline = {type=LT.TABL; text=origtext}
  end
  local function parsefencedline(origtext, spaces, text)
    local type, isfencblock = LT.FENC, (curblock == LT.FENC)
    if isfencblock and L.isstype(text) and text:len() > 0 then type = LT.FMID end
    if addtocurblock(type, origtext) then return end
    closecurblock()
    if not isfencblock then
      curblock = LT.FENC
      table.insert(block, {type=LT.FENC; lang=""..text})
    end
    prevline = {type=LT.FENC; text=text}
  end
  local function parseindentline(origtext, indent, spaces, text, tailblanks)
    if curblock == LT.FENC or (curblock == LT.QUOT and prevline.type ~= LT.BLAN) then
      table.insert(block, {type=LT.INDE; content=origtext})
      prevline = {type=LT.INDE; text=text}
      return
    end
    if curblock == LT.PARA and prevline.type ~= LT.BLAN then
      table.insert(block, {type=LT.INDE; content=spaces..text; tail=tailblanks})
      prevline = {type=LT.INDE; text=text}
      return
    end
    if curblock == LT.LIST then
      local _, strf, pos = (liststart*L.Cp()):match(text)
      if strf ~= nil then
        local headspace, n = indent..spaces, 0
        for i = 1, headspace:len() do
          if headspace:sub(i,i) == "\t" then n = n + 4
          else n = n + 1 end
        end
        local fmt, ordered = strf:sub(1,1), false
        if fmt ~= "*" and fmt ~= "+" and fmt ~= "-" then ordered = true end
        table.insert(block, {type=LT.LIST; ordered=ordered; level=n/2; content=text:sub(pos); tailblank=tailblanks})
        prevline = {type=LT.LIST; text=strf.." "..text:sub(pos)}
      else
        table.insert(block, {type=LT.INDE; content=spaces..text..tailblanks})
        prevline = {type=LT.INDE; text=text}
      end
      return
    end
    if curblock ~= LT.INDE then
      closecurblock()
      curblock = LT.INDE
    end
    table.insert(block, {type=LT.INDE; content=spaces..text..tailblanks})
    prevline = {type=LT.INDE; text=text}
  end
  local function parsequoteline(origtext, spaces, strf, text, tailblanks)
    if curblock == LT.FENC or (curblock == LT.LIST and prevline.type ~= LT.BLAN) then
      appendblockline(LT.QUOT, origtext)
      return
    end
    local level, lastfmtc = 0, 1
    for i = 1, #strf do
      if strf:sub(i,i) == ">" then
        level = level + 1
        lastfmtc = i
      end
    end
    local headblk = ""
    if lastfmtc < #strf then
      local patt = L.blank * L.C(L.blank^0) / function(blk) headblk = blk end
      patt:match(strf:sub(lastfmtc+1))
    end
    if curblock ~= LT.QUOT then
      closecurblock()
      curblock = LT.QUOT
    end
    table.insert(block, {type=LT.QUOT; level=level; content=headblk..text..tailblanks})
    prevline = {type=LT.QUOT; text=strf..text}
  end
  local function parselistline(origtext, spaces, strf, text, tailblank)
    if curblock == LT.FENC or (curblock == LT.QUOT and prevline.type ~= LT.BLAN) then
      appendblockline(LT.LIST, origtext)
      return
    end
    local t, n = {["\x20"]=1, ["\x09"]=4}, 0
    for i = 1, spaces:len() do n = n + t[spaces:sub(i,i)] end
    local fmt, ordered = strf:sub(1,1), false
    if strf ~= "*" and strf ~= "+" and strf ~= "-" then ordered = true end
    if curblock ~= LT.LIST then
      closecurblock()
      curblock = LT.LIST
    end
    table.insert(block, {type=LT.LIST; ordered=ordered; level=n/2; content=text; tailblank=tailblank})
    prevline = {type=LT.LIST; text=strf.." "..text}
  end
  local function parsecomplete()
    if curblock ~= LT.NULL then
      closecurblock()
    end
    return "eof"
  end
  local patt = L.P(-1) / parsecomplete +  -- if no characters then match complete
    blankline / parseblankline +     -- match blankline first
    indentline / parseindentline +   -- so indentline is not a blank line
    atxline / parseatxline +
    stxline / parsestxline +
    horzline / parsehorzline +
    referline / parsereferline +
    fencedline / parsefencedline +
    tableline / parsetableline +
    quoteline / parsequoteline +
    listline / parselistline +
    paraline / parseparaline
  local previdx, idx = 1, patt:match(mdstr)
  while idx ~= "eof" do
    if L.isntype(idx) then
      L.logD("markdown "..LTSTR[curblock].." add "..LTSTR[prevline.type].." line ["..previdx..","..(idx-1).."]\t:"..prevline.text)
      previdx = idx
      idx = patt:match(mdstr, idx)
    else
      L.logE("markdown parse ["..previdx..",...] failed, curblock "..LTSTR[curblock]..", last line "..LTSTR[prevline.type]..":"..prevline.text)
      break
    end
  end
  return outhtml
end
local function parsemd(s)
  refdefs = {}
  local function parsereferline(origtext, spaces, name, href, newline, hint)
     table.insert(refdefs, {name=name; href=href; hint=hint})
     L.logD("markdown REFE\t\t\t:["..name.."]: <"..href.."> ("..hint..")")
  end
  local patt = L.P(-1) * L.Cc("eof") + referline / parsereferline + L.P(1-L.newline)^0 * L.newline
  local idx = 1
  repeat
    idx = patt:match(s, idx)
  until idx == "eof"
  return parse(s)
end
return {setforcebreak=setforcebreak; parse=parsemd; NL=NEWLINE}
