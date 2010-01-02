//-----------------------------------------------------------------------------------
//
// Bitfighter - A multiplayer vector graphics space game
// Based on Zap demo released for Torque Network Library by GarageGames.com
//
// Derivative work copyright (C) 2008 Chris Eykamp
// Original work copyright (C) 2004 GarageGames.com, Inc.
// Other code copyright as noted
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful (and fun!),
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//------------------------------------------------------------------------------------

Global PLAYFIELDW:Int = 1024 '1280  '2^8*5         ' Width of entire game board
Global PLAYFIELDH:Int = 768 '1024  '2^8*2*2


Global GRIDWIDTH:Int = 16   '4,5,8,10,16,20,32,40,64
Global GRIDHEIGHT:Int = 16  '4,8,16,32,64,128,256

Const GRIDHILIGHT:Int = 4

Global NUMGPOINTSW:Int = PLAYFIELDW/GRIDWIDTH
Global NUMGPOINTSH:Int = PLAYFIELDH/GRIDHEIGHT


Global grid:gridpoint[,]
grid = New gridpoint[5120/4+2, 4096/4+2] ' maximum size

Local a:Int,b:Int
For a = 0 To 1920/4
   For b = 0 To 1200/4
      grid[a,b] = New gridpoint
   Next
Next

gridpoint.ResetAll()


   gridpoint.Pull(px,py,32,64)



      gwlow = 0
      gwhi = NUMGPOINTSW
      ghlow = 0
      ghhi = NUMGPOINTSH



      ' draw lines [diagonal,raspberry,stretch]

        SetAlpha Abs(g_opacity)
        SetColor g_red,g_green,g_blue
        SetBlend LIGHTBLEND
        DrawGridLines3(g_opacity)

Global moiAkt:Float=0.0,moiAkt2:Float=0.0




