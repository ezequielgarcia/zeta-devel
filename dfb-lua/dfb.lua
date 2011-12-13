require 'directfb'

directfb.DirectFBInit()

local dfb = directfb.DirectFBCreate()

local surface = dfb:CreateSurface({})

surface:Clear( 255, 255, 255, 255 )
surface:Flip({x1=0, x2=0, y1=200, y2=500}, 0)


