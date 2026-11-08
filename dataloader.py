
import elements as el

# Two-source, 8-node network with a breaker and a fault
#
#  Source A                                                         Source B
#  (Norton)                                                          (ideal)
#    (1)--R--(2)--L--(3)--SW1--(4)--R--(5)--L--(6)--L--(7)--R--(8)
#     |               |                 |       |                  |
#    Vs1+Rs1      Rld1 || C1           C2   Rld2 || F1            Vs2
#     |               |                 |       |                  |
#    ---             ---               ---     ---                ---
#
# Source A : 230 V rms, 50 Hz, 0 deg, 0.1 ohm internal resistance (Norton model)
# Source B : 230 V rms, 50 Hz, -15 deg, ideal (node 8 is a known node)
# Bus 3    : load A = 25 ohm || 30 uF
# Node 5   : 20 uF line-charging capacitance
# Bus 6    : load B = 30 ohm
#
# Events
#   20 ms : bolted fault (0.01 ohm) bus 6 to ground
#   40 ms : fault cleared
#   55 ms : breaker SW1 opens  -> the two halves of the network separate
#   80 ms : breaker SW1 recloses

# Sources
VS1=el.Source(brnType="S",strnode=0,stpnode=1,magnitude=325.0,angle=0,frequency=50,resistance=0.1)
VS2=el.Source(brnType="S",strnode=0,stpnode=8,magnitude=325.0,angle=-15,frequency=50)

# Source A side
R12=el.RLC(brnType="R",strnode=1,stpnode=2,value=0.2)
L23=el.RLC(brnType="L",strnode=2,stpnode=3,value=3.0e-3)
RLD1=el.RLC(brnType="R",strnode=3,stpnode=0,value=25.0)
C1=el.RLC(brnType="C",strnode=3,stpnode=0,value=30.0e-6)

# Breaker between the two halves
SW1=el.Switch(strnode=3,stpnode=4,Ron=0.01,Roff=1.0e6,closed=True,t_switch=[0.055,0.080])

# Line from bus 4 to bus 6
R45=el.RLC(brnType="R",strnode=4,stpnode=5,value=0.3)
L56=el.RLC(brnType="L",strnode=5,stpnode=6,value=4.0e-3)
C2=el.RLC(brnType="C",strnode=5,stpnode=0,value=20.0e-6)

# Bus 6 load and fault
RLD2=el.RLC(brnType="R",strnode=6,stpnode=0,value=30.0)
F1=el.Fault(strnode=6,stpnode=0,Ron=0.01,Roff=1.0e6,t_apply=0.020,t_clear=0.040)

# Source B side
L67=el.RLC(brnType="L",strnode=6,stpnode=7,value=2.0e-3)
R78=el.RLC(brnType="R",strnode=7,stpnode=8,value=0.2)


elemet_list=[VS1,VS2,R12,L23,RLD1,C1,SW1,R45,L56,C2,RLD2,F1,L67,R78]


# ---------------------------------------------------------------------------
# Previous example: radial feeder with a fault at the load bus (3 nodes)
# E1=el.Source(brnType="S",strnode=0,stpnode=1,magnitude=325.0,angle=0,frequency=50,resistance=0.1)
# E2=el.RLC(brnType="R",strnode=1,stpnode=2,value=0.5)
# E3=el.RLC(brnType="L",strnode=2,stpnode=3,value=5.0e-3)
# E4=el.RLC(brnType="R",strnode=3,stpnode=0,value=20.0)
# E5=el.RLC(brnType="C",strnode=3,stpnode=0,value=50.0e-6)
# F1=el.Fault(strnode=3,stpnode=0,Ron=0.01,Roff=1.0e6,t_apply=0.040,t_clear=0.070)
# elemet_list=[E1,E2,E3,E4,E5,F1]

# Original test circuit
# E1=el.Source(brnType="S",strnode=0,stpnode=3,magnitude=100,angle=45,frequency=100,resistance=1)
# E2=el.RLC(brnType="L",strnode=3,stpnode=2,value=1.0e-3)
# E3=el.RLC(brnType="C",strnode=2,stpnode=1,value=1.0e-3)
# E4=el.RLC(brnType="R",strnode=0,stpnode=2,value=1.0e-3)
# E5=el.RLC(brnType="L",strnode=0,stpnode=1,value=1.0e-3)
# elemet_list=[E1,E2,E3,E4,E5]
