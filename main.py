import sys
import emtcls
import config
import elements as el
import dataloader as dl


def refactor(EMTDC):
    # Form the conductance matrix, divide it into GUU, GUK, GKU, GKK
    # and LU-decompose GUU. Only needed at the start and when a switch/fault
    # changes state, because G is constant otherwise.
    EMTDC.formG()
    EMTDC.Gdivide()
    EMTDC.LU()


def run():
    print("\nStart running the simulation..... \n")
    Time=config.srtTime
    #Initialize the network
    EMTDC=emtcls.Network()
    #Set the initial switch/fault states and factorise G
    EMTDC.updateSwitches(Time)
    refactor(EMTDC)

    #Enter the main time loop
    while Time<config.stpTime:
        #Divide node voltage vector into VU and VK
        EMTDC.Vdivide()
        #Advance one time step
        Time=Time+config.Dt
        #Apply/clear faults; rebuild G if anything switched
        if EMTDC.updateSwitches(Time):
            refactor(EMTDC)
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
        #Record the voltages of all nodes
        EMTDC.recordv(Time)

    EMTDC.plotv()


if __name__ == "__main__":
    run()
