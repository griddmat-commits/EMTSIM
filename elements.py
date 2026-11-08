
import numpy as np
import config

class RLC():
    def __init__(self,brnType=None,strnode=None,stpnode=None,value=None):
        self.brnType=brnType
        self.strnode=strnode
        self.stpnode=stpnode
        self.Ilast=0.0
        self.Vlast=0.0
        self.ihistory=0.0
        

        if self.brnType=="R":
            self.Reff=value
            self.ihistory=0.0
            print(self.Reff)
        elif self.brnType=="L":
            self.Reff=(2*value)/config.Dt
            self.ihistory=self.Ilast+self.Vlast/self.Reff
            print(self.Reff)
        elif self.brnType=="C":
            self.Reff=config.Dt/(2*value)
            self.ihistory=-self.Ilast-self.Vlast/self.Reff
            print(self.Reff)
        else:
            print("R L C only")

class Source():
    def __init__(self,brnType=None,strnode=None,stpnode=None,magnitude=None,angle=None,frequency=None,resistance=None):
        self.brnType = brnType
        self.strnode= strnode
        self.stpnode= stpnode
        self.magnitude= magnitude
        self.angle= angle
        self.frequency= frequency
        # resistance None or 0 -> ideal voltage source (its node becomes a known node)
        # resistance > 0       -> Norton equivalent (E/R current source in parallel with R)
        self.ideal = not resistance
        self.Reff = None if self.ideal else resistance

        self.E=0.0
        self.Ilast=0.0
        self.Vlast=0.0
        self.ihistory=0.0
        print(self.Reff)

    def Sourceupdate(self,TheTime):
        # Instantaneous source voltage: potential of stpnode above strnode
        self.E = self.magnitude*np.cos(2.0*np.pi*self.frequency*TheTime + self.angle*np.pi/180.0)
        #print("sourceupdate method clled",self.E)


class Fault():
    '''
    Time-controlled fault: a resistive switch between strnode and stpnode
    (stpnode=0 for a fault to ground). It is open (Roff) before t_apply,
    closed (Ron) from t_apply until t_clear, and open again after t_clear.
    t_clear=None means the fault is never cleared.
    Clearing is instantaneous at t_clear (no waiting for current zero).
    '''
    def __init__(self,brnType="F",strnode=None,stpnode=0,Ron=0.01,Roff=1.0e6,t_apply=None,t_clear=None):
        self.brnType=brnType
        self.strnode=strnode
        self.stpnode=stpnode
        self.Ron=Ron
        self.Roff=Roff
        self.t_apply=t_apply
        self.t_clear=t_clear

        self.closed=False
        self.Reff=Roff
        self.Ilast=0.0
        self.Vlast=0.0
        self.ihistory=0.0

    def update(self,TheTime):
        '''Set the switch state for this time. Returns True if the state changed.'''
        closed = TheTime >= self.t_apply and (self.t_clear is None or TheTime < self.t_clear)
        if closed == self.closed:
            return False
        self.closed = closed
        self.Reff = self.Ron if closed else self.Roff
        print("t = %.5f s: fault %s (nodes %d-%d)" % (TheTime, "applied" if closed else "cleared", self.strnode, self.stpnode))
        return True

'''
class Switch():
    stoptogle=0
    def __init__(self,brnType,strnode,stpnode,valueon,valueoff,status=1):
        self.brnType=brnType
        self.strnode=strnode
        self.stpnode=stpnode
        self.Ilast=0
        self.Vlast=0
        self.valueon=valueon
        self.valueoff=valueoff
        self.status=status
        self.ihistory=0.0
        

        if self.status==1:
            self.Reff=self.valueon
        elif self.status==0:
            self.Reff=self.valueoff

    def togle(self):
        #print(self.status)
        if stoptogle==0:
            if self.status==1:
                self.status=0
                stoptogle=1

            elif self.status==0:
                self.status=1
                stoptogle=1

        print("Togled",self.status)
        if self.status==1:
            self.Reff=self.valueon
        elif self.status==0:
            self.Reff=self.valueoff


'''              


        