-- SAM686 Version 4

-- place this script at the bottom of level lines,
-- to make sure script runs last, after loading all wall barriers.
-- it is not perfect, uses too much bot zones (2000+), but it works


--   Script BotZones.levelgen
--or Script BotZones.levelgen [totalSize=16] [minZoneSize=0.8]

-- to test a robot in Capture the Flag.
-- Robot 0 quickbotv2.bot .5 1 0 1 .25
-- get quickbotv2 from bitfighter.org forum



-- remove "--" to test if pointCanSeePoint works.
-- Pt1 = Point(100,0)
-- Pt2 = Point(99,1)
-- logprint(levelgen:pointCanSeePoint(Pt1,Pt2) )


function output1(str1)
  -- logprint(str1)
  levelgen:addLevelLine(str1)
end

size=tonumber(arg[1]) or 32
minsize=tonumber(arg[2]) or 0.8

-- GRID SIZE - problem can be caused by comparing while not the grid size.
-- fixed in C++
gridsize=1        --tonumber(arg[3]) or 255


function CalcLinePart(x1,y1,x2,y2)
  local mid=0.5
  local c=0.25
  local main = Point((x1)*gridsize,(y1)*gridsize)
  local test
  while(c > 0.1) do
    test = Point((x2*mid+x1*(1-mid))*gridsize,(y2*mid+y1*(1-mid))*gridsize)
    if(levelgen:pointCanSeePoint(main,test) == true) then
      mid = mid + c
    else
      mid = mid - c
    end
    c = c * 0.5
  end
  if(mid > 0.8) then
    test = Point(x2*gridsize,y2*gridsize)
    if(levelgen:pointCanSeePoint(main,test) == true) then
      return 1.0
    end
  end
  mid = mid - c*2
  return mid
end



