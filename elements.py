
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


class Switch():
    '''
    Time-controlled switch (ideal breaker) between strnode and stpnode,
    modelled as a resistance: Ron when closed, Roff when open.

    closed   : initial state at t = 0 (True = closed)
    t_switch : list of times (s) at which the switch changes state.
               Each time toggles it: open -> closed or closed -> open.

    Example: a breaker that opens at 30 ms and recloses at 80 ms
        el.Switch(strnode=1, stpnode=2, closed=True, t_switch=[0.030, 0.080])

    Switching is instantaneous at the given time. A real breaker waits for a
    current zero, so opening an inductive current here gives a worst-case
    (current-chopping) overvoltage.
    '''
    def __init__(self,brnType="SW",strnode=None,stpnode=None,Ron=0.01,Roff=1.0e6,closed=True,t_switch=()):
        self.brnType=brnType
        self.strnode=strnode
        self.stpnode=stpnode
        self.Ron=Ron
        self.Roff=Roff
        self.closed0=closed
        self.t_switch=sorted(t_switch)

        self.closed=closed
        self.Reff=Ron if closed else Roff
        self.Ilast=0.0
        self.Vlast=0.0
        self.ihistory=0.0

    def state_at(self,TheTime):
        '''True if the switch is closed at TheTime.'''
        # Small tolerance: Time is accumulated as Time += Dt, so e.g. 200*Dt can land
        # just below 0.02 and the switch would otherwise operate one step late.
        tol = 1e-6*config.Dt
        n = sum(1 for ts in self.t_switch if TheTime >= ts - tol)
        return self.closed0 != (n % 2 == 1)

    def describe(self,closed):
        return "closed" if closed else "opened"

    def update(self,TheTime):
        '''Set the switch state for this time. Returns True if the state changed.'''
        closed = self.state_at(TheTime)
        if closed == self.closed:
            return False
        self.closed = closed
        self.Reff = self.Ron if closed else self.Roff
        print("t = %.5f s: %s %s (nodes %d-%d)" % (TheTime, type(self).__name__.lower(),
              self.describe(closed), self.strnode, self.stpnode))
        return True


class Fault(Switch):
    '''
    Time-controlled fault: a switch between strnode and stpnode
    (stpnode=0 for a fault to ground) that is open before t_apply,
    closed (Ron) from t_apply until t_clear, and open again after t_clear.
    t_clear=None means the fault is never cleared.
    '''
    def __init__(self,brnType="F",strnode=None,stpnode=0,Ron=0.01,Roff=1.0e6,t_apply=None,t_clear=None):
        times = [t_apply] if t_clear is None else [t_apply, t_clear]
        super().__init__(brnType=brnType,strnode=strnode,stpnode=stpnode,Ron=Ron,Roff=Roff,closed=False,t_switch=times)
        self.t_apply=t_apply
        self.t_clear=t_clear

    def describe(self,closed):
        return "applied" if closed else "cleared"
