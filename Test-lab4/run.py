import multiprocessing
import os
import numpy as np
import re
import matplotlib.pyplot as plt
from tqdm import tqdm
import math


traffic_list = ["uniform_random"]  # "uniform_random", "tornado", "neighbor"
# "Ring", "Torus2d", "Torus3d", "Mesh_XY", "Mesh_westfirst"
topo_list = ["Ring"]
vc_list = [1]  # any integer you want
spin_list = [False]  # True, False
cycle_list = [1000]  # any integer you want
# The list of injection rate.
injR_list = np.round(np.arange(0.00, 0.98, 0.2), 2)
injR_list[0] += 0.01


def file_exists_and_not_empty(file):
    if not os.path.exists(file):
        return False
    if os.path.getsize(file) == 0:
        return False
    return True


def get_fn(traffic, injR, vc, topo, spin, node):
    return f"stats_{traffic}_injR{injR}_vc_{vc}_topo_{topo}_node_{node}{'_spin' if spin else ''}.txt"


dir_name = "run-log/"


def get_cmd(traffic, injR, vc, topo, spin, node):
    if file_exists_and_not_empty(
        dir_name + get_fn(synthetic, injR, vc, topo, spin, node)
    ):
        return ""
    if topo.startswith("Mesh"):
        topo_str = f"--mesh-rows={round(math.sqrt(node))}"
    else:
        topo_str = ""
    if topo == "Mesh_westfirst":
        routing = 0
    else:
        routing = 1
    cmd = f'../build/NULL/gem5.fast -d "{dir_name}" -q --stats-file={get_fn(traffic,injR,vc,topo,spin,node)} ../configs/example/garnet_synth_traffic.py --network=garnet --num-cpus={node} --num-dirs={node} --topology={topo} --inj-vnet=0 --sim-cycles=10000 --injectionrate={injR} --synthetic={traffic} --routing-algorithm {routing} --vcs-per-vnet={vc} {"--spin-enabled" if spin else ""} {topo_str} >/dev/null  2>&1'
    return cmd


img_dir = f"img/{dir_name}"

os.makedirs(img_dir, exist_ok=True)

tail = r"\s+(\d*.?\d+)"
inj_str = r"system.ruby.network.packets_injected::total"
rec_str = r"system.ruby.network.packets_received::total"
recR_str = r"system.ruby.network.reception_rate"
hop_str = r"system.ruby.network.average_hops"
latecy_str = r"system.ruby.network.average_packet_latency"
net_latency_str = r"system.ruby.network.average_packet_network_latency"
# print(re.search(latecy_str,"system.ruby.network.average_packet_latency   923.518975                       (Tick)"))


files_list = [
    (a, b, c, d, e)
    for a in traffic_list
    for b in injR_list
    for c in vc_list
    for d in topo_list
    for e in spin_list
]
array_list = [
    (a, b, c, d)
    for a in traffic_list
    for b in vc_list
    for c in topo_list
    for d in spin_list
]
throughputs = {k: [] for k in array_list}
latencies = {k: [] for k in array_list}
hops = {k: [] for k in array_list}
receiveds = {k: [] for k in array_list}
injecteds = {k: [] for k in array_list}
net_latencies = {k: [] for k in array_list}

pool = multiprocessing.Pool(7)
# use multiprocessing to loop over net_list and inj_rate_list and run the simulation
pbar = tqdm(total=len(files_list))
for arr in files_list:
    synthetic, injR, vc, topo, spin = arr
    if topo == "Ring":
        node = 16
    else:
        node = 64
    cmd = get_cmd(synthetic, injR, vc, topo, spin, node)
    pool.apply_async(os.system, args=(cmd,), callback=lambda _: pbar.update(1))
pool.close()
pool.join()


for file in tqdm(files_list):
    synthetic, injR, vc, topo, spin = file
    if topo == "Ring":
        node = 16
    else:
        node = 64
    lat = 0
    fn = dir_name + get_fn(synthetic, injR, vc, topo, spin, node)
    with open(dir_name + get_fn(synthetic, injR, vc, topo, spin, node)) as f:
        try:
            for line in f.readlines():
                if line.startswith(inj_str):
                    inj = re.search(inj_str + tail, line).group(1)
                if line.startswith(latecy_str):
                    lat = re.search(latecy_str + tail, line).group(1)
                if line.startswith(recR_str):
                    recR = re.search(recR_str + tail, line).group(1)
                if line.startswith(hop_str):
                    hop = re.search(hop_str + tail, line).group(1)

                if line.startswith(rec_str):
                    rec = re.search(rec_str + tail, line).group(1)
                if line.startswith(inj_str):
                    inj = re.search(inj_str + tail, line).group(1)
                if line.startswith(net_latency_str):
                    net_lat = re.search(net_latency_str + tail, line).group(1)
        except:
            hop = np.nan
            rec = np.nan
            inj = np.nan
            recR = np.nan
            lat = np.nan
            net_lat = np.nan
    if lat == 0:
        hop = np.nan
        rec = np.nan
        inj = np.nan
        recR = np.nan
        lat = np.nan
        net_lat = np.nan
    arr = (synthetic, vc, topo, spin)
    hops[arr].append(float(hop))
    throughputs[arr].append(float(recR) * 2)
    latencies[arr].append(float(lat))
    receiveds[arr].append(float(rec))
    injecteds[arr].append(float(inj))
    net_latencies[arr].append(float(net_lat))
