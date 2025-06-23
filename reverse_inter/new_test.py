import subprocess
import re
import time
import csv
import argparse
import json
import itertools


def run_test(outfile:str,prio1: int, prio2: int,x:str, dev: str, test_file: str, f1_: list, f2_: list, s1_: list, s2_: list, q1_: list, q2_: list, l1_: list, l2_: list, n1_: list, n2_: list):
    prio1,prio2 = str(prio1),str(prio2)
    
    # for f1, f2, s1, s2, q1, q2, l1, l2, n1, n2 in zip(f1_, f2_, s1_, s2_, q1_, q2_, l1_, l2_, n1_, n2_):
    for f1, f2, s1, s2, q1, q2, l1, l2, n1, n2 in itertools.product(f1_, f2_, s1_, s2_, q1_, q2_, l1_, l2_, n1_, n2_):
        while(1):
            print(f1, f2, s1, s2, q1, q2, l1, l2, n1, n2)
            
            f1,f2,s1,s2,q1,q2,l1,l2,n1,n2 = str(f1),str(f2),str(s1),str(s2),str(q1),str(q2),str(l1),str(l2),str(n1),str(n2)
            subprocess.run(["bash", "./kill_traffic.sh"])
            print("kill_traffic")
            
            print("bash", test_file, f1, str(0), s1, s2, q1, q2, l1, l2, n1, n2)
            subprocess.run(["bash", test_file, f1, str(0), s1, s2, q1, q2, l1, l2, n1, n2]) # f2=0 无attacker
            
            time.sleep(3)
            print("run")
            
            result1 = subprocess.run(["bash", "./bw_monitor.sh", dev, x, prio1], stdout=subprocess.PIPE)
            
            result1 = result1.stdout.decode().split("\n")
            # result1 = list(map(float, result1))
            try:
                result1 = list(map(float, result1[0:6]))
            except:
                print("error")
                print(result1)
                exit(0)
            times=(result1[3]-result1[0])/1e9
            bytes=(result1[4]-result1[1])
            packets=(result1[5]-result1[2])
            bps_unattacked=bytes/times
            pps_unattacked=packets/times
            print(bps_unattacked,pps_unattacked)

            if bps_unattacked==0 or pps_unattacked==0:
                print("error")
                continue
            else:
                with open(outfile, "a") as f:
                    writer = csv.writer(f)
                    # writer.writerow([f1, f2, s1, s2, q1, q2, l1, l2, n1, n2, bps_unattacked, pps_unattacked, bps_attacked, pps_attacked, bps_attacker, pps_attacker, bps_drop, pps_drop])
                    writer.writerow([f1, f2, s1, s2, q1, q2, l1, l2, n1, n2, bps_unattacked, pps_unattacked])
                break
    return

def main():

    parser = argparse.ArgumentParser()
    parser.add_argument("--config", type=str, default="config.json")
    parser.add_argument("--output", type=str, default="output.csv")
    args = parser.parse_args()
    with open(args.config, "r") as f:
        config = json.load(f)
    
    dev = config["dev"]
    test_file = config["test_file"]
    f1 = config["f1"]
    f2 = config["f2"]
    s1 = config["s1"]
    s2 = config["s2"]
    # print(s1,s2)
    q1 = config["q1"]
    q2 = config["q2"]
    l1 = config["l1"]
    l2 = config["l2"]
    n1 = config["n1"]
    n2 = config["n2"]
    prio1 = config["prio1"]
    prio2 = config["prio2"]
    x = config["x"]
    
    with open(args.output, "w") as f:
        writer = csv.writer(f)
        writer.writerow(["f1", "f2", "s1", "s2", "q1", "q2", "l1", "l2", "n1", "n2", "bps_unattacked", "pps_unattacked"])
    run_test(args.output ,prio1, prio2, x, dev, test_file, f1, f2, s1, s2, q1, q2, l1, l2, n1, n2)
    
    subprocess.run(["bash", "./kill_traffic.sh"])

if __name__ == "__main__":
    main()