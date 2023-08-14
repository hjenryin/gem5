from m5.params import *
from m5.objects import *

from common import FileSystemConfig

from topologies.BaseTopology import SimpleTopology

# Creates a Ring (1-d torus) and directory controllers.
class Ring(SimpleTopology):
    description = "Ring"

    def __init__(self, controllers):
        self.nodes = controllers
        print(controllers)

    def makeTopology(self, options, network, IntLink, ExtLink, Router):
        nodes = self.nodes

        num_routers = options.num_cpus

        # default values for link latency and router latency.
        # Can be over-ridden on a per link/router basis
        link_latency = options.link_latency  # used by simple and garnet
        router_latency = options.router_latency  # only used by garnet

        # There must be an evenly divisible number of cntrls to routers
        assert len(nodes) % num_routers == 0
        cntrls_per_router = len(nodes) // num_routers
        print(len(nodes), num_routers)

        # Create the routers in the ring
        routers = [
            Router(router_id=i, latency=router_latency)
            for i in range(num_routers)
        ]
        network.routers = routers

        # link counter to set unique link ids
        link_count = 0

        # Connect the nodes to the routers in the ring
        ext_links = []
        for (i, n) in enumerate(nodes):
            cntrl_level, router_id = divmod(i, num_routers)
            ext_links.append(
                ExtLink(
                    link_id=link_count,
                    ext_node=n,
                    int_node=routers[router_id],
                    latency=link_latency,
                )
            )
            link_count += 1

        network.ext_links = ext_links

        # Create the ring links.
        int_links = []

        for i in range(num_routers):
            src_router = routers[i]
            dst_router = routers[(i + 1) % num_routers]

            int_links.append(
                IntLink(
                    link_id=link_count,
                    src_node=src_router,
                    dst_node=dst_router,
                    src_outport="East",
                    dst_inport="West",
                    latency=link_latency,
                    weight=1,
                )
            )
            link_count += 1

            int_links.append(
                IntLink(
                    link_id=link_count,
                    src_node=dst_router,
                    dst_node=src_router,
                    src_outport="West",
                    dst_inport="East",
                    latency=link_latency,
                    weight=1,
                )
            )
            link_count += 1

        network.int_links = int_links
