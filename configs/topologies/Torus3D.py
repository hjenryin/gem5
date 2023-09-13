from m5.params import *
from m5.objects import *

from common import FileSystemConfig
import math
from topologies.BaseTopology import SimpleTopology

# Creates a (3-d torus) and directory controllers.
class Torus3D(SimpleTopology):
    description = "3D Torus"

    def __init__(self, controllers):
        self.nodes = controllers

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
        side_length = round(math.pow(num_routers, 1 / 3))
        print("side_length", side_length)
        assert side_length**3 == num_routers

        # Create the routers in the ring
        routers = [
            Router(
                router_id=i,
                latency=router_latency,
                wormhole=options.wormhole,
                spin_enabled=options.spin_enabled,
                num_total=num_routers,
            )
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
            base = i // side_length * side_length
            src_router = routers[i]
            dst_router = routers[base + (i + 1) % side_length]
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

        for i in range(num_routers):
            base = i // (side_length**2) * (side_length**2)
            src_router = routers[i]
            dst_router = routers[base + (i + side_length) % (side_length**2)]
            int_links.append(
                IntLink(
                    link_id=link_count,
                    src_node=src_router,
                    dst_node=dst_router,
                    src_outport="North",
                    dst_inport="South",
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
                    src_outport="South",
                    dst_inport="North",
                    latency=link_latency,
                    weight=1,
                )
            )
            link_count += 1

        for i in range(num_routers):
            src_router = routers[i]
            dst_router = routers[(i + side_length**2) % (side_length**3)]
            int_links.append(
                IntLink(
                    link_id=link_count,
                    src_node=src_router,
                    dst_node=dst_router,
                    src_outport="Up",
                    dst_inport="Down",
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
                    src_outport="Down",
                    dst_inport="Up",
                    latency=link_latency,
                    weight=1,
                )
            )
            link_count += 1

        network.int_links = int_links