for arr in array_list:
    hops[arr] = np.array(hops[arr])
    throughputs[arr] = np.array(throughputs[arr])
    latencies[arr] = np.array(latencies[arr])
    receiveds[arr] = np.array(receiveds[arr])
    injecteds[arr] = np.array(injecteds[arr])
    net_latencies[arr] = np.array(net_latencies[arr])

# pbar=tqdm(total=len(array_list))

# for net in traffic_list:
#     # Create a figure and specify the size
#     fig, axs = plt.subplots(1, 3, figsize=(12, 4))
#     # Set xticks to integer
#     from matplotlib.ticker import MaxNLocator
#     for ax in axs:
#         ax.xaxis.set_major_locator(MaxNLocator(integer=True))
#     # First subplot
#     ax1 = axs[0]
#     # ax1.set_yscale("log")
#     ax1.set_xlabel("Injection Rate")
#     ax1.set_ylabel("Latency (Ticks)")
#     ax1.set_yscale("log")
#     ax1.set_title("\nLatency vs. Injection Rate\nDashed: Network Latency;\n Dotted: Queuing Latency")

#     # Second subplot
#     ax2 = axs[1]
#     ax2.set_xlabel("Injection Rate")
#     ax2.set_ylabel("Throughput (packets/node/cycle)")
#     ax2.set_title("Throughput vs. Injection Rate")
#     ax2.set_yscale("log")

#     # Third subplot
#     ax3 = axs[2]
#     ax3.set_xlabel("Injection Rate")
#     ax3.set_ylabel("Average Hops")
#     ax3.set_title("Average Hops vs. Injection Rate")

#     # Set a global title for all subplots
#     fig.suptitle(f"{net} - 8*8 Mesh", fontsize=16)
#     c=15
#     max_hop=0
#     for vc in vc_list:
#         c-=1
#         for topo, spin in [("Mesh_westfirst",False),("Mesh_XY",True),("Mesh_XY",False)]:
#             arr=(net,vc,topo,spin)
#             mask=np.isfinite(latencies[arr])
#             color=plt.cm.tab20c(c)

#             routing=""
#             if topo=="Mesh_westfirst":
#                 routing="(westfirst)"
#             elif spin:
#                 routing="(spin)"
#             else :
#                 routing=""

#             queue_lat=latencies[arr]-net_latencies[arr]
#             # print(net,topo,vc,spin,len(injR_list),len(latencies[arr]))
#             # First subplot
#             ax1.plot(injR_list[mask], latencies[arr][mask], label=f"#VC={vc} {routing}", color=color)
#             ax1.plot(injR_list[mask], net_latencies[arr][mask], linestyle="--", color=color)
#             ax1.plot(injR_list[mask],queue_lat[mask],linestyle="dotted",color=color)

#             # Second subplot
#             ax2.plot(injR_list[mask], throughputs[arr][mask],color=color)

#             # Third subplot
#             ax3.plot(injR_list[mask], hops[arr][mask],color=color)
#             c-=1
#             pbar.update(1)
#             max_hop=max(max_hop,max(hops[arr][mask]))

#     handles, labels = ax1.get_legend_handles_labels()
#     # Add a single legend for all subplots
#     fig.legend(handles, labels, loc='upper right', ncol=4, bbox_to_anchor=(0.95, 0.95))

#     # Adjust the layout to prevent overlapping of subplots
#     fig.tight_layout()
#     ax3.set_ylim(0,math.floor(max_hop)+1)


#     # Display the figure
#     plt.savefig(f"{img_dir}/{net}-Mesh.png")
#     plt.show()


# exit(0)

