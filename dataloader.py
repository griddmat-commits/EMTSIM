
import elements as el

E1=el.Source("S",0,3,100,45,100,1)
E2=el.RLC("L",3,2,1.0e-3)
E3=el.RLC("C",2,1,1.0e-3)
E4=el.RLC("R",0,2,1.0e-3)
E5=el.RLC("L",0,1,1.0e-3)
#self,brnType,strnode,stpnode,valueon,valueoff,togletime,status=1):

elemet_list=[E1,E2,E3,E4,E5]