'the background dots
Type gridpoint

   Field ox#,oy#
   Field x#
   Field y#
   Field dx#,dy#

   Method Update(xx#,yy#)

      If Abs(xx-x) > 2 Then dx:+ Sgn(xx-x)
      If Abs(yy-y) > 2 Then dy:+ Sgn(yy-y)

      If Abs(ox-x) > 1
         x = x + Sgn(ox-x)
         dx:+ Sgn(ox-x)/2
      Else
         x = ox
      EndIf
      If Abs(oy-y) > 1
         y = y + Sgn(oy-y)
         dy:+ Sgn(oy-y)/2
      Else
         y = oy
      EndIf

      dx = dx *.899 '.89
      dy = dy *.899 '.89

      x = x + dx
      y = y + dy

   End Method


   Function ResetAll()
      Local a:Int,b:Int

      For a = 0 To NUMGPOINTSW
         For b = 0 To NUMGPOINTSH
            grid[a,b].ox = a*GRIDWIDTH
            grid[a,b].oy = b*GRIDHEIGHT
            grid[a,b].x = a*GRIDWIDTH
            grid[a,b].y = b*GRIDHEIGHT
            grid[a,b].dx = 0
            grid[a,b].dy = 0
         Next
      Next
   End Function

   Method disrupt(xx#,yy#)
         If Abs(xx) > 8 Then xx = xx/16
         If Abs(yy) > 8 Then yy = yy/16
         dx = dx + xx
         dy = dy + yy
         Local speed# = dx*dx+dy*dy
         If speed > 160 ' 128
            dx = dx/speed*128
            dy = dy/speed*128
         EndIf
'     EndIf
   End Method

   Function Pull(x1#,y1#, sz:Int = 4,amnt#=4)

      Local a:Int = x1/GRIDWIDTH
      Local b:Int = y1/GRIDHEIGHT
      For Local xx:Int = -sz To sz
         For Local yy:Int = -sz To sz
            If a+xx > 0
               If a+xx =< NUMGPOINTSW'-2
                  If b+yy > 0
                     If b+yy =< NUMGPOINTSH'-2
                        If xx*xx + yy*yy < sz*sz
                           Local diffx# = grid[a+xx,b+yy].x-x1
                           Local diffy# = grid[a+xx,b+yy].y-y1
                           Local dist# = Sqr(diffx*diffx+diffy*diffy)
                           If dist > 0
'                          grid[a+xx,b+yy].fx:- diffx*(1-(dist)/(sz*sz*4*256))
'                          grid[a+xx,b+yy].fy:- diffy*(1-(dist)/(sz*sz*4*256))
                           grid[a+xx,b+yy].dx:- diffx/dist*amnt  '*(1-(dist*dist)/(sz*sz*4*256))
                           grid[a+xx,b+yy].dy:- diffy/dist*amnt  '*(1-(dist*dist)/(sz*sz*4*256))
'                          grid[a+xx,b+yy].fx = - diffx/dist*(1-(dist*dist)/(sz*sz*4*256))
'                          grid[a+xx,b+yy].fy = - diffy/dist*(1-(dist*dist)/(sz*sz*4*256))
                           EndIf
                        EndIf
                     EndIf
                  EndIf
               EndIf
            EndIf
         Next
      Next

   End Function


   Function Push(x1#,y1#, sz:Int = 4,amnt#=1)

      Local a:Int = (x1/GRIDWIDTH)
      Local b:Int = (y1/GRIDHEIGHT)
      For Local xx:Int = -sz To sz
         For Local yy:Int = -sz To sz
         '  If (xx*xx + yy*yy) < sz*sz
            If a+xx > 0
               If a+xx =< NUMGPOINTSW '-2
                  If b+yy > 0
                     If b+yy =< NUMGPOINTSH'-2
                        Local diffx# = grid[a+xx,b+yy].ox-x1
                        Local diffy# = grid[a+xx,b+yy].oy-y1
                        Local diffxo# = grid[a+xx,b+yy].ox-grid[a+xx,b+yy].x
                        Local diffyo# = grid[a+xx,b+yy].oy-grid[a+xx,b+yy].y
                        Local dist# = diffy*diffy+diffx*diffx
                        Local disto# = diffyo*diffyo+diffxo*diffxo
                        If dist > 1 And disto < 400
                           If dist < 50*50
                              grid[a+xx,b+yy].dx:+ diffx*amnt '/dist*amnt
                              grid[a+xx,b+yy].dy:+ diffy*amnt '/dist*amnt
                           EndIf
                        EndIf
                     EndIf
                  EndIf
               EndIf
            'EndIf
            EndIf
         Next
      Next

   End Function


   Function UpdateGrid()

      For Local a:Int = 1 To NUMGPOINTSW-1
         For Local b:Int = 1 To NUMGPOINTSH-1
            Local xx# = 0
            xx:+ grid[a-1,b].x
            xx:+ grid[a,b-1].x
            xx:+ grid[a,b+1].x
            xx:+ grid[a+1,b].x
            xx = xx / 4

            Local yy# = 0
            yy:+ grid[a-1,b].y
            yy:+ grid[a,b-1].y
            yy:+ grid[a,b+1].y
            yy:+ grid[a+1,b].y
            yy = yy / 4

            grid[a,b].update(xx,yy)

         Next
      Next
   End Function


Function DrawGrid(style:Int,small:Int = False)

      If fullgrid
              gwlow = 0
              gwhi = NUMGPOINTSW
              ghlow = 0
              ghhi = NUMGPOINTSH
      Else
              gwlow = Max(gxoff/GRIDWIDTH,0)
              gwhi = Min(gwlow+(ggScreenWidtheight/GRIDWIDTH)+GRIDHILIGHT,NUMGPOINTSW)
              ghlow = Max(gyoff/GRIDHEIGHT,0)
              ghhi = Min(ghlow+(gScreenWidth/GRIDHEIGHT)+GRIDHILIGHT,NUMGPOINTSH)
      EndIf
      If small
             gxoff = -ggScreenWidtheight/8
             gyoff = -gScreenWidth/8
              gwlow = 0
              gwhi = -gxoff/GRIDWIDTH*6
              ghlow = 0
              ghhi = -gyoff/GRIDHEIGHT*6
      EndIf



         ' Draw the grid

                 ' line quads, solid
                 SetAlpha Abs(g_opacity)
                 SetColor g_red,g_green,g_blue
                 SetBlend LIGHTBLEND
                 DrawGridLines3(g_opacity)

      SetScale 1,1
      SetAlpha 1
      SetLineWidth 2

   End Function



   Function DrawGridLines3(alpha#)
      Local a:Int,b:Int
      Local boldw:Int
      Local boldh:Int

      boldw = GRIDHILIGHT-(gwlow Mod GRIDHILIGHT)
      boldh = GRIDHILIGHT-(ghlow Mod GRIDHILIGHT)

      SetScale 1,1
      SetLineWidth 1
      For a = gwlow To gwhi - 1
         If (a+boldh) Mod GRIDHILIGHT = 0
            SetAlpha alpha+.25
         Else
            SetAlpha alpha
         EndIf
         glBegin GL_LINE_STRIP
         For b = ghlow To ghhi-1
            glVertex3f(grid[a,b].x-gxoff,     grid[a,b].y-gyoff,     0)
            glVertex3f(grid[a,b+1].x-gxoff,   grid[a,b+1].y-gyoff,   0)
         Next
         glEnd
      Next
      For b = ghlow To ghhi - 1
         If (b+boldw) Mod GRIDHILIGHT = 0
            SetAlpha alpha+.25
         Else
            SetAlpha alpha
         EndIf
         glBegin GL_LINE_STRIP
         For a = gwlow To gwhi-1
            glVertex3f(grid[a,b].x-gxoff,     grid[a,b].y-gyoff,     0)
            glVertex3f(grid[a+1,b].x-gxoff,   grid[a+1,b].y-gyoff,   0)
         Next
         glEnd
      Next
   End Function


EndType






'particles
Type part

   Field x#,y#,dx#,dy#,r:Int,g:Int,b:Int
   Field active:Int

   Function CreateAll()
      Local t:Int
      For t = 0 To MAXPARTICLES-1
         partarray[t] = New part
         partarray[t].x = 0
         partarray[t].y = 0
         partarray[t].r = 0
         partarray[t].g = 0
         partarray[t].b = 0
         partarray[t].active = 0
         partarray[t].dx = 0
         partarray[t].dy = 0
         Part_list.addlast( partarray[t] )
      Next
      slotcount = 0
   End Function

   Function Create( x#, y# ,typ:Int, r:Int,g:Int,b:Int, rot:Float = 0, sz:Int = 1)
'  Function Create( x#, y# ,typ:Int, r:Int,g:Int,b:Int, rot:Int = 0, sz:Int = 1)
         Local p:Part
         Local flag:Int
         Local dir:Int, mag#

         p:Part = partarray[slotcount]
         p.x = x
         p.y = y
         p.r = r
         p.g = g
         p.b = b
         p.active = Rand(particlelife-20,particlelife)
         Select typ
            Case 0
               ' random
               dir = Rand(0,359)
               mag# = Rnd(3,10)
               p.dx = Cos(dir)*mag
               p.dy = Sin(dir)*mag
            Case 1
               mag# = 16
               p.dx = Cos(rot)*mag
               p.dy = Sin(rot)*mag
               p.active = 24
            Case 2
               dir = rot
               mag# = 8
               p.dx = Cos(dir)*mag
               p.dy = Sin(dir)*mag
            Case 8
               ' 3 dirs
               dir = 120*Rand(0,2)+rot
               mag# = Rnd(3,10)
               p.dx = Cos(dir)*mag
               p.dy = Sin(dir)*mag
            Case 3
               ' 4 dirs
               dir = 90*Rand(0,3)+rot
               mag# = Rnd(3,10)
               p.dx = Cos(dir)*mag
               p.dy = Sin(dir)*mag
            Case 6
               ' 8 dirs
               dir = 45*Rand(0,7)+rot
               mag# = Rnd(3,10)
               p.dx = Cos(dir)*mag
               p.dy = Sin(dir)*mag
            Case 7
               ' any dir and speed
               mag# = Rnd(.5,1)
               p.dx = Cos(rot)*mag
               p.dy = Sin(rot)*mag
               ' evil!
            Case 9
               ' bomb internal particles
               dir = Rand(0,359)
               mag# = Rnd(1,13)
               p.dx = Cos(dir)*mag
               p.dy = Sin(dir)*mag
               ' /evil
         End Select
         p.dx = p.dx*2
         p.dy = p.dy*2
         p.x:+ p.dx*sz
         p.y:+ p.dy*sz
         slotcount:+1
         If slotcount > numparticles-1 Then slotcount = 0
   EndFunction


   Method UpdateWide()
      If active > 0
         x = x + dx
         y = y + dy
         If x =< dx
            dx = Abs(dx)
            x = x + dx*2
         EndIf
         If x > ggScreenWidtheight-1-dx
            dx = -Abs(dx)
            x = x + dx*2
         EndIf
         If y =< dy
            dy = Abs(dy)
            y = y + dy*2
         EndIf
         If y > gScreenWidth-1-dy
            dy = -Abs(dy)
            y = y + dy*2
         EndIf
         dx = dx *particledecay
         dy = dy *particledecay
         active:-1
         If active < 20
            If active < 10
               r:*.8';If r < 0 Then r = 0
               g:*.8';If g < 0 Then g = 0
               b:*.8';If b < 0 Then b = 0
            Else
               r:*.97';If r < 0 Then r = 0
               g:*.97';If g < 0 Then g = 0
               b:*.97';If b < 0 Then b = 0
            EndIf
         ElseIf active > 200
            active = 200
         EndIf
      EndIf
   End Method


   Method Update()
      If active > 0
         x = x + dx
         y = y + dy
         If x =< dx
            dx = Abs(dx)
            x = x + dx*2
         EndIf
         If x > PLAYFIELDW-1-dx
            dx = -Abs(dx)
            x = x + dx*2
         EndIf
         If y =< dy
            dy = Abs(dy)
            y = y + dy*2
         EndIf
         If y > PLAYFIELDH-1-dy
            dy = -Abs(dy)
            y = y + dy*2
         EndIf
         dx = dx *particledecay
         dy = dy *particledecay
         active:-1
         If active < 20
            If active < 10
               r:*.8';If r < 0 Then r = 0
               g:*.8';If g < 0 Then g = 0
               b:*.8';If b < 0 Then b = 0
            Else
               r:*.97';If r < 0 Then r = 0
               g:*.97';If g < 0 Then g = 0
               b:*.97';If b < 0 Then b = 0
            EndIf
         ElseIf active > 200
            active = 200
         EndIf
      EndIf
   End Method


   Function DrawParticles()
      Local p:part
      Local t:Int

      Select particlestyle

         Case 0
            SetBlend lightblend
            SetScale 2,2
            SetAlpha 1
            SetLineWidth 1.0
            For t = 0 To numparticles-1
               p:part = partarray[t]
               If p.active > 0
                  Local rr%,gg%,bb%
                  rr = p.r*1.25;If rr>255 Then rr = 255
                  gg = p.g*1.25;If gg>255 Then gg = 255
                  bb = p.b*1.25;If bb>255 Then bb = 255
                  SetColor rr,gg,bb
                  DrawLine p.x-gxoff,p.y-gyoff,p.x-gxoff+p.dx,p.y-gyoff+p.dy
               EndIf
            Next
            SetAlpha 1
            SetLineWidth 2.0
            SetScale 1,1
         Case 1
            SetBlend lightblend
            SetScale 2,2 '3,3
            SetAlpha .9
            SetLineWidth 2
            For t = 0 To numparticles-1
               p:part = partarray[t]
               If p.active > 0
                  Local rr%,gg%,bb%
                  rr = p.r*1.25;If rr>255 Then rr = 255
                  gg = p.g*1.25;If gg>255 Then gg = 255
                  bb = p.b*1.25;If bb>255 Then bb = 255
                  SetColor rr,gg,bb
                  DrawLine p.x-gxoff,p.y-gyoff,p.x-gxoff+p.dx,p.y-gyoff+p.dy
               EndIf
            Next
            SetAlpha 1
            SetLineWidth 2.0
            SetScale 1,1
         Case 2
            SetBlend lightblend
            SetScale .5,.5
            For t = 0 To numparticles-1
               p:part = partarray[t]
               If p.active > 0
                  Local rr%,gg%,bb%
                  rr = p.r*1.5;If rr>255 Then rr = 255
                  gg = p.g*1.5;If gg>255 Then gg = 255
                  bb = p.b*1.5;If bb>255 Then bb = 255
                  SetColor rr,gg,bb
                  'SetAlpha .7
                  'DrawImage particleimg,p.x-gxoff,p.y-gyoff
                  SetAlpha 1 '.9
                  DrawImage particleimg,p.x-gxoff+p.dx,p.y-gyoff+p.dy
               EndIf
            Next
            SetAlpha 1
            SetScale 1,1
         Case 3  'bloom lines
            Local win:Float,px:Float,py:Float',dx:Float,dy:Float
            Local rr:Int,gg:Int,bb:Int

            SetBlend lightblend
            SetLineWidth 2
            SetAlpha .8
            SetTransform 0,2,2

            For t = 0 To numparticles-1
               p:part = partarray[t]
               If p.active > 0
                  rr = p.r*1.25;If rr>255 Then rr = 255
                  gg = p.g*1.25;If gg>255 Then gg = 255
                  bb = p.b*1.25;If bb>255 Then bb = 255
                  SetColor rr,gg,bb
                  px=p.x-gxoff;py=p.y-gyoff
                  DrawLine px,py,px+p.dx,py+p.dy
               EndIf
            Next
            For t = 0 To numparticles-1
               p:part = partarray[t]
               If p.active > 0
                  rr = p.r*1.25;If rr>255 Then rr = 255
                  gg = p.g*1.25;If gg>255 Then gg = 255
                  bb = p.b*1.25;If bb>255 Then bb = 255
                  SetColor rr,gg,bb

                  win=ATan(p.dy/p.dx)
                  px=p.x-gxoff;py=p.y-gyoff
                  SetAlpha .25
                  SetTransform win,Sqr(p.dx*p.dx+p.dy*p.dy)*.4,1.2
                  DrawImage particleimg,px+p.dx*1.0,py+p.dy*1.0
               EndIf
            Next
            SetAlpha 1
            SetTransform 0,1,1
            SetLineWidth 2.0

      End Select

   End Function


   Function UpdateParticles(ww:Int=0)
      Local p:part,t:Int
      If ww
         For t = 0 To numparticles-1
            p:part = partarray[t]
            If p.active > 0
               p.UpdateWide()
            EndIf
         Next
      Else
         For t = 0 To numparticles-1
            p:part = partarray[t]
            If p.active > 0
               p.Update()
            EndIf
         Next
      EndIf
   End Function


   Function CreateFireWorks(style:Int)
      Local t:Int,x:Int,y:Int,r:Int,g:Int,b:Int
      r = Rand(0,3)*64
      g = Rand(0,3)*64
      b = Rand(0,3)*64
      If style = 1
         If Rand(0,1)
            x = Rand(100,ggScreenWidtheight-100)
            y = 16
            If Rand(0,1) Then y = gScreenWidth-16
         Else
            y = Rand(50,gScreenWidth-50)
            x = 16
            If Rand(0,1) Then x = ggScreenWidtheight-16
         EndIf
      ElseIf style = 2
         x = ggScreenWidtheight/2
         y = gScreenWidth/2
      Else
         x = Rand(100,ggScreenWidtheight-100)
         y = Rand(50,gScreenWidth-50)
      EndIf
      For t = 0 To 63
         part.Create(x,y,0,r,g,b)
      Next
   End Function


   Function ResetAll()
      Local p:Part
      Local t:Int

      For t = 0 To MAXPARTICLES-1
         p:Part = partarray[t]
         p.x = 0
         p.y = 0
         p.r = 0
         p.g = 0
         p.b = 0
         p.active = 0
         p.dx = 0
         p.dy = 0
      Next
      slotcount = 0
   End Function

End Type