for net in traffic_list:
    for topo in topo_list:
        # Create a figure and specify the size
        fig, axs = plt.subplots(1, 3, figsize=(12, 4))
        # Set xticks to integer
        from matplotlib.ticker import MaxNLocator

        for ax in axs:
            ax.xaxis.set_major_locator(MaxNLocator(integer=True))
        # First subplot
        ax1 = axs[0]
        # ax1.set_yscale("log")
        ax1.set_xlabel("Injection Rate")
        ax1.set_ylabel("Latency (Ticks)")
        ax1.set_yscale("log")
        ax1.set_title(
            "Latency vs. Injection Rate\nDashed: Network Latency;\n Dotted: Queuing Latency"
        )

        # Second subplot
        ax2 = axs[1]
        ax2.set_xlabel("Injection Rate")
        ax2.set_ylabel("Throughput (packets/node/cycle)")
        ax2.set_title("Throughput vs. Injection Rate")
        ax2.set_yscale("log")

        # Third subplot
        ax3 = axs[2]
        ax3.set_xlabel("Injection Rate")
        ax3.set_ylabel("Average Hops")
        ax3.set_title("Average Hops vs. Injection Rate")

        # Set a global title for all subplots
        fig.suptitle(f"{net} - {topo}", fontsize=14)
        c = len(spin_list) * len(vc_list) - 1
        max_hop = 0
        for vc in vc_list:
            for spin in spin_list:
                arr = (net, vc, topo, spin)
                mask = np.isfinite(latencies[arr])
                color = plt.cm.tab20(c)

                queue_lat = latencies[arr] - net_latencies[arr]
                print(net, topo, vc, spin, len(injR_list), len(latencies[arr]))
                # First subplot
                ax1.plot(
                    injR_list[mask],
                    latencies[arr][mask],
                    label=f"#VC={vc}, spin {'on' if spin else 'off'}",
                    color=color,
                )
                ax1.plot(
                    injR_list[mask],
                    net_latencies[arr][mask],
                    linestyle="--",
                    color=color,
                )
                ax1.plot(
                    injR_list[mask],
                    queue_lat[mask],
                    linestyle="dotted",
                    color=color,
                )

                # Second subplot
                ax2.plot(injR_list[mask], throughputs[arr][mask], color=color)

                # Third subplot
                ax3.plot(injR_list[mask], hops[arr][mask], color=color)
                c -= 1
                pbar.update(1)
                max_hop = max(max_hop, max(hops[arr][mask]))

        handles, labels = ax1.get_legend_handles_labels()

        # Add a single legend for all subplots
        fig.legend(
            handles,
            labels,
            loc="upper right",
            ncol=4,
            bbox_to_anchor=(0.95, 0.93),
        )

        # Adjust the layout to prevent overlapping of subplots
        fig.tight_layout()
        ax3.set_ylim(0, math.floor(max_hop) + 0.5)

        # Display the figure
        plt.savefig(f"{img_dir}/{net}-{topo}.png")
        plt.show()


# for i,net in enumerate(array_list):
#     color = plt.cm.tab10(i)
#     plt.plot(vcs_list,np.array(latencies[net]),label=net,color=color)
#     plt.plot(vcs_list,np.array(net_latencies[net]),linestyle="--",color=color)
# plt.legend()
# # print(throughputs,latencies)
# plt.xlabel("Injection Rate")
# plt.ylabel("Latency (Ticks)")
# plt.yscale("log")
# plt.title("Latency vs. IInjection Rate\n(Dashed: Network Latency; Solid: Network+Queuing Latency)")
# plt.savefig(f"{img_dir}/2-1_latency_long_log_both.png")
# plt.show()

# for net in array_list:
#     plt.plot(vcs_list,throughputs[net],label=net)
# plt.legend()
# # print(throughputs,latencies)
# plt.xlabel("Injection Rate (Inj. Rate = 0.8)")
# plt.ylabel("Throughput (Packets/Node/Cycle)")
# plt.title("Throughput vs. Injection Rate")
# plt.savefig(f"{img_dir}/2-1_throughput_long.png")
# plt.show()

# for net in array_list:
#     plt.plot(vcs_list,np.array(hops[net]),label=net)
# plt.legend()
# # print(throughputs,latencies)
# plt.xlabel("Injection Rate")
# plt.ylabel("Average Hops")
# plt.title("Avg hops vs. Injection Rate")
# plt.savefig(f"{img_dir}/2-1_hops_long.png")
# plt.show()

# for net in array_list:
#     plt.plot(vcs_list,1-np.array(receiveds[net])/np.array(injecteds[net]),label=net)
# plt.legend()
# plt.ylabel("Package Loss Rate")
# plt.xlabel("Injection Rate")
# plt.savefig(f"{img_dir}/2-1_loss_long.png")
# plt.show()

# # for net in net_list:
# #     plt.plot(vcs_list,injecteds[net],label=net)
# # plt.legend()
# # # print(throughputs,latencies)
# # plt.xlabel("Injection Rate")
# # plt.ylabel("Packages injected")
# # plt.savefig(f"{img_dir}/2-1_inj_long.png")
# # plt.show()

# # for net in net_list:
# #     plt.plot(vcs_list,receiveds[net],label=net)
# # plt.legend()
# # # print(throughputs,latencies)
# # plt.xlabel("Injection Rate")
# # plt.ylabel("Packages received")
# # plt.savefig(f"{img_dir}/2-1_rec_long.png")
# # plt.show()
