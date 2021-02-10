___
![Screenshot](logo.png)
___
___
EMTSim is a simulation program based on algorithm proposed by dommel. 
This simulation program contains only R, L, C elements and voltage sources.
With this program structure, it is easy to add new components to the program. for example, if you want to add  a new element into the program, go to the dataloader.py and add the new component with specified parameters. Available elements are defined in the elments.py file.

Feel free to Fork and improve :D

---
## Steps of the simulation algorithm inside DommelSim
__
1. Generate the conductance matrix G

2. Enter main time loop

3. Divide the G into Guu,Guk,Gku,Gkk
4. Use LU decompossition and obtain upper triangular matrix of Guu
5. Initialize IU,IK,VU and VK
6. Update voltage sources
8. Calculate the history currents of all the branches(ihistory)
9. Calculate the current injection (IH)
10. Divide the IH into IU and IK
11. Calculate IUnew (IUnew=IU-Guk*Vk)
12. Calculate node voltages(VU)
13. Reconstruct Vn (concat VU and VK)
14. Update the branch currents
15. Plot the voltage of specific node

    
___
### This is how you include the circuit elements to the simulation in dataloader.py 
```python
E1=el.Source(brnType="S",strnode=0,stpnode=3,magnitude=100,angle=45,frequency=100,resistance=1)
E2=el.RLC(brnType="L",strnode=3,stpnode=2,value=1.0e-3)
E3=el.RLC(brnType="C",strnode=2,stpnode=1,value=1.0e-3)
E4=el.RLC(brnType="R",strnode=0,stpnode=2,value=1.0e-3)
E5=el.RLC(brnType="L",strnode=0,stpnode=1,value=1.0e-3)
```
___
### This is how you define the circuit elements in elements.py. The goal of this part is to calculate the history current and equivalent resistance of the element for each and every time step. Following code segment shows, how i defined R,L,C elements.

```python
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
```


___
### This is how the main program runs. Each method called in this section is defined in emtcls.py

```python

def run():
    print("\nStart running the simulation..... \n")
    Time=config.srtTime  
    #Initialize the network
    EMTDC=emtcls.Network()
    # Form the conductance matrix
    EMTDC.formG()

    #Enter the main time loop
    while Time<config.stpTime:
        # Divide the Conductance matrix into  GUU, GUK, GKU, GKK
        EMTDC.Gdivide()
        #LU decomposition of GUU
        EMTDC.LU()
        #Divide node voltage vector into VU and VK
        EMTDC.Vdivide()
        #Advance one time step
        Time=Time+config.Dt
        #Update the voltage sources
        EMTDC.Vkupdate(Time)
        #Calculate the history current of each branch       
        EMTDC.calcBrnHistory()
        #Calculate the current injection at each node
        EMTDC.calcinjection()
        #Divide IH in to IU and IK
        EMTDC.Idivide()
        #Calculate new IU
        EMTDC.calIUnew()
        #Calculate new VU
        EMTDC.calcnewV()
        #Reconstruct node voltage vector
        EMTDC.reconstruct_V()
        #Calculate branch currents of each element
        EMTDC.calcNewbranchI()
        #Record the voltage of node 2 or any other node :)
        EMTDC.recordv(2)
      
    EMTDC.plotv()


if __name__ == "__main__":
    run()
```

## Amila Pathirana


