___

# DommelSim by APathirana
___
DommelSim is a open source EMT simulation program based on algorithm proposed by dommel. 
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

###Sample program
```python
def run():
    print("\nStart running the simulation..... \n")
    Time=config.srtTime

    EMTDC=emtcls.Network()
    EMTDC.formG()


    while Time<config.stpTime:
        EMTDC.Gdivide()
        EMTDC.LU()
        EMTDC.Vdivide()
        Time=Time+config.Dt
        EMTDC.Vkupdate(Time)        
        EMTDC.calcBrnHistory()
        EMTDC.calcinjection()
        EMTDC.Idivide()
        EMTDC.calIUnew()
        EMTDC.calcnewV()
        EMTDC.reconstruct_V()
        EMTDC.calcNewbranchI()
        EMTDC.recordv(2)
       
    EMTDC.plotv()


if __name__ == "__main__":
    run()
```