function CalcPolygon(w,n,e,s,x1,y1)
  local nw1 = CalcLinePart(x1,y1,w,n)
  local nn1 = CalcLinePart(x1,y1,x1,n)
  local ne1 = CalcLinePart(x1,y1,e,n)
  local ee1 = CalcLinePart(x1,y1,e,y1)
  local se1 = CalcLinePart(x1,y1,e,s)
  local ss1 = CalcLinePart(x1,y1,x1,s)
  local sw1 = CalcLinePart(x1,y1,w,s)
  local ww1 = CalcLinePart(x1,y1,w,y1)
  if((nn1 > 0.4 and ee1 > 0.4 and ss1 > 0.4 and ww1 > 0.4) or s-n <= 0.2 or e-w <= 0.2) then
  local str1 = "BotNavMeshZone"
  if(nw1==1) then
    if(ww1 ~= 1) then str1=str1 .. " " .. w .. " " .. (y1*0.05+n*0.95) end
    str1=str1 .. " " .. w .. " " .. n
    if(nn1 ~= 1) then str1=str1 .. " " .. (x1*0.05+w*0.95) .. " " .. n end
  else
    str1=str1 .. " " .. (w*nw1 + x1*(1-nw1)) .. " " .. (n*nw1 + y1*(1-nw1))
  end
  if(nn1==1) then
    local nnw1 = CalcLinePart(x1,n,w,n)
    local nne1 = CalcLinePart(x1,n,e,n)
    if(nnw1 ~= 1 or nw1 ~= 1) then str1 = str1 .. " " .. (w*nnw1+x1*(1-nnw1)) .. " " .. n end
    if(nne1 ~= 1 or ne1 ~= 1) then str1 = str1 .. " " .. (e*nne1+x1*(1-nne1)) .. " " .. n end
  else
    str1 = str1 .. " " .. x1 .. " " .. (n*nn1 + y1*(1-nn1))
  end
  if(ne1==1) then
    if(nn1 ~= 1) then str1=str1 .. " " .. (x1*0.05+e*0.95) .. " " .. n end
    str1=str1 .. " " .. e .. " " .. n
    if(ee1 ~= 1) then str1=str1 .. " " .. e .. " " .. (y1*0.05+n*0.95) end
  else
    str1 = str1 .. " " .. (e*ne1 + x1*(1-ne1)) .. " " .. (n*ne1 + y1*(1-ne1))
  end
  if(ee1==1) then
    local een1 = CalcLinePart(e,y1,e,n)
    local ees1 = CalcLinePart(e,y1,e,s)
    if(een1 ~= 1 or ne1 ~= 1) then str1 = str1 .. " " .. e .. " " .. (n*een1+y1*(1-een1)) end
    if(ees1 ~= 1 or se1 ~= 1) then str1 = str1 .. " " .. e .. " " .. (s*ees1+y1*(1-ees1)) end
  else
    str1 = str1 .. " " .. (e*ee1 + x1*(1-ee1)) .. " " .. y1
  end
  if(se1==1) then
    if(ee1 ~= 1) then str1=str1 .. " " .. e .. " " .. (y1*0.05+s*0.95) end
    str1=str1 .. " " .. e .. " " .. s
    if(ss1 ~= 1) then str1=str1 .. " " .. (x1*0.05+e*0.95) .. " " .. s end
  else
    str1 = str1 .. " " .. (e*se1 + x1*(1-se1)) .. " " .. (s*se1 + y1*(1-se1))
  end
  if(ss1==1) then
    local ssw1 = CalcLinePart(x1,s,w,s)
    local sse1 = CalcLinePart(x1,s,e,s)
    if(sse1 ~= 1 or se1 ~= 1) then str1 = str1 .. " " .. (e*sse1+x1*(1-sse1)) .. " " .. s end
    if(ssw1 ~= 1 or sw1 ~= 1) then str1 = str1 .. " " .. (w*ssw1+x1*(1-ssw1)) .. " " .. s end
  else
    str1 = str1 .. " " .. x1 .. " " .. (s*ss1 + y1*(1-ss1))
  end
  if(sw1==1) then
    if(ss1 ~= 1) then str1=str1 .. " " .. (x1*0.05+w*0.95) .. " " .. s end
    str1=str1 .. " " .. w .. " " .. s
    if(ee1 ~= 1) then str1=str1 .. " " .. w .. " " .. (y1*0.05+s*0.95) end
  else
    str1 = str1 .. " " .. (w*sw1 + x1*(1-sw1)) .. " " .. (s*sw1 + y1*(1-sw1))
  end
  if(ww1==1) then
    local wwn1 = CalcLinePart(w,y1,w,n)
    local wws1 = CalcLinePart(w,y1,w,s)
    if(wws1 ~= 1 or sw1 ~= 1) then str1 = str1 .. " " .. w .. " " .. (s*wws1+y1*(1-wws1)) end
    if(wwn1 ~= 1 or nw1 ~= 1) then str1 = str1 .. " " .. w .. " " .. (n*wwn1+y1*(1-wwn1)) end
  else
    str1 = str1 .. " " .. (w*ww1 + x1*(1-ww1)) .. " " .. y1
  end
  output1(str1)
  else
    if(nn1+ss1 > ee1+ww1) then ee1=0 ww1=0 else nn1=0 ss1=0 end
  end
  local skip1 = false
  if(s-n > 0.2) then
    if(nn1 < 0.2) then CalcPolygon(w,n,e,(n+s)*0.5,x1,y1*0.5+n*0.5) skip1=true end
    if(ss1 < 0.2) then CalcPolygon(w,(n+s)*0.5,e,s,x1,y1*0.5+s*0.5) skip1=true end
  end
  if(skip1 ~= true and e-w > 0.2) then
    if(ww1 < 0.2) then CalcPolygon(w,n,(w+e)*0.5,s,x1*0.5+w*0.5,y1) end
    if(ee1 < 0.2) then CalcPolygon((w+e)*0.5,n,e,s,x1*0.5+e*0.5,y1) end
  end
end



function CalcBot(w,n,e,s)
  local nw = Point(w*gridsize,n*gridsize)
  local ne = Point(e*gridsize,n*gridsize)
  local sw = Point(w*gridsize,s*gridsize)
  local se = Point(e*gridsize,s*gridsize)
  local nwd = levelgen:pointCanSeePoint(nw,se)
  local ned = levelgen:pointCanSeePoint(ne,sw)
  local ww = levelgen:pointCanSeePoint(nw,sw)
  local ee = levelgen:pointCanSeePoint(ne,se)
  local nn = levelgen:pointCanSeePoint(nw,ne)
  local ss = levelgen:pointCanSeePoint(sw,se)
  if(nwd == true
 and ned==true
 and nn==true
 and ss==true
 and ee==true
 and ww==true) then
    output1("BotNavMeshZone " .. w .. " " .. n .. " " .. e .. " " .. n .. " " .. e .. " " .. s .. " " .. w .. " " .. s);
  else
    if(e > w+minsize and s > n+minsize) then
      local midx = (w + e) * 0.5
      local midy = (n + s) * 0.5
      CalcBot(w,n,midx,midy)
      CalcBot(w,midy,midx,s)
      CalcBot(midx,n,e,midy)
      CalcBot(midx,midy,e,s)
    else
      CalcPolygon(w,n,e,s,(w + e) * 0.5, (n + s) * 0.5)
    end
  end
end


CalcBot(-size,-size,size,size)
