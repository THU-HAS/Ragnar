import glob
import argparse
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import os

def read_data(batch, delay):
    
    filename=batch[0]
    period=batch[1]
    t_l = [[],[]]
    # y_l = [[],[]]
    q_l = [[],[]]
    r_l = [[],[]]
    p_l = [[],[]]
    with open(filename, 'r') as file:
        # print("file %s opened"%(filename))
        for line in file:
            t, n, y, q = line.strip().split()
            tt = int(t)-delay
            if(tt >= 0):
                res = tt%(period*2)
                n = int(n)
                t_l[n].append(res/(period))
                p_l[n].append(int((tt-res)/(period*2)))

                # y_l[n].append(float(y))
                q_l[n].append(float(q)+1)
                r_l[n].append(float(y)/(float(q)+1))
    
    return (t_l, p_l, q_l, r_l)

def process_one(config, savedir, step, thres):

    # print("++ processing "+config[0])
    
    period = config[1]

    (t_l, p_l, q_l, r_l) = read_data(config, 0)
    
    qpidx=0
    t=t_l[qpidx]
    p=p_l[qpidx]
    q=q_l[qpidx]
    r=r_l[qpidx]
    
    p99,p50,p01 = np.percentile(r, [99,50,1])
    reserved_idx = [i for i,x in enumerate(r)]
    # reserved_idx = [i for i,x in enumerate(r) if abs(x-p50) <= p99-p01]
    t = [t[i] for i in reserved_idx]
    p = [p[i] for i in reserved_idx]
    q = [q[i] for i in reserved_idx]
    r = [r[i] for i in reserved_idx]
    
    buckets = np.zeros(200)
    buckets_cnt = np.zeros(200)
    for i in range(len(t)):
        buckets[int(t[i]*100)] += r[i] - p50
        buckets_cnt[int(t[i]*100)] += 1
    for i in range(200):
        if(buckets_cnt[i] == 0):
            buckets_cnt[i] = 1
    avg = buckets / buckets_cnt
    result = np.zeros(200)
    for rlat in range(200):
        result[rlat] = (np.sum(avg[rlat-200:rlat-100]) + np.sum(avg[rlat:rlat+100]))/100 
    # print(avg)
    # print(result)

    index=np.argmax(abs(result))
    if(result[index] > 0):
        delay = period * ((1 + index/100.0)%2)
    else:
        delay = period * ((index/100.0)%2)
    # print(delay)

    
    # if(not os.path.exists(os.path.join(savedir,"moddd_%d.png"%(period)))):
    #     N = 100000
    #     plt.figure(dpi=400)
    #     plt.scatter(t[0:N], r[0:N],c=q[0:N], cmap='viridis', s=1, alpha=0.2)
    #     plt.plot(np.array(range(200))*0.01, p50+avg, color="red")
    #     plt.plot(np.array(range(200))*0.01, p50+result, color="blue")
    #     plt.plot(np.array([delay/period,delay/period]),np.array([4000,6000]),"--")
    #     plt.colorbar()
    #     plt.xlim([0,2])
    #     plt.savefig(os.path.join(savedir,"moddd_%d.png"%(period)), format='png')
    #     plt.close()

    
    (t_l, p_l, q_l, r_l) = read_data(config, delay)
    
    qpidx=0
    t=t_l[qpidx]
    p=p_l[qpidx]
    q=q_l[qpidx]
    r=r_l[qpidx]
    
    p99,p50,p01 = np.percentile(r, [99,50,1])

    reserved_idx = [i for i,x in enumerate(r) if abs(x-p50) <= p99-p01]
    t = [t[i] for i in reserved_idx]
    p = [p[i] for i in reserved_idx]
    q = [q[i] for i in reserved_idx]
    r = [r[i] for i in reserved_idx]

    # print([np99,np01])
    p_start = 0
    idx_start = 0
    p_step = step

    tvalid = 0
    tcorrect = 0
    tspace = 0
    for i in range(len(t)):
        if(p[i]-p_start >= p_step):
            bits0 = np.zeros(p_step)
            bits1 = np.zeros(p_step)
            counts0 = np.zeros(p_step)
            counts1 = np.zeros(p_step)
            med = np.median(r[idx_start:i])
            for index in range(idx_start,i):
                if(abs(t[index]-0.5)<=thres):
                    bits0[p[index]-p_start] += r[index]-med
                    counts0[p[index]-p_start] += 1
                elif(abs(t[index]-1.5)<=thres):
                    bits1[p[index]-p_start] += -r[index]+med
                    counts1[p[index]-p_start] += 1        
            correct0 = np.sum(bits0 > 0)
            valid0 = np.sum(counts0 > 0)
            correct1 = np.sum(bits1 > 0)
            valid1 = np.sum(counts1 > 0)
            # print("[%5d-%5d] bit0 %4d of %4d  , bit1 %4d of %4d"%(p_start,p_start+p_step-1, correct0, valid0, correct1, valid1))
            tvalid += valid0 + valid1
            tcorrect += correct0 + correct1
            tspace += p_step
            p_start = p[i]
            idx_start = i
    
    print("%6d, %.3f %%, %.3f kHz, %.6d"%(period, tcorrect/tvalid*100, 2.4e6/period*tvalid/tspace/2, delay))



        

    if(not os.path.exists(os.path.join(savedir,"mod_%d.png"%(period)))):
        N = 100000
        plt.figure(dpi=400)
        plt.scatter(t[0:N], r[0:N],c=q[0:N], cmap='viridis', s=1, alpha=0.2)
        plt.colorbar()
        plt.xlim([0,2])
        plt.savefig(os.path.join(savedir,"mod_%d.png"%(period)), format='png')
        plt.close()



    

    

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('-dir', type=str, help='config dir')
    parser.add_argument('-step', type=int, help='step', default=10)
    parser.add_argument('-delay', type=int, help='delay', default=115942)
    parser.add_argument('-thres', type=float, help='thres', default=0.3)
    args = parser.parse_args()

    dir = args.dir
    step = args.step
    delay = args.delay
    thres = args.thres

    pattern = "./"+dir+"/result_config_*.txt"
    files = glob.glob(pattern)
    configs = []


    for file in files:
        star_value = file[3+len(dir)+len('result_config_'):-len('.txt')]
        configs.append((file, float(star_value)))
    
    configs.sort(key=lambda x:x[1], reverse=True)
    print("## [%s] [step = %d] [thres = %f]"%(dir, step, thres))
    for config in configs:
        process_one(config, "./"+dir, step, thres)
    

