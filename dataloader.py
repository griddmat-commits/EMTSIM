
import elements as el

# Simple radial feeder with a fault at the load bus
#
#   (1)--Rline--(2)--Lline--(3)
#    |                       |
#   Vs + Rs               Rload || Cload || Fault
#    |                       |
#   ---                     ---
#
# 230 V rms, 50 Hz source with 0.1 ohm internal resistance (Norton model)
# Line: 0.5 ohm + 5 mH
# Load: 20 ohm in parallel with 50 uF
# Bolted 3-to-ground fault (0.01 ohm) applied at 40 ms, cleared at 70 ms

E1=el.Source(brnType="S",strnode=0,stpnode=1,magnitude=325.0,angle=0,frequency=50,resistance=0.1)
E2=el.RLC(brnType="R",strnode=1,stpnode=2,value=0.5)
E3=el.RLC(brnType="L",strnode=2,stpnode=3,value=5.0e-3)
E4=el.RLC(brnType="R",strnode=3,stpnode=0,value=20.0)
E5=el.RLC(brnType="C",strnode=3,stpnode=0,value=50.0e-6)
F1=el.Fault(strnode=3,stpnode=0,Ron=0.01,Roff=1.0e6,t_apply=0.040,t_clear=0.070)


elemet_list=[E1,E2,E3,E4,E5,F1]

# Original test circuit
# E1=el.Source(brnType="S",strnode=0,stpnode=3,magnitude=100,angle=45,frequency=100,resistance=1)
# E2=el.RLC(brnType="L",strnode=3,stpnode=2,value=1.0e-3)
# E3=el.RLC(brnType="C",strnode=2,stpnode=1,value=1.0e-3)
# E4=el.RLC(brnType="R",strnode=0,stpnode=2,value=1.0e-3)
# E5=el.RLC(brnType="L",strnode=0,stpnode=1,value=1.0e-3)
# elemet_list=[E1,E2,E3,E4,E5]
