import sys
d=open('/home/strutb/projects/strutb/aladdinsvhc/build/aladdin/baserom.sfc','rb').read()
off=len(d)&0x3FF
def c(bank,addr): return off+((bank&0x7F)*0x8000)+(addr&0x7FFF)
def r16(a): return d[a]|d[a+1]<<8
lvl=int(sys.argv[1]); want=[int(x,16) for x in sys.argv[2:]]
base=0x8A7A+r16(c(0x82,0x8A7A+2*lvl)); n=d[c(0x82,base)]
for i in range(n):
    e=c(0x82,base+1+6*i)
    if not want or d[e] in want: print(i,'kind %02X byte %02X x %d y %d'%(d[e],d[e+1],r16(e+2)&0x3FFF,r16(e+4)&0x3FFF))
