import numpy as np
import config
import elements as el
import dataloader as dl
import matplotlib.pyplot as plt
import scipy.linalg as la

class Network:
    def __init__(self):

        self.comp_list = dl.elemet_list

        self.numNodes = 0
        for obj in self.comp_list:
            self.numNodes = max(self.numNodes, obj.strnode, obj.stpnode)

        # Known nodes are the nodes whose voltage is fixed by an ideal voltage source.
        # Sources with a series resistance are modelled as Norton equivalents and
        # do not create known nodes.
        known = []
        for obj in self.comp_list:
            if obj.brnType == "S" and obj.ideal:
                if obj.strnode != 0 and obj.stpnode != 0:
                    raise ValueError("An ideal voltage source must have one node connected to ground (node 0)")
                node = obj.stpnode if obj.strnode == 0 else obj.strnode
                if node in known:
                    raise ValueError("Two ideal voltage sources on node %d" % node)
                known.append(node)
        unknown = [n for n in range(1, self.numNodes + 1) if n not in known]

        # Internal node ordering: unknown nodes first, then known nodes, so that
        # G can be partitioned into G_UU, G_UK, G_KU, G_KK regardless of how the
        # user numbered the nodes. idx maps user node number -> internal index
        # (ground -> -1).
        self.order = unknown + known
        self.idx = {n: i for i, n in enumerate(self.order)}
        self.idx[0] = -1
        for obj in self.comp_list:
            obj.fi = self.idx[obj.strnode]
            obj.ti = self.idx[obj.stpnode]

        self.src_cont = len(known)
        self.unknownNodes = self.numNodes - self.src_cont
        self.numBranches = len(self.comp_list)


        self.G = np.zeros((self.numNodes,self.numNodes))

        self.Vn=np.zeros((self.numNodes, 1))
        self.I_H=np.zeros((self.numNodes, 1))

        self.time=[]
        self.trace=[]   # one row per time step: voltages of nodes 1..numNodes


        print("No of ideal sources",self.src_cont,"\n")
        print("No of unknown nodes",self.unknownNodes,"\n")


    def updateSwitches(self,TheTime):
        '''Update all faults/switches. Returns True if any changed state,
        meaning G must be rebuilt and refactorised.'''
        changed = False
        for obj in self.comp_list:
            if isinstance(obj, el.Switch):
                changed = obj.update(TheTime) or changed
        return changed


    def formG(self):
        #print("Generating the conductance matrix...")
        self.G = np.zeros((self.numNodes,self.numNodes))

        for obj in self.comp_list:
            if obj.strnode == 0 and obj.stpnode == 0:
                print("*** Both nodes of a element is zero ***")

            if obj.strnode == obj.stpnode:
                print("*** Both nodes of a element is same ***")

            # Ideal sources have no conductance; their node voltage is imposed via V_K
            if obj.brnType == "S" and obj.ideal:
                continue

            g = 1/obj.Reff
            f = obj.fi
            t = obj.ti
            if f >= 0:
                self.G[f,f] = self.G[f,f] + g
            if t >= 0:
                self.G[t,t] = self.G[t,t] + g
            if f >= 0 and t >= 0:
                self.G[f,t] = self.G[f,t] - g
                self.G[t,f] = self.G[t,f] - g

        print(self.G)

        #print("Conductance matrix:\n ",self.G,"\n")
        #print("IH",self.I_H,"\n")
        #print("Vn",self.Vn,"\n")


    def Gdivide(self):
        print("\nModifying the conductance matrix...\n")

        self.G_UU = self.G[0:self.unknownNodes,0:self.unknownNodes]
        print("G_UU",self.G_UU)
        print("\n")

        self.G_UK = self.G[0:self.unknownNodes,self.unknownNodes:self.numNodes]
        print("G_UK",self.G_UK)
        print("\n")

        self.G_KU = self.G[self.unknownNodes:self.numNodes,0:self.unknownNodes]
        print("G_KU",self.G_KU)
        print("\n")

        self.G_KK = self.G[self.unknownNodes:self.numNodes,self.unknownNodes:self.numNodes]
        print("G_KK",self.G_KK)
        print("\n")


    def LU(self):
        # Keep the full factorisation (L, U and the pivots) for lu_solve
        self.G_UU_LU = la.lu_factor(self.G_UU)
        print("\nLU decom the GUU...\n")
        print("G_UU after LU decom (L and U packed)",self.G_UU_LU[0])
        print("\n")

    def Vdivide(self):
        self.V_K=self.Vn[self.unknownNodes:self.numNodes]
        #print("V_K",self.V_K)
        #print("\n")

        self.V_U=self.Vn[0:self.unknownNodes]
        #print("V_U",self.V_U)
        #print("\n ///////////////////////////////////////// \n")


    def Vkupdate(self,TheTime):
        for obj in self.comp_list:
            if obj.brnType=="S":
                obj.Sourceupdate(TheTime)
                if obj.ideal:
                    # Source raises the potential from strnode to stpnode by E
                    if obj.strnode == 0:
                        self.V_K[obj.ti - self.unknownNodes] = obj.E
                    else:
                        self.V_K[obj.fi - self.unknownNodes] = -obj.E


    def calcBrnHistory(self):
        for obj in self.comp_list:
            if obj.brnType in ("R", "F", "SW"):
               obj.ihistory=0.0
            elif obj.brnType=="S":
                # Norton equivalent: current source E/R in parallel with R
                obj.ihistory = 0.0 if obj.ideal else obj.E/obj.Reff
            elif obj.brnType=="L":
                obj.ihistory=obj.Ilast+obj.Vlast/obj.Reff
            elif obj.brnType=="C":
                obj.ihistory=-obj.Ilast-obj.Vlast/obj.Reff
            else:
                print("Only R L C S elements considered")

            #print("history of branch",obj.ihistory,"\n")

    def calcinjection(self):

        self.I_H=np.zeros((self.numNodes, 1))

        for obj in self.comp_list:
            if obj.brnType == "S" and obj.ideal:
                continue
            # Branch current i = V/Reff + ihistory flows from strnode to stpnode,
            # so ihistory leaves strnode and enters stpnode. Ground (-1) is skipped.
            if obj.fi >= 0:
                self.I_H[obj.fi] = self.I_H[obj.fi] - obj.ihistory
            if obj.ti >= 0:
                self.I_H[obj.ti] = self.I_H[obj.ti] + obj.ihistory

        #print("I_H",self.I_H)

    def Idivide(self):

        self.I_K=self.I_H[self.unknownNodes:self.numNodes]
        #print("I_K",self.I_K)
        #print("\n")

        self.I_U = self.I_H[0:self.unknownNodes]
        #print("I_U",self.I_U)
        #print("\n ///////////////////////////////////////// \n")

    def calIUnew(self):
        self.I_U_new= self.I_U - np.matmul(self.G_UK,self.V_K)
        #print("I_U_new",self.I_U_new)


    def calcnewV(self):
        self.V_U = la.lu_solve(self.G_UU_LU, self.I_U_new)
        #print("V_U",self.V_U)


    def reconstruct_V(self):
        self.Vn=np.concatenate([self.V_U,self.V_K])
        #print("Vn",self.Vn)



    def calcNewbranchI(self):
        for obj in self.comp_list:
            # Branch voltage V = V(strnode) - V(stpnode), ground = 0
            Vfrom = self.Vn[obj.fi,0] if obj.fi >= 0 else 0.0
            Vto = self.Vn[obj.ti,0] if obj.ti >= 0 else 0.0
            V = Vfrom - Vto
            obj.Vlast=V
            if obj.brnType == "S" and obj.ideal:
                continue
            # R: ihistory = 0, Norton source: ihistory = E/R, L/C: trapezoidal history
            obj.Ilast = V/obj.Reff + obj.ihistory




    def recordv(self,TheTime):
        # Store all node voltages in user node order (node 1, 2, ...)
        self.time.append(TheTime)
        self.trace.append([self.Vn[self.idx[n],0] for n in range(1, self.numNodes + 1)])


    def plotv(self):
        from plot import plot_nodes
        faults = [(o.t_apply, o.t_clear) for o in self.comp_list if isinstance(o, el.Fault)]
        switches = [ts for o in self.comp_list
                    if isinstance(o, el.Switch) and not isinstance(o, el.Fault) for ts in o.t_switch]
        plt.close('all')
        plot_nodes(np.array(self.time)*1000.0, np.array(self.trace),
                   ["V%d" % (n + 1) for n in range(self.numNodes)], faults, switches)
        plt.show()
