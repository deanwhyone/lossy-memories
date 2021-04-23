#! /usr/bin/python3

import sys
import subprocess

if __name__ == '__main__':
    assert(len(sys.argv) == 5)

    iterations = int(sys.argv[1])
    L1_hit_latency = int(sys.argv[2])
    L2_hit_latency = int(sys.argv[3])
    Mem_hit_latency = int(sys.argv[4])

    l1_sizes = ["16", "32", "64", "128"]
    l2_sizes = ["256", "512", "1024", "2048"]

    AMAT_avgs = {}

    for l1s in l1_sizes:
        for l2s in l2_sizes:

            print("Running test %d times. L1 Size is %s kB, L2 Size is %s kB" %
                (iterations, l1s, l2s))

            L1I_load_hits = []
            L1I_load_misses = []

            L1D_load_hits = []
            L1D_load_misses = []

            L1D_store_hits = []
            L1D_store_misses = []

            L2_load_hits = []
            L2_load_misses = []

            L2_store_hits = []
            L2_store_misses = []

            AMAT_Imem = []
            AMAT_Dmem = []
            data_fraction = []
            AMAT_total = []


            for itr in range(iterations):
                # call test function, edit this to run different files
                subprocess.run(args=[
                    "../../../pin",
                    "-t",
                    "obj-intel64/lcache.so",
                    "-l1s", l1s,
                    "-l2s", l2s,
                    "-compression", "2",
                    "--",
                    "../../../../benchmarks/sobel_benchmark/sobel",
                    "../../../../benchmarks/sobel_benchmark/Super-Mario-Avatar-500x500.jpg"
                    ], universal_newlines=False, stdout=subprocess.DEVNULL);

                # parse output file
                out_file = open("lcache.out", "r");
                out_lines = out_file.readlines()
                out_file.close()
                for line_idx in range(len(out_lines)):
                    if ("L1 Instruction Cache" in out_lines[line_idx]):
                        L1I_hit_data = out_lines[line_idx + 1].split()
                        L1I_load_hits.append(int(L1I_hit_data[2]))

                        L1I_miss_data = out_lines[line_idx + 2].split()
                        L1I_load_misses.append(int(L1I_miss_data[2]))

                        L1I_total = int(L1I_hit_data[2]) + int(L1I_miss_data[2])

                        L1I_miss_rate = int(L1I_miss_data[2]) / L1I_total
                        print("Iteration %d L1I miss rate = %f" % (itr, L1I_miss_rate))

                    if ("L1 Data Cache" in out_lines[line_idx]):
                        L1D_load_hit_data = out_lines[line_idx + 1].split()
                        L1D_load_hits.append(int(L1D_load_hit_data[2]))

                        L1D_load_miss_data = out_lines[line_idx + 2].split()
                        L1D_load_misses.append(int(L1D_load_miss_data[2]))

                        L1D_total_loads = int(L1D_load_hit_data[2]) + int(L1D_load_miss_data[2])

                        L1D_store_hit_data = out_lines[line_idx + 5].split()
                        L1D_store_hits.append(int(L1D_store_hit_data[2]))

                        L1D_store_miss_data = out_lines[line_idx + 6].split()
                        L1D_store_misses.append(int(L1D_store_miss_data[2]))

                        L1D_total_stores = int(L1D_store_hit_data[2]) + int(L1D_store_miss_data[2])

                        L1D_total = L1D_total_loads + L1D_total_stores

                        L1D_miss_rate = (int(L1D_load_miss_data[2]) + int(L1D_store_miss_data[2])) / L1D_total
                        print("Iteration %d L1D miss rate = %f" % (itr, L1D_miss_rate))

                    if ("L2 Unified Cache" in out_lines[line_idx]):
                        L2_load_hit_data = out_lines[line_idx + 1].split()
                        L2_load_hits.append(int(L2_load_hit_data[2]))

                        L2_load_miss_data = out_lines[line_idx + 2].split()
                        L2_load_misses.append(int(L2_load_miss_data[2]))

                        L2_total_loads = int(L2_load_hit_data[2]) + int(L2_load_miss_data[2])

                        L2_store_hit_data = out_lines[line_idx + 5].split()
                        L2_store_hits.append(int(L2_store_hit_data[2]))

                        L2_store_miss_data = out_lines[line_idx + 6].split()
                        L2_store_misses.append(int(L2_store_miss_data[2]))

                        L2_total_stores = int(L2_store_hit_data[2]) + int(L2_store_miss_data[2])

                        L2_total = L2_total_loads + L2_total_stores

                        L2_miss_rate = (int(L2_load_miss_data[2]) + int(L2_store_miss_data[2])) / L2_total
                        print("Iteration %d L2 miss rate = %f" % (itr, L2_miss_rate))


                AMAT_I = L1_hit_latency + L1I_miss_rate * (L2_hit_latency + L2_miss_rate * Mem_hit_latency)
                AMAT_Imem.append(AMAT_I)
                AMAT_D = L1_hit_latency + L1D_miss_rate * (L2_hit_latency + L2_miss_rate * Mem_hit_latency)
                AMAT_Dmem.append(AMAT_D)

                print("Iteration %d: AMAT[Imem, Dmem] is [%f, %f]" % (itr, AMAT_I, AMAT_D))


                # calculating the fraction of memory accesses that go to data memory
                data_frac = L1D_total / (L1D_total + L1I_total)
                data_fraction.append(data_frac)

                AMAT = (data_frac * AMAT_D) + ((1 - data_frac) * AMAT_I)
                AMAT_total.append(AMAT)

                print("Iteration %d: Memory access mix is %f data accesses" % (itr, data_frac))
                print("Iteration %d: Memory AMAT is %f" % (itr, AMAT))

            print("Data Dump")

            print("L1I Load Hits")
            print(L1I_load_hits)
            print("L1I Load Misses")
            print(L1I_load_misses)

            print("L1D Load Hits")
            print(L1D_load_hits)
            print("L1D Load Misses")
            print(L1D_load_misses)
            print("L1D Store Hits")
            print(L1D_store_hits)
            print("L1D Store Misses")
            print(L1D_store_misses)

            print("L2 Load Hits")
            print(L2_load_hits)
            print("L2 Load Misses")
            print(L2_load_misses)
            print("L2 Store Hits")
            print(L2_store_hits)
            print("L2 Store Misses")
            print(L2_store_misses)

            print("Instruction AMAT")
            print(AMAT_Imem)
            print("Data AMAT")
            print(AMAT_Dmem)
            print("Fraction of total accesses related to data")
            print(data_fraction)
            print("Total AMAT")
            print(AMAT_total)

            AMAT_avg = sum(AMAT_total) / iterations
            AMAT_key = "L1 size = " + l1s + " kB, L2 size = " + l2s + " kB"
            AMAT_avgs[AMAT_key] = AMAT_avg

    print(AMAT_avgs)