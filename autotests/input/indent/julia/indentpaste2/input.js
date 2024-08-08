v.setCursorPosition(2, 8);
v.paste(" function hypot(x,y)\n           x = abs(x)\n           y = abs(y)\n           if x > y\n               r = y/x\n               return x*sqrt(1+r*r)\n           end\n           if y == 0\n               return zero(x)\n           end\n           r = x/y\n           return y*sqrt(1+r*r)\n           end");
