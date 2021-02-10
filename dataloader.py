
import elements as el

E1=el.Source(brnType="S",strnode=0,stpnode=3,magnitude=100,angle=45,frequency=100,resistance=1)
E2=el.RLC(brnType="L",strnode=3,stpnode=2,value=1.0e-3)
E3=el.RLC(brnType="C",strnode=2,stpnode=1,value=1.0e-3)
E4=el.RLC(brnType="R",strnode=0,stpnode=2,value=1.0e-3)
E5=el.RLC(brnType="L",strnode=0,stpnode=1,value=1.0e-3)


elemet_list=[E1,E2,E3,E4,E5]


