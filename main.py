import sys
import emtcls
import config
import elements as el
import dataloader as dl
            

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